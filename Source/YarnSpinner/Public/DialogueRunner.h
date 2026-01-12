// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YarnVirtualMachine.h"
#include "DialogueRunner.generated.h"

/**
 * blueprint-exposed option data
 */
USTRUCT(BlueprintType)
struct FDialogueOption
{
	GENERATED_BODY()

	// the line id for this option
	UPROPERTY(BlueprintReadOnly, Category = "Yarn Spinner")
	FString LineID;

	// the localized text to display
	UPROPERTY(BlueprintReadOnly, Category = "Yarn Spinner")
	FString Text;

	// whether this option is available
	UPROPERTY(BlueprintReadOnly, Category = "Yarn Spinner")
	bool bIsAvailable;

	// the index of this option
	UPROPERTY(BlueprintReadOnly, Category = "Yarn Spinner")
	int32 Index;

	FDialogueOption()
		: bIsAvailable(true)
		, Index(0)
	{
	}
};

/**
 * dialogue runner component
 *
 * this is the main blueprint interface to yarn spinner
 * add this component to an actor to run yarn dialogue
 *
 * usage:
 * 1. set the yarn program asset
 * 2. implement the blueprint events (on line received, etc.)
 * 3. call start dialogue with a starting node name
 * 4. call continue to advance dialogue
 * 5. call select option when presenting choices
 */
UCLASS(ClassGroup=(YarnSpinner), meta=(BlueprintSpawnableComponent))
class YARNSPINNER_API UDialogueRunner : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogueRunner();

	/**
	 * configuration
	 */

	// the yarn program to run
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner")
	class UYarnProgram* YarnProgram;

	// whether to automatically start dialogue when the component begins play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner")
	bool bAutoStart;

	// the node to start at when auto-starting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner", meta = (EditCondition = "bAutoStart"))
	FString StartNode;

	// optional dialogue presenter component
	// if set, the presenter will handle ui display instead of blueprint events
	// leave empty to use blueprint events directly
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner")
	TScriptInterface<class IDialoguePresenter> DialoguePresenter;

	/**
	 * blueprint api
	 */

	// start dialogue at the specified node
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void StartDialogue(const FString& NodeName = TEXT("Start"));

	// continue dialogue after a line has been delivered
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void Continue();

	// select an option (when options are presented)
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void SelectOption(int32 OptionIndex);

	// stop the current dialogue
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void StopDialogue();

	// check if dialogue is currently running
	UFUNCTION(BlueprintPure, Category = "Yarn Spinner")
	bool IsDialogueRunning() const;

	/**
	 * variable access
	 */

	// get a boolean variable
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	bool GetBoolVariable(const FString& VariableName, bool DefaultValue = false);

	// set a boolean variable
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void SetBoolVariable(const FString& VariableName, bool Value);

	// get a number variable
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	float GetNumberVariable(const FString& VariableName, float DefaultValue = 0.0f);

	// set a number variable
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void SetNumberVariable(const FString& VariableName, float Value);

	// get a string variable
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	FString GetStringVariable(const FString& VariableName, const FString& DefaultValue = TEXT(""));

	// set a string variable
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void SetStringVariable(const FString& VariableName, const FString& Value);

	/**
	 * blueprint events
	 * implement these in your blueprint to respond to dialogue
	 */

	// called when a line should be displayed
	// implement this to show dialogue ui
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner", meta = (DisplayName = "On Line Received"))
	void OnLineReceived(const FString& LineID, const FString& LineText);

	// called when a command is executed
	// implement this to handle game commands
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner", meta = (DisplayName = "On Command Received"))
	void OnCommandReceived(const FString& CommandText);

	// called when options should be presented
	// implement this to show choice ui
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner", meta = (DisplayName = "On Options Received"))
	void OnOptionsReceived(const TArray<FDialogueOption>& Options);

	// called when a node starts
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner", meta = (DisplayName = "On Node Start"))
	void OnNodeStart(const FString& NodeName);

	// called when a node completes
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner", meta = (DisplayName = "On Node Complete"))
	void OnNodeComplete(const FString& NodeName);

	// called when all dialogue is complete
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner", meta = (DisplayName = "On Dialogue Complete"))
	void OnDialogueComplete();

	/**
	 * ue component lifecycle
	 */

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// the virtual machine that executes dialogue
	FYarnVirtualMachine VirtualMachine;

private:
	// convert internal option to blueprint option
	FDialogueOption ConvertOption(const FYarnOption& InternalOption) const;

	// vm event handlers
	void HandleLine(const FString& LineID, const TArray<FString>& Substitutions);
	void HandleCommand(const FString& CommandText);
	void HandleOptions(const TArray<FYarnOption>& Options);
	void HandleNodeStart(const FString& NodeName);
	void HandleNodeComplete(const FString& NodeName);
	void HandleDialogueComplete();
};
