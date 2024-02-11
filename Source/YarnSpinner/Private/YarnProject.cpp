// Fill out your copyright notice in the Description page of Project Settings.


#include "YarnProject.h"

#include "YarnSpinner.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/DataTable.h"
#include "Misc/YarnAssetHelpers.h"
#include "Misc/YSLogging.h"


void UYarnProject::Init()
{
    // Find related line assets
    LineAssets.Empty();
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FARFilter Filter;
    Filter.bRecursivePaths = true;
    Filter.PackagePaths.Add("/Game");
    TArray<FAssetData> AssetData;
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    for (auto Asset : AssetData)
    {
        YS_LOG_FUNC("Found asset: %s", *Asset.AssetName.ToString());
        auto LineId = FName(TEXT("line:") + Asset.AssetName.ToString());
        if (Lines.Contains(LineId))
        {
            // TODO: if path contains a loc identifier, add to loc table
            LineAssets.FindOrAdd(LineId).Add(TSoftObjectPtr<UObject>(Asset.ToSoftObjectPath()));
        }
    }
}


FString UYarnProject::GetLocAssetPackage() const
{
    return FPaths::Combine(FPaths::GetPath(GetPathName()), GetName() + TEXT("_Loc"));
}


FString UYarnProject::GetLocAssetPackage(const FName Language) const
{
    return FPaths::Combine(GetLocAssetPackage(), Language.ToString());
}


UDataTable* UYarnProject::GetLocTextDataTable(const FName Language) const
{
    const FString LocalisedAssetPackage = GetLocAssetPackage(Language);
    
    TArray<FAssetData> AssetData = FYarnAssetHelpers::FindAssetsInRegistryByPackagePath<UDataTable>(LocalisedAssetPackage);

    if (AssetData.Num() == 0)
        return nullptr;
        
    UDataTable* DataTable = Cast<UDataTable>(AssetData[0].GetAsset());
    return DataTable;
}


TArray<TSoftObjectPtr<UObject>> UYarnProject::GetLineAssets(const FName Name)
{
    if (!LineAssets.Contains(Name))
        return {};
    return LineAssets[Name];
}


#if WITH_EDITORONLY_DATA
void UYarnProject::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData)
	{
		OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}

	Super::GetAssetRegistryTags(OutTags);
}
#endif


void UYarnProject::PostInitProperties()
{
#if WITH_EDITORONLY_DATA

	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
	Super::PostInitProperties();
}


void UYarnProject::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	if (AssetImportData == nullptr)
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
}


#if WITH_EDITORONLY_DATA
void UYarnProject::SetYarnSources(const TArray<FString>& NewYarnSources)
{
	const FString ProjectPath = YarnProjectPath();
	YarnFiles.Reset();

	YS_LOG("Setting yarn project asset sources for project: %s", *ProjectPath);
	for (const FString& SourceFile : NewYarnSources)
	{
		const FString FullPath = FPaths::IsRelative(SourceFile) ? FPaths::Combine(ProjectPath, SourceFile) : SourceFile;
		if (!FPaths::FileExists(FullPath))
		{
			YS_WARN("Yarn source file '%s' does not exist.", *FullPath);
			continue;
		}
		YarnFiles.Add(
			SourceFile,
			FYarnSourceMeta {
			IFileManager::Get().GetTimeStamp(*FullPath),
			LexToString(FMD5Hash::HashFile(*FullPath)) 
			}
		);
		YS_LOG("--> set source file %s - %s - %s", *SourceFile, *YarnFiles.FindChecked(SourceFile).Timestamp.ToString(), *YarnFiles.FindChecked(SourceFile).FileHash);
	}
}
#endif


#if WITH_EDITORONLY_DATA
bool UYarnProject::ShouldRecompile(const TArray<FString>& LatestYarnSources) const
{
	const FString ProjectPath = YarnProjectPath();

	YS_LOG("Checking if should recompile for project: %s", *ProjectPath);
	YS_LOG("Sources included in last compile:");
	for (auto OriginalSource : YarnFiles)
	{
		YS_LOG("--> %s", *OriginalSource.Key);
	}
	YS_LOG("Latest yarn sources:");
	for (auto NewSource : LatestYarnSources)
	{
		YS_LOG("--> %s", *NewSource);
	}

	if (YarnFiles.Num() != LatestYarnSources.Num())
	{
		return true;
	}

	for (auto OriginalSource : YarnFiles)
	{
		if (!LatestYarnSources.Contains(OriginalSource.Key))
		{
			YS_LOG("Original source file %s not in latest yarn sources", *OriginalSource.Key);
			return true;
		}
	}

	// it's all the same files, so compare timestamps and hashes
	for (auto OriginalSource : YarnFiles)
	{
		const FString FullPath = FPaths::IsRelative(OriginalSource.Key) ? FPaths::Combine(ProjectPath, OriginalSource.Key) : OriginalSource.Key;
		if (!FPaths::FileExists(FullPath))
		{
			YS_WARN("Yarn source file '%s' does not exist.", *FullPath);
			return true;
		}
		
		if (IFileManager::Get().GetTimeStamp(*FullPath) != OriginalSource.Value.Timestamp)
		{
			if (LexToString(FMD5Hash::HashFile(*FullPath)) != OriginalSource.Value.FileHash)
			{
				YS_LOG("Source file %s has changed", *OriginalSource.Key);
				return true;
			}
		}
	}
	
	return false;
}
#endif


#if WITH_EDITORONLY_DATA
FString UYarnProject::YarnProjectPath() const
{
	return FPaths::GetPath(AssetImportData->GetFirstFilename());
}
#endif
