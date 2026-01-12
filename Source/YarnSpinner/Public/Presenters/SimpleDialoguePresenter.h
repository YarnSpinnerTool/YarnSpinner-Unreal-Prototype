// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialoguePresenter.h"
#include "SimpleDialoguePresenter.generated.h"

/**
 * simple umg-based dialogue presenter
 *
 * this is a basic implementation that you can use as-is or as a reference
 * for building your own custom presenters
 *
 * usage:
 * 1. add this component to the same actor as your dialogue runner
 * 2. assign umg widget classes for line view and options view
 * 3. the presenter will automatically create and manage the widgets
 *
 * alternatively, create your own blueprint that implements idialogguepresenter
 */
UCLASS(ClassGroup=(YarnSpinner), meta=(BlueprintSpawnableComponent))
class YARNSPINNER_API USimpleDialoguePresenter : public UActorComponent, public IDialoguePresenter
{
	GENERATED_BODY()

public:
	USimpleDialoguePresenter();

	/**
	 * configuration
	 */

	// the widget class to use for displaying lines
	// should have a text block named "LineText" and a button named "ContinueButton"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner")
	TSubclassOf<class UUserWidget> LineViewClass;

	// the widget class to use for displaying options
	// should have a vertical box named "OptionsContainer"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner")
	TSubclassOf<class UUserWidget> OptionsViewClass;

	// the widget class to use for individual option buttons
	// should have a text block named "OptionText" and expose a button named "OptionButton"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner")
	TSubclassOf<class UUserWidget> OptionButtonClass;

	// whether to auto-advance lines after a delay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner")
	bool bAutoAdvance;

	// delay before auto-advancing (if auto-advance is enabled)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yarn Spinner", meta = (EditCondition = "bAutoAdvance"))
	float AutoAdvanceDelay;

	/**
	 * idialogguepresenter interface
	 */

	virtual void PresentLine_Implementation(const FString& LineID, const FString& LineText) override;
	virtual void PresentOptions_Implementation(const TArray<FDialogueOption>& Options) override;
	virtual void DismissLine_Implementation() override;
	virtual void DismissOptions_Implementation() override;

	/**
	 * ue component lifecycle
	 */

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// currently displayed widgets
	UPROPERTY()
	UUserWidget* CurrentLineView;

	UPROPERTY()
	UUserWidget* CurrentOptionsView;

	UPROPERTY()
	TArray<UUserWidget*> CurrentOptionButtons;

	// reference to the dialogue runner
	UPROPERTY()
	class UDialogueRunner* DialogueRunner;

	// timer for auto-advance
	FTimerHandle AutoAdvanceTimer;

	// create and show line view widget
	void ShowLineView(const FString& LineText);

	// create and show options view widget
	void ShowOptionsView(const TArray<FDialogueOption>& Options);

	// cleanup current widgets
	void CleanupWidgets();

	// callbacks
	UFUNCTION()
	void OnContinueClicked();

	// called when any option button is clicked
	UFUNCTION()
	void OnOptionButtonClicked();

	void OnOptionClicked(int32 OptionIndex);

	UFUNCTION()
	void OnAutoAdvanceTimerElapsed();
};
