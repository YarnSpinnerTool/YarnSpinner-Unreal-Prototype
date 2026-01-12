// copyright yarn spinner pty ltd
// licensed under the mit license

#include "YarnVirtualMachine.h"
#include "YarnProgram.h"
#include "yarn_spinner.pb.h"

FYarnVirtualMachine::FYarnVirtualMachine()
	: CurrentProgram(nullptr)
	, CurrentNode(nullptr)
	, ProgramCounter(0)
	, SelectedOptionIndex(-1)
	, ExecutionState(EExecutionState::Stopped)
	, VariableStorage(nullptr)
{
	RegisterBuiltInFunctions();
}

FYarnVirtualMachine::~FYarnVirtualMachine()
{
}

void FYarnVirtualMachine::LoadProgram(UYarnProgram* InProgram)
{
	CurrentProgram = InProgram;
	ExecutionState = EExecutionState::Stopped;

	UE_LOG(LogTemp, Log, TEXT("loaded yarn program into vm"));
}

void FYarnVirtualMachine::SetStartNode(const FString& NodeName)
{
	if (!CurrentProgram)
	{
		UE_LOG(LogTemp, Error, TEXT("cannot start node: no program loaded"));
		return;
	}

	// find the starting node
	CurrentNode = FindNode(NodeName);

	if (!CurrentNode)
	{
		UE_LOG(LogTemp, Error, TEXT("start node not found: %s"), *NodeName);
		return;
	}

	// reset vm state
	ProgramCounter = 0;
	Stack.Empty();
	CallStack.Empty();
	PendingOptions.Empty();
	SelectedOptionIndex = -1;
	ExecutionState = EExecutionState::Running;

	UE_LOG(LogTemp, Log, TEXT("starting dialogue at node: %s"), *NodeName);

	// track node visit
	int32& VisitCount = VisitedNodes.FindOrAdd(NodeName, 0);
	VisitCount++;

	// notify listeners
	if (OnNodeStart)
	{
		OnNodeStart(NodeName);
	}

	// begin execution
	Continue();
}

void FYarnVirtualMachine::Continue()
{
	if (ExecutionState == EExecutionState::Stopped)
	{
		UE_LOG(LogTemp, Warning, TEXT("cannot continue: vm is stopped"));
		return;
	}

	// resume from waiting state
	if (ExecutionState == EExecutionState::WaitingForContinue)
	{
		ExecutionState = EExecutionState::Running;
	}

	// execute instructions until we hit a yield point
	while (ExecutionState == EExecutionState::Running)
	{
		// check if we've reached the end of the current node
		if (!CurrentNode || ProgramCounter >= CurrentNode->Instructions.Num())
		{
			// node completed
			const FString& CompletedNodeName = CurrentNode ? CurrentNode->Name : TEXT("unknown");

			if (OnNodeComplete)
			{
				OnNodeComplete(CompletedNodeName);
			}

			// check if we have a return address on the call stack
			if (CallStack.Num() > 0)
			{
				// return from detour
				FCallFrame Frame = CallStack.Pop();
				CurrentNode = FindNode(Frame.NodeName);
				ProgramCounter = Frame.ReturnAddress;

				UE_LOG(LogTemp, Log, TEXT("returning from detour to %s at pc %d"), *Frame.NodeName, Frame.ReturnAddress);
				continue;
			}
			else
			{
				// dialogue complete
				ExecutionState = EExecutionState::Stopped;

				if (OnDialogueComplete)
				{
					OnDialogueComplete();
				}

				UE_LOG(LogTemp, Log, TEXT("dialogue complete"));
				return;
			}
		}

		// execute next instruction
		const FYarnInstruction& Inst = CurrentNode->Instructions[ProgramCounter];
		ProgramCounter++;

		ExecuteInstruction(Inst);

		// check if we hit a yield point
		if (ExecutionState != EExecutionState::Running)
		{
			return;
		}
	}
}

void FYarnVirtualMachine::SetSelectedOption(int32 OptionIndex)
{
	if (ExecutionState != EExecutionState::WaitingForOption)
	{
		UE_LOG(LogTemp, Warning, TEXT("cannot select option: not waiting for option"));
		return;
	}

	if (OptionIndex < 0 || OptionIndex >= PendingOptions.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("invalid option index: %d (have %d options)"), OptionIndex, PendingOptions.Num());
		return;
	}

	SelectedOptionIndex = OptionIndex;

	// jump to the selected option's destination
	const FYarnOption& Selected = PendingOptions[OptionIndex];
	ProgramCounter = Selected.Destination;

	// clear pending options
	PendingOptions.Empty();

	// resume execution
	ExecutionState = EExecutionState::Running;
	Continue();
}

void FYarnVirtualMachine::Stop()
{
	ExecutionState = EExecutionState::Stopped;
	Stack.Empty();
	CallStack.Empty();
	PendingOptions.Empty();
	CurrentNode = nullptr;
	ProgramCounter = 0;

	UE_LOG(LogTemp, Log, TEXT("vm stopped"));
}

void FYarnVirtualMachine::SetVariableStorage(IYarnVariableStorage* InStorage)
{
	VariableStorage = InStorage;
}

FYarnValue FYarnVirtualMachine::GetVariable(const FString& VarName) const
{
	// check external storage first
	if (VariableStorage && VariableStorage->Contains(VarName))
	{
		return VariableStorage->GetValue(VarName);
	}

	// check internal storage
	const FYarnValue* Found = InternalVariables.Find(VarName);
	if (Found)
	{
		return *Found;
	}

	// check program initial values
	if (CurrentProgram)
	{
		FYarnValue InitialValue;
		if (CurrentProgram->GetInitialValue(VarName, InitialValue))
		{
			return InitialValue;
		}
	}

	// default to false if not found
	return FYarnValue(false);
}

void FYarnVirtualMachine::SetVariable(const FString& VarName, const FYarnValue& Value)
{
	// store in external storage if available
	if (VariableStorage)
	{
		VariableStorage->SetValue(VarName, Value);
	}
	else
	{
		// store internally
		InternalVariables.Add(VarName, Value);
	}
}

void FYarnVirtualMachine::RegisterFunction(const FString& FunctionName, FYarnFunction Function, int32 ParameterCount)
{
	FFunctionInfo Info;
	Info.Function = Function;
	Info.ParameterCount = ParameterCount;
	FunctionLibrary.Add(FunctionName, Info);
	UE_LOG(LogTemp, Log, TEXT("registered yarn function: %s (params: %d)"), *FunctionName, ParameterCount);
}

void FYarnVirtualMachine::UnregisterFunction(const FString& FunctionName)
{
	FunctionLibrary.Remove(FunctionName);
}

const FYarnNode* FYarnVirtualMachine::FindNode(const FString& NodeName) const
{
	if (!CurrentProgram)
	{
		return nullptr;
	}

	// use the yarn program's cached node lookup (ue-friendly structure)
	return CurrentProgram->GetNode(NodeName);
}

void FYarnVirtualMachine::ExecuteInstruction(const FYarnInstruction& Inst)
{
	// dispatch based on opcode (ue-friendly enum)
	switch (Inst.OpCode)
	{
		case FYarnInstruction::EOpCode::JumpTo:
			Execute_JumpTo(Inst);
			break;

		case FYarnInstruction::EOpCode::PeekAndJump:
			Execute_PeekAndJump(Inst);
			break;

		case FYarnInstruction::EOpCode::RunLine:
			Execute_RunLine(Inst);
			break;

		case FYarnInstruction::EOpCode::RunCommand:
			Execute_RunCommand(Inst);
			break;

		case FYarnInstruction::EOpCode::AddOption:
			Execute_AddOption(Inst);
			break;

		case FYarnInstruction::EOpCode::ShowOptions:
			Execute_ShowOptions(Inst);
			break;

		case FYarnInstruction::EOpCode::PushString:
			Execute_PushString(Inst);
			break;

		case FYarnInstruction::EOpCode::PushFloat:
			Execute_PushFloat(Inst);
			break;

		case FYarnInstruction::EOpCode::PushBool:
			Execute_PushBool(Inst);
			break;

		case FYarnInstruction::EOpCode::JumpIfFalse:
			Execute_JumpIfFalse(Inst);
			break;

		case FYarnInstruction::EOpCode::Pop:
			Execute_Pop(Inst);
			break;

		case FYarnInstruction::EOpCode::CallFunc:
			Execute_CallFunc(Inst);
			break;

		case FYarnInstruction::EOpCode::PushVariable:
			Execute_PushVariable(Inst);
			break;

		case FYarnInstruction::EOpCode::StoreVariable:
			Execute_StoreVariable(Inst);
			break;

		case FYarnInstruction::EOpCode::Stop:
			Execute_Stop(Inst);
			break;

		case FYarnInstruction::EOpCode::RunNode:
			Execute_RunNode(Inst);
			break;

		case FYarnInstruction::EOpCode::PeekAndRunNode:
			Execute_PeekAndRunNode(Inst);
			break;

		case FYarnInstruction::EOpCode::DetourToNode:
			Execute_DetourToNode(Inst);
			break;

		case FYarnInstruction::EOpCode::PeekAndDetourToNode:
			Execute_PeekAndDetourToNode(Inst);
			break;

		case FYarnInstruction::EOpCode::Return:
			Execute_Return(Inst);
			break;

		case FYarnInstruction::EOpCode::AddSaliencyCandidate:
			Execute_AddSaliencyCandidate(Inst);
			break;

		case FYarnInstruction::EOpCode::AddSaliencyCandidateFromNode:
			Execute_AddSaliencyCandidateFromNode(Inst);
			break;

		case FYarnInstruction::EOpCode::SelectSaliencyCandidate:
			Execute_SelectSaliencyCandidate(Inst);
			break;

		default:
			UE_LOG(LogTemp, Error, TEXT("unknown instruction opcode"));
			ExecutionState = EExecutionState::Stopped;
			break;
	}
}

// instruction handlers

void FYarnVirtualMachine::Execute_JumpTo(const FYarnInstruction& Inst)
{
	ProgramCounter = Inst.IntOperand;
}

void FYarnVirtualMachine::Execute_PeekAndJump(const FYarnInstruction& Inst)
{
	// peek string from stack and jump to that named position
	if (Stack.Num() > 0)
	{
		FString LabelName = Stack.Last().ToString();
		// todo: implement label lookup
		UE_LOG(LogTemp, Warning, TEXT("peekand jump to label not yet implemented: %s"), *LabelName);
	}
}

void FYarnVirtualMachine::Execute_RunLine(const FYarnInstruction& Inst)
{
	// get line id from instruction (already decoded as fstring)
	const FString& LineID = Inst.StringOperand;

	UE_LOG(LogTemp, Log, TEXT("RunLine: LineID='%s', SubstitutionCount=%d"), *LineID, Inst.SubstitutionCount);

	// pop substitutions from stack (in reverse order)
	TArray<FString> Substitutions;
	for (int32 i = 0; i < Inst.SubstitutionCount; i++)
	{
		if (Stack.Num() > 0)
		{
			FYarnValue Val = Stack.Pop();
			Substitutions.Insert(Val.ToString(), 0); // insert at beginning to reverse order
		}
	}

	// get localized text
	FString LineText = CurrentProgram ? CurrentProgram->GetLineText(LineID) : LineID;

	UE_LOG(LogTemp, Log, TEXT("line: %s"), *LineText);

	// notify listener
	if (OnLine)
	{
		OnLine(LineID, Substitutions);
	}

	// wait for continue
	ExecutionState = EExecutionState::WaitingForContinue;
}

void FYarnVirtualMachine::Execute_RunCommand(const FYarnInstruction& Inst)
{
	// get command text
	FString CommandText = Inst.StringOperand;

	// pop substitutions from stack
	TArray<FString> Substitutions;
	for (int32 i = 0; i < Inst.SubstitutionCount; i++)
	{
		if (Stack.Num() > 0)
		{
			FYarnValue Val = Stack.Pop();
			Substitutions.Insert(Val.ToString(), 0);
		}
	}

	// perform substitutions in command text
	// replace {0}, {1}, {2}, etc. with the corresponding values
	for (int32 i = 0; i < Substitutions.Num(); i++)
	{
		FString Placeholder = FString::Printf(TEXT("{%d}"), i);
		CommandText = CommandText.Replace(*Placeholder, *Substitutions[i]);
	}

	UE_LOG(LogTemp, Log, TEXT("command: %s"), *CommandText);

	// notify listener
	if (OnCommand)
	{
		OnCommand(CommandText);
	}
}

void FYarnVirtualMachine::Execute_AddOption(const FYarnInstruction& Inst)
{
	FYarnOption Option;
	Option.LineID = Inst.StringOperand;
	Option.Destination = Inst.IntOperand;
	Option.Index = PendingOptions.Num();

	// get localized text
	Option.Text = CurrentProgram ? CurrentProgram->GetLineText(Option.LineID) : Option.LineID;

	// check if option has a condition
	if (Inst.HasCondition)
	{
		// pop condition from stack
		if (Stack.Num() > 0)
		{
			FYarnValue Condition = Stack.Pop();
			Option.bIsAvailable = Condition.ToBool();
		}
		else
		{
			Option.bIsAvailable = false;
		}
	}
	else
	{
		Option.bIsAvailable = true;
	}

	// pop substitutions from stack
	TArray<FString> Substitutions;
	for (int32 i = 0; i < Inst.SubstitutionCount; i++)
	{
		if (Stack.Num() > 0)
		{
			FYarnValue Val = Stack.Pop();
			Substitutions.Insert(Val.ToString(), 0);
		}
	}

	// perform substitutions in option text
	// replace {0}, {1}, {2}, etc. with the corresponding values
	for (int32 i = 0; i < Substitutions.Num(); i++)
	{
		FString Placeholder = FString::Printf(TEXT("{%d}"), i);
		Option.Text = Option.Text.Replace(*Placeholder, *Substitutions[i]);
	}

	PendingOptions.Add(Option);
}

void FYarnVirtualMachine::Execute_ShowOptions(const FYarnInstruction& Inst)
{
	if (PendingOptions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("showoptions called with no pending options"));
		return;
	}

	// notify listener
	if (OnOptions)
	{
		OnOptions(PendingOptions);
	}

	// wait for option selection
	ExecutionState = EExecutionState::WaitingForOption;
}

void FYarnVirtualMachine::Execute_PushString(const FYarnInstruction& Inst)
{
	Stack.Push(FYarnValue(Inst.StringOperand));
}

void FYarnVirtualMachine::Execute_PushFloat(const FYarnInstruction& Inst)
{
	Stack.Push(FYarnValue(Inst.FloatOperand));
}

void FYarnVirtualMachine::Execute_PushBool(const FYarnInstruction& Inst)
{
	Stack.Push(FYarnValue(Inst.BoolOperand));
}

void FYarnVirtualMachine::Execute_JumpIfFalse(const FYarnInstruction& Inst)
{
	if (Stack.Num() > 0)
	{
		FYarnValue Condition = Stack.Pop();
		if (!Condition.ToBool())
		{
			ProgramCounter = Inst.IntOperand;
		}
	}
}

void FYarnVirtualMachine::Execute_Pop(const FYarnInstruction& Inst)
{
	if (Stack.Num() > 0)
	{
		Stack.Pop();
	}
}

void FYarnVirtualMachine::Execute_CallFunc(const FYarnInstruction& Inst)
{
	// function name is in the instruction
	FString FunctionName = Inst.StringOperand;

	// look up function to get parameter count
	FFunctionInfo* FuncInfo = FunctionLibrary.Find(FunctionName);
	int32 ParamCount = 0;

	if (FuncInfo)
	{
		ParamCount = FuncInfo->ParameterCount;
		// -1 means variable argument count - for now, default to 0
		// in the future, this could be determined by stack markers or other mechanisms
		if (ParamCount < 0)
		{
			ParamCount = 0;
		}
	}

	// call the function with the determined parameter count
	FYarnValue Result = CallFunction(FunctionName, ParamCount);

	// push result onto stack
	Stack.Push(Result);
}

void FYarnVirtualMachine::Execute_PushVariable(const FYarnInstruction& Inst)
{
	FYarnValue Value = GetVariable(Inst.StringOperand);
	Stack.Push(Value);
}

void FYarnVirtualMachine::Execute_StoreVariable(const FYarnInstruction& Inst)
{
	if (Stack.Num() > 0)
	{
		FYarnValue Value = Stack.Last(); // peek, don't pop
		SetVariable(Inst.StringOperand, Value);
	}
}

void FYarnVirtualMachine::Execute_Stop(const FYarnInstruction& Inst)
{
	ExecutionState = EExecutionState::Stopped;
}

void FYarnVirtualMachine::Execute_RunNode(const FYarnInstruction& Inst)
{
	SetStartNode(Inst.StringOperand);
}

void FYarnVirtualMachine::Execute_PeekAndRunNode(const FYarnInstruction& Inst)
{
	if (Stack.Num() > 0)
	{
		FString NodeName = Stack.Pop().ToString();
		SetStartNode(NodeName);
	}
}

void FYarnVirtualMachine::Execute_DetourToNode(const FYarnInstruction& Inst)
{
	FString NodeName = Inst.StringOperand;

	// push current position onto call stack
	FCallFrame Frame;
	Frame.NodeName = CurrentNode->Name;
	Frame.ReturnAddress = ProgramCounter;
	CallStack.Push(Frame);

	// jump to detour node
	CurrentNode = FindNode(NodeName);
	ProgramCounter = 0;

	if (CurrentNode)
	{
		// track node visit
		int32& VisitCount = VisitedNodes.FindOrAdd(NodeName, 0);
		VisitCount++;

		// notify listeners
		if (OnNodeStart)
		{
			OnNodeStart(NodeName);
		}
	}
}

void FYarnVirtualMachine::Execute_PeekAndDetourToNode(const FYarnInstruction& Inst)
{
	if (Stack.Num() > 0)
	{
		FString NodeName = Stack.Pop().ToString();

		FCallFrame Frame;
		Frame.NodeName = CurrentNode->Name;
		Frame.ReturnAddress = ProgramCounter;
		CallStack.Push(Frame);

		CurrentNode = FindNode(NodeName);
		ProgramCounter = 0;

		if (CurrentNode)
		{
			// track node visit
			int32& VisitCount = VisitedNodes.FindOrAdd(NodeName, 0);
			VisitCount++;

			// notify listeners
			if (OnNodeStart)
			{
				OnNodeStart(NodeName);
			}
		}
	}
}

void FYarnVirtualMachine::Execute_Return(const FYarnInstruction& Inst)
{
	if (CallStack.Num() > 0)
	{
		// pop return address from call stack
		FCallFrame Frame = CallStack.Pop();
		CurrentNode = FindNode(Frame.NodeName);
		ProgramCounter = Frame.ReturnAddress;
	}
	else
	{
		// no return address - stop
		ExecutionState = EExecutionState::Stopped;
	}
}

void FYarnVirtualMachine::Execute_AddSaliencyCandidate(const FYarnInstruction& Inst)
{
	// pop the condition result from the stack
	// this bool indicates whether this candidate's condition passed
	bool bConditionPassed = true;
	if (Stack.Num() > 0)
	{
		FYarnValue ConditionValue = Stack.Pop();
		bConditionPassed = ConditionValue.ToBool();
	}

	// create the candidate
	FSaliencyCandidate Candidate;
	Candidate.ContentID = Inst.StringOperand;
	Candidate.ComplexityScore = Inst.IntOperand;
	Candidate.Destination = Inst.IntOperand2;
	Candidate.bPassed = bConditionPassed;

	// add to the list
	SaliencyCandidates.Add(Candidate);

	UE_LOG(LogTemp, Verbose, TEXT("added saliency candidate: %s (score: %d, passed: %d)"),
		*Candidate.ContentID, Candidate.ComplexityScore, Candidate.bPassed);
}

void FYarnVirtualMachine::Execute_AddSaliencyCandidateFromNode(const FYarnInstruction& Inst)
{
	// get the node name
	FString NodeName = Inst.StringOperand;

	// find the node
	const FYarnNode* Node = FindNode(NodeName);
	if (!Node)
	{
		UE_LOG(LogTemp, Warning, TEXT("saliency: node not found: %s"), *NodeName);
		return;
	}

	// look for a saliency header in the node
	// headers are stored as key/value pairs in the node
	// we're looking for a "saliency" header with a numeric value
	int32 ComplexityScore = 0;

	// check if the node has a saliency header
	if (const FString* SaliencyValue = Node->Headers.Find(TEXT("saliency")))
	{
		ComplexityScore = FCString::Atoi(**SaliencyValue);
	}

	// create the candidate (always passes since we're adding from a node reference)
	FSaliencyCandidate Candidate;
	Candidate.ContentID = NodeName;
	Candidate.ComplexityScore = ComplexityScore;
	Candidate.Destination = Inst.IntOperand2;
	Candidate.bPassed = true;

	// add to the list
	SaliencyCandidates.Add(Candidate);

	UE_LOG(LogTemp, Verbose, TEXT("added saliency candidate from node: %s (score: %d, dest: %d)"),
		*NodeName, ComplexityScore, Candidate.Destination);
}

void FYarnVirtualMachine::Execute_SelectSaliencyCandidate(const FYarnInstruction& Inst)
{
	// filter to only candidates that passed their conditions
	TArray<FSaliencyCandidate> PassedCandidates;
	for (const FSaliencyCandidate& Candidate : SaliencyCandidates)
	{
		if (Candidate.bPassed)
		{
			PassedCandidates.Add(Candidate);
		}
	}

	// clear the candidate list
	SaliencyCandidates.Empty();

	// if no candidates passed, push false
	if (PassedCandidates.Num() == 0)
	{
		Stack.Push(FYarnValue(false));
		UE_LOG(LogTemp, Verbose, TEXT("no saliency candidates passed"));
		return;
	}

	// calculate total complexity score (used for weighted random selection)
	int32 TotalComplexity = 0;
	for (const FSaliencyCandidate& Candidate : PassedCandidates)
	{
		TotalComplexity += FMath::Max(1, Candidate.ComplexityScore); // ensure at least 1
	}

	// select a random candidate based on weighted probability
	int32 RandomValue = FMath::RandRange(0, TotalComplexity - 1);
	int32 Accumulator = 0;
	FSaliencyCandidate* SelectedCandidate = nullptr;

	for (FSaliencyCandidate& Candidate : PassedCandidates)
	{
		Accumulator += FMath::Max(1, Candidate.ComplexityScore);
		if (RandomValue < Accumulator)
		{
			SelectedCandidate = &Candidate;
			break;
		}
	}

	// fallback to first candidate if something went wrong
	if (!SelectedCandidate)
	{
		SelectedCandidate = &PassedCandidates[0];
	}

	// push the destination and true onto the stack
	Stack.Push(FYarnValue(static_cast<float>(SelectedCandidate->Destination)));
	Stack.Push(FYarnValue(true));

	UE_LOG(LogTemp, Log, TEXT("selected saliency candidate: %s (dest: %d)"),
		*SelectedCandidate->ContentID, SelectedCandidate->Destination);
}

FYarnValue FYarnVirtualMachine::CallFunction(const FString& FunctionName, int32 ArgumentCount)
{
	// find function in library
	FFunctionInfo* FuncInfo = FunctionLibrary.Find(FunctionName);

	if (!FuncInfo)
	{
		UE_LOG(LogTemp, Error, TEXT("function not found: %s"), *FunctionName);
		return FYarnValue(false);
	}

	// pop arguments from stack (in reverse order)
	TArray<FYarnValue> Arguments;
	for (int32 i = 0; i < ArgumentCount; i++)
	{
		if (Stack.Num() > 0)
		{
			Arguments.Insert(Stack.Pop(), 0);
		}
	}

	// call function
	return FuncInfo->Function(Arguments);
}

void FYarnVirtualMachine::RegisterBuiltInFunctions()
{
	// type conversion functions

	// string(value) - convert any value to string
	RegisterFunction(TEXT("string"), [](const TArray<FYarnValue>& Args) -> FYarnValue
	{
		if (Args.Num() > 0)
		{
			return FYarnValue(Args[0].ToString());
		}
		return FYarnValue(FString());
	}, 1);

	// number(value) - convert any value to number
	RegisterFunction(TEXT("number"), [](const TArray<FYarnValue>& Args) -> FYarnValue
	{
		if (Args.Num() > 0)
		{
			return FYarnValue(Args[0].ToFloat());
		}
		return FYarnValue(0.0f);
	}, 1);

	// bool(value) - convert any value to boolean
	RegisterFunction(TEXT("bool"), [](const TArray<FYarnValue>& Args) -> FYarnValue
	{
		if (Args.Num() > 0)
		{
			return FYarnValue(Args[0].ToBool());
		}
		return FYarnValue(false);
	}, 1);

	// visited node tracking functions

	// visited(node_name) - check if a node has been visited
	RegisterFunction(TEXT("visited"), [this](const TArray<FYarnValue>& Args) -> FYarnValue
	{
		if (Args.Num() > 0)
		{
			FString NodeName = Args[0].ToString();
			const int32* VisitCount = VisitedNodes.Find(NodeName);
			return FYarnValue(VisitCount != nullptr && *VisitCount > 0);
		}
		return FYarnValue(false);
	}, 1);

	// visited_count(node_name) - count how many times a node has been visited
	RegisterFunction(TEXT("visited_count"), [this](const TArray<FYarnValue>& Args) -> FYarnValue
	{
		if (Args.Num() > 0)
		{
			FString NodeName = Args[0].ToString();
			const int32* VisitCount = VisitedNodes.Find(NodeName);
			return FYarnValue(VisitCount ? static_cast<float>(*VisitCount) : 0.0f);
		}
		return FYarnValue(0.0f);
	}, 1);
}
