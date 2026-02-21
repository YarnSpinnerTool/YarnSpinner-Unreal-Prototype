// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "YarnSpinnerEditor.h"


#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YarnProject.h"
#include "EditorReimportHandler.h"
#include "LocalizationTargetTypes.h"

THIRD_PARTY_INCLUDES_START
#include "YarnSpinnerCore/compiler_output.pb.h"
THIRD_PARTY_INCLUDES_END

#include "YarnAssetFactory.generated.h"


/**
 * 
 */
UCLASS(hidecategories=Object)
class UYarnAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	virtual UObject* FactoryCreateBinary
	(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		const TCHAR* Type,
		const uint8*& Buffer,
		const uint8* BufferEnd,
		FFeedbackContext* Warn
	) override;

	virtual bool FactoryCanImport(const FString& Filename) override;

	EReimportResult::Type Reimport(UYarnProject* YarnProject);

	static FString YscPath();

	static bool GetCompiledDataForYarnProject(const TCHAR* InFilePath, Yarn::CompilerOutput& CompilerOutput);
	static bool GetSourcesForProject(const TCHAR* InFilePath, TArray<FString>& SourceFiles);
	static bool GetSourcesForProject(const UYarnProject* YarnProjectAsset, TArray<FString>& SourceFiles);
    
private:
    // Set up localization target for the Yarn project
    void BuildLocalizationTarget(const UYarnProject* YarnProject, const Yarn::CompilerOutput& CompilerOutput) const;
    // Compile all texts
    static void CompileTexts(const ULocalizationTarget* LocalizationTarget);
};
