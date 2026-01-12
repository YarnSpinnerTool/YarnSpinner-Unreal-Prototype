// copyright yarn spinner pty ltd
// licensed under the mit license

#include "YarnAssetFactory.h"
#include "YarnProgram.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

UYarnAssetFactory::UYarnAssetFactory()
{
	// configure factory for .yarnproject and .yarn files
	SupportedClass = UYarnProgram::StaticClass();
	Formats.Add(TEXT("yarnproject;Yarn Spinner Project"));
	Formats.Add(TEXT("yarn;Yarn Dialogue Script"));

	bCreateNew = false;
	bEditorImport = true;
	bText = false;
}

bool UYarnAssetFactory::FactoryCanImport(const FString& Filename)
{
	// support both .yarnproject and individual .yarn files
	FString Extension = FPaths::GetExtension(Filename);
	return Extension.Equals(TEXT("yarnproject"), ESearchCase::IgnoreCase) ||
	       Extension.Equals(TEXT("yarn"), ESearchCase::IgnoreCase);
}

UObject* UYarnAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn,
	bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;

	UE_LOG(LogTemp, Log, TEXT("importing yarn file: %s"), *Filename);

	// create temporary directory for ysc output
	FString TempDir = FPaths::ProjectIntermediateDir() / TEXT("YarnCompile") / FGuid::NewGuid().ToString();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectory(*TempDir);

	// get output name from asset name
	FString OutputName = InName.ToString();

	// determine if we're importing a .yarn or .yarnproject file
	FString Extension = FPaths::GetExtension(Filename);
	FString ProjectPath = Filename;

	// if importing a single .yarn file, create a temporary .yarnproject
	if (Extension.Equals(TEXT("yarn"), ESearchCase::IgnoreCase))
	{
		ProjectPath = TempDir / TEXT("Temp.yarnproject");

		// create minimal .yarnproject json file
		FString ProjectJson = FString::Printf(
			TEXT("{\"fileVersion\":2,\"projectFileVersion\":2,\"files\":[\"%s\"],\"baseLanguage\":\"en\"}"),
			*Filename.Replace(TEXT("\\"), TEXT("\\\\"))
		);

		if (!FFileHelper::SaveStringToFile(ProjectJson, *ProjectPath))
		{
			UE_LOG(LogTemp, Error, TEXT("failed to create temporary .yarnproject file"));
			PlatformFile.DeleteDirectoryRecursively(*TempDir);
			bOutOperationCanceled = true;
			return nullptr;
		}

		UE_LOG(LogTemp, Log, TEXT("created temporary project: %s"), *ProjectPath);
	}

	// compile the yarn project
	FString CompileError;
	if (!CompileYarnProject(ProjectPath, TempDir, OutputName, CompileError))
	{
		UE_LOG(LogTemp, Error, TEXT("yarn compilation failed: %s"), *CompileError);

		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("yarn compilation failed: %s"), *CompileError);
		}

		// clean up temp directory
		PlatformFile.DeleteDirectoryRecursively(*TempDir);
		return nullptr;
	}

	// read the compiled .yarnc file (just raw bytes - no parsing!)
	FString YarncPath = TempDir / (OutputName + TEXT(".yarnc"));
	TArray<uint8> YarncBytecode;

	if (!FFileHelper::LoadFileToArray(YarncBytecode, *YarncPath))
	{
		UE_LOG(LogTemp, Error, TEXT("failed to read compiled .yarnc file: %s"), *YarncPath);
		PlatformFile.DeleteDirectoryRecursively(*TempDir);
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("loaded %d bytes of compiled bytecode"), YarncBytecode.Num());

	// create the yarn program asset
	UYarnProgram* YarnProgram = NewObject<UYarnProgram>(InParent, InClass, InName, Flags);

	// store the raw bytecode - that's it! no parsing, no conversion
	YarnProgram->CompiledBytecode = MoveTemp(YarncBytecode);

	// load string table from csv
	FString StringTablePath = TempDir / (OutputName + TEXT("-Lines.csv"));
	if (FPaths::FileExists(StringTablePath))
	{
		LoadStringTableFromCSV(YarnProgram, StringTablePath);
		UE_LOG(LogTemp, Log, TEXT("loaded %d strings from string table"), YarnProgram->StringTable.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("no string table found at: %s"), *StringTablePath);
	}

	// load metadata (optional)
	FString MetadataPath = TempDir / (OutputName + TEXT("-Metadata.csv"));
	if (FPaths::FileExists(MetadataPath))
	{
		LoadMetadataFromCSV(YarnProgram, MetadataPath);
		UE_LOG(LogTemp, Log, TEXT("loaded %d metadata entries"), YarnProgram->LineMetadata.Num());
	}

	// clean up temp directory
	PlatformFile.DeleteDirectoryRecursively(*TempDir);

	// mark package dirty so unreal saves the asset
	YarnProgram->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("successfully imported yarn project: %s"), *InName.ToString());

	return YarnProgram;
}

bool UYarnAssetFactory::CompileYarnProject(const FString& ProjectPath, const FString& OutputDir,
	const FString& OutputName, FString& OutError)
{
	// get ysc compiler path
	FString YscPath = GetYscPath();

	if (!FPaths::FileExists(YscPath))
	{
		OutError = FString::Printf(TEXT("ysc compiler not found at: %s"), *YscPath);
		return false;
	}

	// build ysc command line arguments
	// ysc compile --output-directory <dir> --output-name <name> <project.yarnproject>
	FString Arguments = FString::Printf(
		TEXT("compile --output-directory \"%s\" --output-name \"%s\" \"%s\""),
		*OutputDir,
		*OutputName,
		*ProjectPath
	);

	UE_LOG(LogTemp, Log, TEXT("running ysc: %s %s"), *YscPath, *Arguments);

	// execute ysc compiler
	int32 ReturnCode;
	FString StdOut;
	FString StdErr;

	FPlatformProcess::ExecProcess(*YscPath, *Arguments, &ReturnCode, &StdOut, &StdErr);

	// log output
	if (!StdOut.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("ysc output: %s"), *StdOut);
	}

	if (!StdErr.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ysc stderr: %s"), *StdErr);
	}

	// check return code
	if (ReturnCode != 0)
	{
		OutError = FString::Printf(TEXT("ysc exited with code %d: %s"), ReturnCode, *StdErr);
		return false;
	}

	return true;
}

void UYarnAssetFactory::LoadStringTableFromCSV(UYarnProgram* Program, const FString& CSVPath)
{
	// load csv file content
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *CSVPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("failed to load string table csv: %s"), *CSVPath);
		return;
	}

	// parse csv line by line
	// format: id,text,file,node,lineNumber
	TArray<FString> Lines;
	CSVContent.ParseIntoArrayLines(Lines);

	// skip header row (index 0)
	for (int32 i = 1; i < Lines.Num(); i++)
	{
		// simple csv parsing - handles quoted fields
		// yarn spinner csv output is straightforward, so we don't need a full csv parser
		FString Line = Lines[i].TrimStartAndEnd();

		if (Line.IsEmpty())
		{
			continue;
		}

		// find first comma to separate line id from text
		// text might contain commas, so we only split on the first one
		int32 FirstComma;
		if (Line.FindChar(TEXT(','), FirstComma))
		{
			FString LineID = Line.Left(FirstComma).TrimQuotes();

			// remaining part contains: text,file,node,lineNumber
			// we only care about the text (second field)
			FString Remaining = Line.Mid(FirstComma + 1);

			// find second comma to extract just the text field
			int32 SecondComma;
			FString Text;

			if (Remaining.FindChar(TEXT(','), SecondComma))
			{
				Text = Remaining.Left(SecondComma).TrimQuotes();
			}
			else
			{
				// no third field, so text is the rest
				Text = Remaining.TrimQuotes();
			}

			// unescape csv escaped characters
			Text.ReplaceInline(TEXT("\"\""), TEXT("\""));

			Program->StringTable.Add(LineID, Text);
		}
	}
}

void UYarnAssetFactory::LoadMetadataFromCSV(UYarnProgram* Program, const FString& CSVPath)
{
	// similar to string table loading
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *CSVPath))
	{
		return;
	}

	TArray<FString> Lines;
	CSVContent.ParseIntoArrayLines(Lines);

	// metadata csv format varies, but typically: id,metadata
	// we store it as-is for now
	for (int32 i = 1; i < Lines.Num(); i++)
	{
		FString Line = Lines[i].TrimStartAndEnd();

		if (Line.IsEmpty())
		{
			continue;
		}

		int32 FirstComma;
		if (Line.FindChar(TEXT(','), FirstComma))
		{
			FString LineID = Line.Left(FirstComma).TrimQuotes();
			FString Metadata = Line.Mid(FirstComma + 1).TrimQuotes();

			Program->LineMetadata.Add(LineID, Metadata);
		}
	}
}

FString UYarnAssetFactory::GetYscPath() const
{
	// ysc path is defined in build.cs via YSC_PATH macro
	// fallback to looking in plugin directory if not defined

#ifdef YSC_PATH
	return YSC_PATH;
#else
	// fallback - look for ysc in plugin tools directory
	FString PluginDir = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectPluginsDir() / TEXT("YarnSpinner")
	);

#if PLATFORM_WINDOWS
	return PluginDir / TEXT("Tools/Win64/ysc.exe");
#elif PLATFORM_MAC
	return PluginDir / TEXT("Tools/Mac/ysc");
#else
	return TEXT("ysc"); // hope it's in PATH
#endif
#endif
}
