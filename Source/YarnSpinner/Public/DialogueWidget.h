// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "DialogueWidget.generated.h"

/**
 * simple dialogue ui widget
 *
 * displays dialogue lines and handles continue/option selection
 * create a blueprint based on this class and add it to your viewport
 */
UCLASS()
class YARNSPINNER_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * widget components (bind these in the UMG designer)
	 */

	// text block for displaying dialogue lines
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* DialogueText;

	// container for option buttons
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* OptionsContainer;

	// continue button
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* ContinueButton;

	/**
	 * blueprint-callable functions
	 */

	// display a line of dialogue
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void ShowLine(const FString& LineText);

	// display options
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void ShowOptions(const TArray<struct FDialogueOption>& Options);

	// hide the widget
	UFUNCTION(BlueprintCallable, Category = "Yarn Spinner")
	void HideDialogue();

	/**
	 * events for blueprint to handle
	 */

	// called when continue button is clicked
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner")
	void OnContinueClicked();

	// called when an option is selected
	UFUNCTION(BlueprintImplementableEvent, Category = "Yarn Spinner")
	void OnOptionSelected(int32 OptionIndex);

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleContinueClicked();
};
