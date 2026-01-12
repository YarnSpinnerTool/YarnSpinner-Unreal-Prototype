// copyright yarn spinner pty ltd
// licensed under the mit license

#include "DialogueWidget.h"
#include "DialogueRunner.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"

void UDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// bind continue button if it exists
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &UDialogueWidget::HandleContinueClicked);
	}
}

void UDialogueWidget::ShowLine(const FString& LineText)
{
	if (DialogueText)
	{
		DialogueText->SetText(FText::FromString(LineText));
	}

	// show continue button, hide options
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (OptionsContainer)
	{
		OptionsContainer->SetVisibility(ESlateVisibility::Collapsed);
	}

	// make widget visible
	SetVisibility(ESlateVisibility::Visible);
}

void UDialogueWidget::ShowOptions(const TArray<FDialogueOption>& Options)
{
	// hide continue button, show options container
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (OptionsContainer)
	{
		OptionsContainer->SetVisibility(ESlateVisibility::Visible);
	}

	// make widget visible
	SetVisibility(ESlateVisibility::Visible);

	// note: actual option button creation should be done in blueprint
	// override this function in blueprint to create option buttons dynamically
	// or use the OnOptionsReceived event in DialogueRunner directly
}

void UDialogueWidget::HideDialogue()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDialogueWidget::HandleContinueClicked()
{
	OnContinueClicked();
}
