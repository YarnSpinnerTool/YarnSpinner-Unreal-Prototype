// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YarnProject.h"

THIRD_PARTY_INCLUDES_START
#include "YarnSpinnerCore/VirtualMachine.h"
#include "YarnSpinnerCore/Library.h"
#include "YarnSpinnerCore/Common.h"
THIRD_PARTY_INCLUDES_END

#include "DialogueRunner.generated.h"

DECLARE_DELEGATE(FYarnDialogueRunnerContinueDelegate);

UCLASS()
class YARNSPINNER_API ADialogueRunner : public AActor, public Yarn::ILogger, public Yarn::IVariableStorage
{
    GENERATED_BODY()
    
public:
    // Sets default values for this actor's properties
    ADialogueRunner();

protected:
    virtual void PreInitializeComponents() override;
    
public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintNativeEvent, Category="Dialogue Runner")
    void OnDialogueStarted();
    
    UFUNCTION(BlueprintNativeEvent, Category="Dialogue Runner")
    void OnDialogueEnded();
    
    UFUNCTION(BlueprintNativeEvent, Category="Dialogue Runner")
    void OnRunLine(class ULine* Line, const TArray<TSoftObjectPtr<UObject>>& LineAssets);

    UFUNCTION(BlueprintNativeEvent, Category="Dialogue Runner")
    void OnRunOptions(const TArray<class UOption*>& Options);

    UFUNCTION(BlueprintNativeEvent, Category="Dialogue Runner")
    void OnRunCommand(const FString& Command, const TArray<FString>& Parameters);
    
    UFUNCTION(BlueprintCallable, Category="Dialogue Runner")
    void StartDialogue(FName NodeName);
    
    UFUNCTION(BlueprintCallable, Category="Dialogue Runner")
    void ContinueDialogue();

    // TODO: add StopDialogue() blueprint callback
    
    UFUNCTION(BlueprintCallable, Category="Dialogue Runner")
    void SelectOption(UOption* Option);
    
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Dialogue Runner")
    UYarnProject* YarnProject;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Dialogue Runner")
    bool bRunSelectedOptionsAsLines = false;

private:
    TUniquePtr<Yarn::VirtualMachine> VirtualMachine;

    TUniquePtr<Yarn::Library> Library;

    FYarnDialogueRunnerContinueDelegate ContinueDelegate;

    // ILogger
    virtual void Log(std::string Message, Type Severity = Type::INFO) override;

    // IVariableStorage
    virtual void SetValue(std::string Name, bool bValue) override;
    virtual void SetValue(std::string Name, float Value) override;
    virtual void SetValue(std::string Name, std::string Value) override;

    virtual bool HasValue(std::string Name) override;
    virtual Yarn::Value GetValue(std::string Name) override;

    virtual void ClearValue(std::string Name) override;

    FString GetLine(FName LineID, FName Language);

    UPROPERTY()
    FString Blah;

    class UYarnSubsystem* YarnSubsystem() const;
    
    void GetDisplayTextForLine(class ULine* Line, const Yarn::Line& YarnLine);
};
