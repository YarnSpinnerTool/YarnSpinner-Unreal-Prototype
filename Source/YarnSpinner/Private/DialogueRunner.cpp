// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueRunner.h"
#include "Line.h"
#include "Option.h"
#include "YarnSubsystem.h"
#include "YarnSpinner.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "Misc/YSLogging.h"

THIRD_PARTY_INCLUDES_START
#include "YarnSpinnerCore/VirtualMachine.h"
THIRD_PARTY_INCLUDES_END
//#include "StaticParty.h"


// Sets default values
ADialogueRunner::ADialogueRunner()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
}


// Called when the game starts or when spawned
void ADialogueRunner::PreInitializeComponents()
{
    Super::PreInitializeComponents();

    if (!YarnProject)
    {
        UE_LOG(LogYarnSpinner, Error, TEXT("DialogueRunner can't initialize, because it doesn't have a Yarn Asset."));
        return;
    }

    YarnProject->Init();

    Yarn::Program Program{};

    bool bParseSuccess = Program.ParsePartialFromArray(YarnProject->Data.GetData(), YarnProject->Data.Num());

    if (!bParseSuccess)
    {
        UE_LOG(LogYarnSpinner, Error, TEXT("DialogueRunner can't initialize, because its Yarn Asset failed to load."));
        return;
    }

    // Create the Library
    Library = TUniquePtr<Yarn::Library>(new Yarn::Library(*this));

    UYarnSubsystem* SS = YarnSubsystem();


    // Create the VirtualMachine, supplying it with the loaded Program and
    // configuring it to use our library, plus use this ADialogueRunner as the
    // logger and the variable storage
    // VirtualMachine = TUniquePtr<Yarn::VirtualMachine>(new Yarn::VirtualMachine(Program, *(Library), *this, *this));
    VirtualMachine = TUniquePtr<Yarn::VirtualMachine>(new Yarn::VirtualMachine(Program, *this, *this));

    VirtualMachine->LineHandler = [this](Yarn::Line& Line)
    {
        UE_LOG(LogYarnSpinner, Log, TEXT("Received line %s"), UTF8_TO_TCHAR(Line.LineID.c_str()));

        // Get the Yarn line struct, and make a ULine out of it to use
        ULine* LineObject = NewObject<ULine>(this);
        LineObject->LineID = FName(Line.LineID.c_str());

        GetDisplayTextForLine(LineObject, Line);

        const TArray<TSoftObjectPtr<UObject>> LineAssets = YarnProject->GetLineAssets(LineObject->LineID);
        YS_LOG_FUNC("Got %d line assets for line '%s'", LineAssets.Num(), *LineObject->LineID.ToString())

        OnRunLine(LineObject, LineAssets);
    };

    VirtualMachine->OptionsHandler = [this](Yarn::OptionSet& OptionSet)
    {
        UE_LOG(LogYarnSpinner, Log, TEXT("Received %i options"), OptionSet.Options.size());

        // Build a TArray for every option in this OptionSet
        TArray<UOption*> Options;

        for (auto Option : OptionSet.Options)
        {
            UE_LOG(LogYarnSpinner, Log, TEXT("- %i: %s"), Option.ID, UTF8_TO_TCHAR(Option.Line.LineID.c_str()));

            UOption* Opt = NewObject<UOption>(this);
            Opt->OptionID = Option.ID;

            Opt->Line = NewObject<ULine>(Opt);
            Opt->Line->LineID = FName(Option.Line.LineID.c_str());

            GetDisplayTextForLine(Opt->Line, Option.Line);

            Opt->bIsAvailable = Option.IsAvailable;

            Opt->SourceDialogueRunner = this;

            Options.Add(Opt);
        }

        OnRunOptions(Options);
    };

    VirtualMachine->DoesFunctionExist = [this](const std::string& FunctionName) -> bool
    {
        return YarnSubsystem()->GetYarnLibraryRegistry()->HasFunction(FName(UTF8_TO_TCHAR(FunctionName.c_str())));
    };

    VirtualMachine->GetExpectedFunctionParamCount = [this](const std::string& FunctionName) -> int
    {
        return YarnSubsystem()->GetYarnLibraryRegistry()->GetExpectedFunctionParamCount(FName(UTF8_TO_TCHAR(FunctionName.c_str())));
    };

    VirtualMachine->CallFunction = [this](const std::string& FunctionName, const std::vector<Yarn::Value>& Parameters) -> Yarn::Value
    {
        return YarnSubsystem()->GetYarnLibraryRegistry()->CallFunction(
            FName(UTF8_TO_TCHAR(FunctionName.c_str())),
            TArray<Yarn::Value>(Parameters.data(), Parameters.size())
        );
    };

    VirtualMachine->CommandHandler = [this](Yarn::Command& Command)
    {
        UE_LOG(LogYarnSpinner, Log, TEXT("Received command \"%s\""), UTF8_TO_TCHAR(Command.Text.c_str()));

        FString CommandText = FString(UTF8_TO_TCHAR(Command.Text.c_str()));

        TArray<FString> CommandElements;
        CommandText.ParseIntoArray(CommandElements, TEXT(" "));

        if (CommandElements.Num() == 0)
        {
            TArray<FString> EmptyParameters;
            UE_LOG(LogYarnSpinner, Error, TEXT("Command received, but was unable to parse it."));
            OnRunCommand(FString("(unknown)"), EmptyParameters);
            return;
        }

        FName CommandName = FName(CommandElements[0]);
        CommandElements.RemoveAt(0);

        auto Lib = YarnSubsystem()->GetYarnLibraryRegistry();

        if (Lib->HasCommand(CommandName))
        {
            return Lib->CallCommand(
                CommandName,
                this,
                CommandElements
            );
        }

        // Haven't handled the function yet, so call the DialogueRunner's handler
        OnRunCommand(CommandName.ToString(), CommandElements);
    };

    VirtualMachine->NodeStartHandler = [this](std::string NodeName)
    {
        UE_LOG(LogYarnSpinner, Log, TEXT("Received node start \"%s\""), UTF8_TO_TCHAR(NodeName.c_str()));
    };

    VirtualMachine->NodeCompleteHandler = [this](std::string NodeName)
    {
        UE_LOG(LogYarnSpinner, Log, TEXT("Received node complete \"%s\""), UTF8_TO_TCHAR(NodeName.c_str()));
    };

    VirtualMachine->DialogueCompleteHandler = [this]()
    {
        UE_LOG(LogYarnSpinner, Log, TEXT("Received dialogue complete"));
        OnDialogueEnded();
    };
}


// Called every frame
void ADialogueRunner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}


void ADialogueRunner::OnDialogueStarted_Implementation()
{
    // default = no-op
}


void ADialogueRunner::OnDialogueEnded_Implementation()
{
    // default = no-op
}


void ADialogueRunner::OnRunLine_Implementation(ULine* Line, const TArray<TSoftObjectPtr<UObject>>& LineAssets)
{
    // default = log and immediately continue
    UE_LOG(LogYarnSpinner, Warning, TEXT("DialogueRunner received line with ID \"%s\". Implement OnRunLine to customise its behaviour."), *Line->LineID.ToString());
    ContinueDialogue();
}


void ADialogueRunner::OnRunOptions_Implementation(const TArray<class UOption*>& Options)
{
    // default = log and choose the first option
    UE_LOG(LogYarnSpinner, Warning, TEXT("DialogueRunner received %i options. Choosing the first one by default. Implement OnRunOptions to customise its behaviour."), Options.Num());

    SelectOption(Options[0]);
}


void ADialogueRunner::OnRunCommand_Implementation(const FString& Command, const TArray<FString>& Parameters)
{
    // default = no-op
    UE_LOG(LogYarnSpinner, Warning, TEXT("DialogueRunner received command \"%s\". Implement OnRunCommand to customise its behaviour."), *Command);
    ContinueDialogue();
}


/** Starts running dialogue from the given node name. */
void ADialogueRunner::StartDialogue(FName NodeName)
{
    if (VirtualMachine.IsValid() == false)
    {
        UE_LOG(LogYarnSpinner, Error, TEXT("DialogueRunner can't start node %s, because it failed to load a Yarn asset."), *NodeName.ToString());
        return;
    }

    bool bNodeSelected = VirtualMachine->SetNode(TCHAR_TO_UTF8(*NodeName.ToString()));

    if (bNodeSelected)
    {
        OnDialogueStarted();
        ContinueDialogue();
    }
    else
    {
        UE_LOG(LogYarnSpinner, Error, TEXT("DialogueRunner can't start node %s, because a node with that name was not found."), *NodeName.ToString());
        return;
    }
}


/** Continues running the current dialogue, producing either lines, options, commands, or a dialogue-end signal. */
void ADialogueRunner::ContinueDialogue()
{
    YS_LOG_FUNCSIG

    if (VirtualMachine->GetCurrentExecutionState() == Yarn::VirtualMachine::ExecutionState::ERROR)
    {
        UE_LOG(LogYarnSpinner, Error, TEXT("VirtualMachine is in an error state and cannot continue running."));
        return;
    }

    VirtualMachine->Continue();

    Yarn::VirtualMachine::ExecutionState State = VirtualMachine->GetCurrentExecutionState();

    if (State == Yarn::VirtualMachine::ExecutionState::ERROR)
    {
        UE_LOG(LogYarnSpinner, Error, TEXT("VirtualMachine encountered an error."));
        return;
    }
}


/** Indicates to the dialogue runner that an option was selected. */
void ADialogueRunner::SelectOption(UOption* Option)
{
    Yarn::VirtualMachine::ExecutionState State = this->VirtualMachine->GetCurrentExecutionState();

    if (State != Yarn::VirtualMachine::ExecutionState::WAITING_ON_OPTION_SELECTION)
    {
        UE_LOG(LogYarnSpinner, Error, TEXT("Dialogue Runner received a call to SelectOption but it wasn't expecting a selection!"));
        return;
    }

    UE_LOG(LogYarnSpinner, Log, TEXT("Selected option %i (%s)"), Option->OptionID, *Option->Line->LineID.ToString());

    VirtualMachine->SetSelectedOption(Option->OptionID);

    if (bRunSelectedOptionsAsLines)
    {
        const TArray<TSoftObjectPtr<UObject>> LineAssets = YarnProject->GetLineAssets(Option->Line->LineID);
        YS_LOG_FUNC("Got %d line assets for line '%s'", LineAssets.Num(), *Option->Line->LineID.ToString())

        OnRunLine(Option->Line, LineAssets);
    }
    else
    {
        ContinueDialogue();
    }
}


void ADialogueRunner::Log(std::string Message, Type Severity)
{
    FString MessageText = FString(UTF8_TO_TCHAR(Message.c_str()));

    switch (Severity)
    {
    case Type::INFO:
        YS_LOG("YarnSpinner: %s", *MessageText);
        break;
    case Type::WARNING:
        YS_WARN("YarnSpinner: %s", *MessageText);
        break;
    case Type::ERROR:
        YS_ERR("YarnSpinner: %s", *MessageText);
        break;
    }
}


void ADialogueRunner::SetValue(std::string Name, bool bValue)
{
    YS_LOG("Setting variable %s to bool %i", UTF8_TO_TCHAR(Name.c_str()), bValue)
    YarnSubsystem()->SetValue(Name, bValue);
}


void ADialogueRunner::SetValue(std::string Name, float Value)
{
    YS_LOG("Setting variable %s to float %f", UTF8_TO_TCHAR(Name.c_str()), Value)
    YarnSubsystem()->SetValue(Name, Value);
}


void ADialogueRunner::SetValue(std::string Name, std::string Value)
{
    YS_LOG("Setting variable %s to string %s", UTF8_TO_TCHAR(Name.c_str()), UTF8_TO_TCHAR(Value.c_str()))
    YarnSubsystem()->SetValue(Name, Value);
}


bool ADialogueRunner::HasValue(std::string Name)
{
    return YarnSubsystem()->HasValue(Name);
}


Yarn::Value ADialogueRunner::GetValue(std::string Name)
{
    Yarn::Value Value = YarnSubsystem()->GetValue(Name);
    YS_LOG("Retrieving variable %s with value %s", UTF8_TO_TCHAR(Name.c_str()), UTF8_TO_TCHAR(Value.ConvertToString().c_str()))
    return Value;
}


void ADialogueRunner::ClearValue(std::string Name)
{
    YS_LOG("Clearing variable %s", UTF8_TO_TCHAR(Name.c_str()))
    YarnSubsystem()->ClearValue(Name);
}


UYarnSubsystem* ADialogueRunner::YarnSubsystem() const
{
    if (!GetGameInstance())
    {
        YS_WARN("Could not retrieve YarnSubsystem because GetGameInstance() returned null")
        return nullptr;
    }
    return GetGameInstance()->GetSubsystem<UYarnSubsystem>();
}


void ADialogueRunner::GetDisplayTextForLine(ULine* Line, const Yarn::Line& YarnLine)
{
    const FName LineID = FName(YarnLine.LineID.c_str());

    // This assumes that we only ever care about lines that actually exist in .yarn files (rather than allowing extra lines in .csv files)
    if (!YarnProject || !YarnProject->Lines.Contains(LineID))
    {
        Line->DisplayText = FText::FromString(TEXT("(missing line!)"));
        return;
    }

    const FText LocalisedDisplayText = FText::FromString(YarnProject->Lines[LineID]);

    const FText NonLocalisedDisplayText = FText::FromString(YarnProject->Lines[LineID]);

    // Apply substitutions
    FFormatOrderedArguments FormatArgs;
    for (auto Substitution : YarnLine.Substitutions)
    {
        FormatArgs.Emplace(FText::FromString(UTF8_TO_TCHAR(Substitution.c_str())));
    }

    const FText TextWithSubstitutions = (LocalisedDisplayText.IsEmptyOrWhitespace()) ? FText::Format(NonLocalisedDisplayText, FormatArgs) : FText::Format(LocalisedDisplayText, FormatArgs);

    // TODO: add support for markup & context (speaker, target)

    YS_LOG_FUNC("Setting line %s to display text '%s'", *LineID.ToString(), *TextWithSubstitutions.ToString())

    Line->DisplayText = TextWithSubstitutions;
}
