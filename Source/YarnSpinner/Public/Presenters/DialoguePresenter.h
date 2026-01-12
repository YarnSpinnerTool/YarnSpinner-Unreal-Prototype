// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DialoguePresenter.generated.h"

/**
 * interface for presenting dialogue to the player
 *
 * implement this interface to create custom dialogue ui
 * the dialogue runner will call these methods when dialogue events occur
 *
 * this is similar to unity's IDialoguePresenter pattern
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UDialoguePresenter : public UInterface
{
	GENERATED_BODY()
};

class IDialoguePresenter
{
	GENERATED_BODY()

public:
	/**
	 * called when a line should be displayed
	 * implement this to show the line in your ui
	 *
	 * call the completion delegate when the line presentation is complete
	 * (e.g., after the player presses continue)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Yarn Spinner")
	void PresentLine(const FString& LineID, const FString& LineText);

	/**
	 * called when options should be presented
	 * implement this to show the options in your ui
	 *
	 * call the completion delegate with the selected option index
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Yarn Spinner")
	void PresentOptions(const TArray<struct FDialogueOption>& Options);

	/**
	 * called when a line presentation should be interrupted
	 * (e.g., player skipped ahead)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Yarn Spinner")
	void DismissLine();

	/**
	 * called when options should be dismissed
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Yarn Spinner")
	void DismissOptions();
};
