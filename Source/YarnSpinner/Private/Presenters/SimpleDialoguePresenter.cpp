// copyright yarn spinner pty ltd
// licensed under the mit license

#include "Presenters/SimpleDialoguePresenter.h"
#include "DialogueRunner.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "TimerManager.h"

USimpleDialoguePresenter::USimpleDialoguePresenter()
{
	PrimaryComponentTick.bCanEverTick = false;

	bAutoAdvance = false;
	AutoAdvanceDelay = 2.0f;

	CurrentLineView = nullptr;
	CurrentOptionsView = nullptr;
	DialogueRunner = nullptr;
}

void USimpleDialoguePresenter::BeginPlay()
{
	Super::BeginPlay();

	// find the dialogue runner component on the same actor
	DialogueRunner = GetOwner()->FindComponentByClass<UDialogueRunner>();

	if (!DialogueRunner)
	{
		UE_LOG(LogTemp, Warning, TEXT("simple dialogue presenter: no dialogue runner found on actor"));
	}
}

void USimpleDialoguePresenter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupWidgets();

	// clear auto-advance timer
	if (AutoAdvanceTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoAdvanceTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void USimpleDialoguePresenter::PresentLine_Implementation(const FString& LineID, const FString& LineText)
{
	// cleanup any existing widgets first
	CleanupWidgets();

	// show the line
	ShowLineView(LineText);

	// set up auto-advance if enabled
	if (bAutoAdvance && AutoAdvanceDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutoAdvanceTimer,
			this,
			&USimpleDialoguePresenter::OnAutoAdvanceTimerElapsed,
			AutoAdvanceDelay,
			false
		);
	}
}

void USimpleDialoguePresenter::PresentOptions_Implementation(const TArray<FDialogueOption>& Options)
{
	// cleanup any existing widgets first
	CleanupWidgets();

	// show the options
	ShowOptionsView(Options);
}

void USimpleDialoguePresenter::DismissLine_Implementation()
{
	// remove line view
	if (CurrentLineView)
	{
		CurrentLineView->RemoveFromParent();
		CurrentLineView = nullptr;
	}

	// clear auto-advance timer
	if (AutoAdvanceTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoAdvanceTimer);
	}
}

void USimpleDialoguePresenter::DismissOptions_Implementation()
{
	// remove options view
	if (CurrentOptionsView)
	{
		CurrentOptionsView->RemoveFromParent();
		CurrentOptionsView = nullptr;
	}

	// clear option buttons
	for (UUserWidget* Button : CurrentOptionButtons)
	{
		if (Button)
		{
			Button->RemoveFromParent();
		}
	}
	CurrentOptionButtons.Empty();
}

void USimpleDialoguePresenter::ShowLineView(const FString& LineText)
{
	if (!LineViewClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("simple dialogue presenter: no line view class assigned"));
		return;
	}

	// create the widget
	CurrentLineView = CreateWidget<UUserWidget>(GetWorld(), LineViewClass);

	if (!CurrentLineView)
	{
		UE_LOG(LogTemp, Error, TEXT("failed to create line view widget"));
		return;
	}

	// find the text block and set the text
	UTextBlock* LineTextBlock = Cast<UTextBlock>(CurrentLineView->GetWidgetFromName(TEXT("LineText")));
	if (LineTextBlock)
	{
		LineTextBlock->SetText(FText::FromString(LineText));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("line view widget has no 'LineText' text block"));
	}

	// find the continue button and bind to it
	UButton* ContinueButton = Cast<UButton>(CurrentLineView->GetWidgetFromName(TEXT("ContinueButton")));
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &USimpleDialoguePresenter::OnContinueClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("line view widget has no 'ContinueButton' button"));
	}

	// add to viewport
	CurrentLineView->AddToViewport();
}

void USimpleDialoguePresenter::ShowOptionsView(const TArray<FDialogueOption>& Options)
{
	if (!OptionsViewClass || !OptionButtonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("simple dialogue presenter: no options view or button class assigned"));
		return;
	}

	// create the options container widget
	CurrentOptionsView = CreateWidget<UUserWidget>(GetWorld(), OptionsViewClass);

	if (!CurrentOptionsView)
	{
		UE_LOG(LogTemp, Error, TEXT("failed to create options view widget"));
		return;
	}

	// find the vertical box to add buttons to
	UVerticalBox* OptionsContainer = Cast<UVerticalBox>(CurrentOptionsView->GetWidgetFromName(TEXT("OptionsContainer")));

	if (!OptionsContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("options view widget has no 'OptionsContainer' vertical box"));
		CurrentOptionsView->AddToViewport();
		return;
	}

	// create a button for each option
	for (const FDialogueOption& Option : Options)
	{
		// create button widget
		UUserWidget* ButtonWidget = CreateWidget<UUserWidget>(GetWorld(), OptionButtonClass);

		if (!ButtonWidget)
		{
			continue;
		}

		// find text block and set option text
		UTextBlock* OptionTextBlock = Cast<UTextBlock>(ButtonWidget->GetWidgetFromName(TEXT("OptionText")));
		if (OptionTextBlock)
		{
			OptionTextBlock->SetText(FText::FromString(Option.Text));
		}

		// find button and bind click
		UButton* OptionButton = Cast<UButton>(ButtonWidget->GetWidgetFromName(TEXT("OptionButton")));
		if (OptionButton)
		{
			// capture option index for the lambda
			// note: we can't use AddDynamic with parameters, so we use a lambda
			int32 OptionIndex = Option.Index;
			OptionButton->OnClicked.AddUniqueDynamic(this, &USimpleDialoguePresenter::OnOptionButtonClicked);

			// store the index in the button's tag so we can retrieve it later
			ButtonWidget->SetRenderOpacity(1.0f); // just to have something to identify it

			// disable if not available
			OptionButton->SetIsEnabled(Option.bIsAvailable);
		}

		// add to container
		OptionsContainer->AddChild(ButtonWidget);
		CurrentOptionButtons.Add(ButtonWidget);
	}

	// add to viewport
	CurrentOptionsView->AddToViewport();
}

void USimpleDialoguePresenter::CleanupWidgets()
{
	DismissLine_Implementation();
	DismissOptions_Implementation();
}

void USimpleDialoguePresenter::OnContinueClicked()
{
	UE_LOG(LogTemp, Log, TEXT("continue clicked"));

	// dismiss the line view
	DismissLine_Implementation();

	// tell the dialogue runner to continue
	if (DialogueRunner)
	{
		DialogueRunner->Continue();
	}
}

void USimpleDialoguePresenter::OnOptionButtonClicked()
{
	// since all buttons call this same function, we default to first option
	// todo: properly track which button was clicked using button metadata
	OnOptionClicked(0);
}

void USimpleDialoguePresenter::OnOptionClicked(int32 OptionIndex)
{
	UE_LOG(LogTemp, Log, TEXT("option %d clicked"), OptionIndex);

	// dismiss the options view
	DismissOptions_Implementation();

	// tell the dialogue runner which option was selected
	if (DialogueRunner)
	{
		DialogueRunner->SelectOption(OptionIndex);
	}
}

void USimpleDialoguePresenter::OnAutoAdvanceTimerElapsed()
{
	UE_LOG(LogTemp, Log, TEXT("auto-advance timer elapsed"));

	// auto-advance the dialogue
	OnContinueClicked();
}
