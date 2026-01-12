// copyright yarn spinner pty ltd
// licensed under the mit license

#include "DialogueRunner.h"
#include "YarnProgram.h"
#include "Presenters/DialoguePresenter.h"

UDialogueRunner::UDialogueRunner()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoStart = false;
	StartNode = TEXT("Start");
}

void UDialogueRunner::BeginPlay()
{
	Super::BeginPlay();

	// bind vm events to our handlers
	VirtualMachine.OnLine = [this](const FString& LineID, const TArray<FString>& Substitutions)
	{
		HandleLine(LineID, Substitutions);
	};

	VirtualMachine.OnCommand = [this](const FString& CommandText)
	{
		HandleCommand(CommandText);
	};

	VirtualMachine.OnOptions = [this](const TArray<FYarnOption>& Options)
	{
		HandleOptions(Options);
	};

	VirtualMachine.OnNodeStart = [this](const FString& NodeName)
	{
		HandleNodeStart(NodeName);
	};

	VirtualMachine.OnNodeComplete = [this](const FString& NodeName)
	{
		HandleNodeComplete(NodeName);
	};

	VirtualMachine.OnDialogueComplete = [this]()
	{
		HandleDialogueComplete();
	};

	// auto-start if configured
	if (bAutoStart && YarnProgram != nullptr)
	{
		StartDialogue(StartNode);
	}
}

void UDialogueRunner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// stop any running dialogue
	if (VirtualMachine.IsRunning())
	{
		VirtualMachine.Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void UDialogueRunner::StartDialogue(const FString& NodeName)
{
	if (YarnProgram == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("cannot start dialogue: no yarn program assigned"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("starting dialogue at node: %s"), *NodeName);

	// load program into vm
	VirtualMachine.LoadProgram(YarnProgram);

	// start execution
	VirtualMachine.SetStartNode(NodeName);
}

void UDialogueRunner::Continue()
{
	if (!VirtualMachine.IsRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("cannot continue: dialogue not running"));
		return;
	}

	VirtualMachine.Continue();
}

void UDialogueRunner::SelectOption(int32 OptionIndex)
{
	if (!VirtualMachine.IsRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("cannot select option: dialogue not running"));
		return;
	}

	VirtualMachine.SetSelectedOption(OptionIndex);
}

void UDialogueRunner::StopDialogue()
{
	VirtualMachine.Stop();
	UE_LOG(LogTemp, Log, TEXT("dialogue stopped"));
}

bool UDialogueRunner::IsDialogueRunning() const
{
	return VirtualMachine.IsRunning();
}

bool UDialogueRunner::GetBoolVariable(const FString& VariableName, bool DefaultValue)
{
	FYarnValue Value = VirtualMachine.GetVariable(VariableName);

	if (Value.Type == FYarnValue::EType::Bool)
	{
		return Value.BoolValue;
	}

	return DefaultValue;
}

void UDialogueRunner::SetBoolVariable(const FString& VariableName, bool Value)
{
	VirtualMachine.SetVariable(VariableName, FYarnValue(Value));
}

float UDialogueRunner::GetNumberVariable(const FString& VariableName, float DefaultValue)
{
	FYarnValue Value = VirtualMachine.GetVariable(VariableName);

	if (Value.Type == FYarnValue::EType::Float)
	{
		return Value.FloatValue;
	}

	return DefaultValue;
}

void UDialogueRunner::SetNumberVariable(const FString& VariableName, float Value)
{
	VirtualMachine.SetVariable(VariableName, FYarnValue(Value));
}

FString UDialogueRunner::GetStringVariable(const FString& VariableName, const FString& DefaultValue)
{
	FYarnValue Value = VirtualMachine.GetVariable(VariableName);

	if (Value.Type == FYarnValue::EType::String)
	{
		return Value.StringValue;
	}

	return DefaultValue;
}

void UDialogueRunner::SetStringVariable(const FString& VariableName, const FString& Value)
{
	VirtualMachine.SetVariable(VariableName, FYarnValue(Value));
}

FDialogueOption UDialogueRunner::ConvertOption(const FYarnOption& InternalOption) const
{
	FDialogueOption Result;
	Result.LineID = InternalOption.LineID;
	Result.Text = InternalOption.Text;
	Result.bIsAvailable = InternalOption.bIsAvailable;
	Result.Index = InternalOption.Index;
	return Result;
}

void UDialogueRunner::HandleLine(const FString& LineID, const TArray<FString>& Substitutions)
{
	// get the localized text
	FString LineText = YarnProgram ? YarnProgram->GetLineText(LineID) : LineID;

	// perform substitutions
	// replace {0}, {1}, {2}, etc. with the corresponding values from the substitutions array
	for (int32 i = 0; i < Substitutions.Num(); i++)
	{
		FString Placeholder = FString::Printf(TEXT("{%d}"), i);
		LineText = LineText.Replace(*Placeholder, *Substitutions[i]);
	}

	// if we have a presenter, use it; otherwise use blueprint events
	if (DialoguePresenter.GetInterface())
	{
		IDialoguePresenter::Execute_PresentLine(DialoguePresenter.GetObject(), LineID, LineText);
	}
	else
	{
		// call blueprint event
		OnLineReceived(LineID, LineText);
	}
}

void UDialogueRunner::HandleCommand(const FString& CommandText)
{
	UE_LOG(LogTemp, Log, TEXT("command: %s"), *CommandText);

	// call blueprint event
	OnCommandReceived(CommandText);
}

void UDialogueRunner::HandleOptions(const TArray<FYarnOption>& Options)
{
	// convert to blueprint-friendly format
	TArray<FDialogueOption> BlueprintOptions;
	BlueprintOptions.Reserve(Options.Num());

	for (const FYarnOption& Option : Options)
	{
		BlueprintOptions.Add(ConvertOption(Option));
	}

	// if we have a presenter, use it; otherwise use blueprint events
	if (DialoguePresenter.GetInterface())
	{
		IDialoguePresenter::Execute_PresentOptions(DialoguePresenter.GetObject(), BlueprintOptions);
	}
	else
	{
		// call blueprint event
		OnOptionsReceived(BlueprintOptions);
	}
}

void UDialogueRunner::HandleNodeStart(const FString& NodeName)
{
	UE_LOG(LogTemp, Log, TEXT("node started: %s"), *NodeName);

	// call blueprint event
	OnNodeStart(NodeName);
}

void UDialogueRunner::HandleNodeComplete(const FString& NodeName)
{
	UE_LOG(LogTemp, Log, TEXT("node completed: %s"), *NodeName);

	// call blueprint event
	OnNodeComplete(NodeName);
}

void UDialogueRunner::HandleDialogueComplete()
{
	UE_LOG(LogTemp, Log, TEXT("dialogue complete"));

	// call blueprint event
	OnDialogueComplete();
}
