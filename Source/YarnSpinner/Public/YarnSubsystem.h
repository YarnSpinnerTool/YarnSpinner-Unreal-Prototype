// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/YarnLibraryRegistry.h"
#include "Engine/ObjectLibrary.h"
#include "YarnSpinnerCore/VirtualMachine.h"
#include "YarnSubsystem.generated.h"


/**
 * 
 */
UCLASS()
class YARNSPINNER_API UYarnSubsystem : public UGameInstanceSubsystem, public Yarn::IVariableStorage
{
    GENERATED_BODY()
public:
    UYarnSubsystem();

    static UYarnSubsystem* Get();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void SetValue(std::string name, bool value) override;
    virtual void SetValue(std::string name, float value) override;
    virtual void SetValue(std::string name, std::string value) override;

    virtual bool HasValue(std::string name) override;
    virtual Yarn::Value GetValue(std::string name) override;

    virtual void ClearValue(std::string name) override;

    const UYarnLibraryRegistry* GetYarnLibraryRegistry() const { return YarnFunctionRegistry; }

private:
    UPROPERTY()
    UYarnLibraryRegistry* YarnFunctionRegistry;

    UPROPERTY()
    UObjectLibrary* YarnFunctionObjectLibrary;
    UPROPERTY()
    UObjectLibrary* YarnCommandObjectLibrary;
    
    TMap<FString, Yarn::Value> Variables;
    
    FDelegateHandle OnAssetRegistryFilesLoadedHandle;
    FDelegateHandle OnLevelAddedToWorldHandle;
    FDelegateHandle OnWorldInitializedActorsHandle;

    void LogVariables();
};



