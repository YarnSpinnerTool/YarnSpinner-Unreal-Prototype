// copyright yarn spinner pty ltd
// licensed under the mit license

#include "YarnProgram.h"
#include "yarn_spinner.pb.h"
#include "pb_decode.h"

// helper: decode a nanopb string callback into fstring
static bool DecodeString(pb_istream_t* Stream, const pb_field_iter_t* Field, void** Args)
{
	FString* TargetString = (FString*)(*Args);

	uint32 Length = Stream->bytes_left;
	UE_LOG(LogTemp, Log, TEXT("DecodeString: bytes_left=%d"), Length);

	if (Length == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("DecodeString: Empty string (0 bytes)"));
		*TargetString = TEXT("");
		return true;
	}

	char* Buffer = (char*)FMemory::Malloc(Length + 1);
	if (!pb_read(Stream, (uint8*)Buffer, Length))
	{
		UE_LOG(LogTemp, Error, TEXT("DecodeString: pb_read failed"));
		FMemory::Free(Buffer);
		return false;
	}

	Buffer[Length] = '\0';
	*TargetString = FString(UTF8_TO_TCHAR(Buffer));
	UE_LOG(LogTemp, Log, TEXT("DecodeString: Decoded '%s'"), **TargetString);
	FMemory::Free(Buffer);
	return true;
}

// context for decoding instructions array
struct FInstructionDecodeContext
{
	TArray<FYarnInstruction>* Instructions;
};

// decode a single instruction from the instructions array
static bool DecodeInstruction(pb_istream_t* Stream, const pb_field_iter_t* Field, void** Args)
{
	FInstructionDecodeContext* Context = (FInstructionDecodeContext*)(*Args);

	// With static string buffers, nanopb decodes everything automatically!
	Yarn_Instruction Inst = Yarn_Instruction_init_zero;

	if (!pb_decode(Stream, Yarn_Instruction_fields, &Inst))
	{
		UE_LOG(LogTemp, Error, TEXT("DecodeInstruction: pb_decode failed"));
		return false;
	}

	// Convert nanopb instruction to UE-friendly structure
	FYarnInstruction UEInst;

	switch (Inst.which_InstructionType)
	{
	case Yarn_Instruction_jumpTo_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::JumpTo;
		UEInst.IntOperand = Inst.InstructionType.jumpTo.destination;
		break;

	case Yarn_Instruction_runLine_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::RunLine;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.runLine.lineID));
		UEInst.SubstitutionCount = Inst.InstructionType.runLine.substitutionCount;
		break;

	case Yarn_Instruction_runCommand_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::RunCommand;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.runCommand.commandText));
		UEInst.SubstitutionCount = Inst.InstructionType.runCommand.substitutionCount;
		break;

	case Yarn_Instruction_addOption_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::AddOption;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.addOption.lineID));
		UEInst.IntOperand = Inst.InstructionType.addOption.destination;
		UEInst.SubstitutionCount = Inst.InstructionType.addOption.substitutionCount;
		UEInst.HasCondition = Inst.InstructionType.addOption.hasCondition;
		break;

	case Yarn_Instruction_showOptions_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::ShowOptions;
		break;

	case Yarn_Instruction_pushString_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::PushString;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.pushString.value));
		break;

	case Yarn_Instruction_pushFloat_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::PushFloat;
		UEInst.FloatOperand = Inst.InstructionType.pushFloat.value;
		break;

	case Yarn_Instruction_pushBool_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::PushBool;
		UEInst.BoolOperand = Inst.InstructionType.pushBool.value;
		break;

	case Yarn_Instruction_jumpIfFalse_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::JumpIfFalse;
		UEInst.IntOperand = Inst.InstructionType.jumpIfFalse.destination;
		break;

	case Yarn_Instruction_pop_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::Pop;
		break;

	case Yarn_Instruction_callFunc_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::CallFunc;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.callFunc.functionName));
		// note: parameter count is looked up from function library, not stored in instruction
		break;

	case Yarn_Instruction_pushVariable_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::PushVariable;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.pushVariable.variableName));
		break;

	case Yarn_Instruction_storeVariable_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::StoreVariable;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.storeVariable.variableName));
		break;

	case Yarn_Instruction_stop_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::Stop;
		break;

	case Yarn_Instruction_runNode_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::RunNode;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.runNode.nodeName));
		break;

	case Yarn_Instruction_detourToNode_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::DetourToNode;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.detourToNode.nodeName));
		break;

	case Yarn_Instruction_return_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::Return;
		break;

	case Yarn_Instruction_addSaliencyCandidate_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::AddSaliencyCandidate;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.addSaliencyCandidate.contentID));
		UEInst.IntOperand = Inst.InstructionType.addSaliencyCandidate.complexityScore;
		UEInst.IntOperand2 = Inst.InstructionType.addSaliencyCandidate.destination;
		break;

	case Yarn_Instruction_addSaliencyCandidateFromNode_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::AddSaliencyCandidateFromNode;
		UEInst.StringOperand = FString(UTF8_TO_TCHAR(Inst.InstructionType.addSaliencyCandidateFromNode.nodeName));
		UEInst.IntOperand2 = Inst.InstructionType.addSaliencyCandidateFromNode.destination;
		break;

	case Yarn_Instruction_selectSaliencyCandidate_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::SelectSaliencyCandidate;
		break;

	case Yarn_Instruction_peekAndJump_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::PeekAndJump;
		break;

	case Yarn_Instruction_peekAndRunNode_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::PeekAndRunNode;
		break;

	case Yarn_Instruction_peekAndDetourToNode_tag:
		UEInst.OpCode = FYarnInstruction::EOpCode::PeekAndDetourToNode;
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("unknown instruction type: %d"), Inst.which_InstructionType);
		return true; // continue parsing
	}

	Context->Instructions->Add(UEInst);
	return true;
}

// context for decoding nodes map
struct FNodeDecodeContext
{
	TMap<FString, FYarnNode>* Nodes;
};

// decode a single node from the nodes map
static bool DecodeNode(pb_istream_t* Stream, const pb_field_iter_t* Field, void** Args)
{
	FNodeDecodeContext* Context = (FNodeDecodeContext*)(*Args);

	Yarn_Program_NodesEntry Entry = Yarn_Program_NodesEntry_init_zero;

	// still need a callback for the map key (it's a pb_callback_t)
	FString MapKey;
	Entry.key.funcs.decode = &DecodeString;
	Entry.key.arg = &MapKey;

	// set up callback for instructions array
	TArray<FYarnInstruction> Instructions;
	FInstructionDecodeContext InstContext{&Instructions};
	Entry.value.instructions.funcs.decode = &DecodeInstruction;
	Entry.value.instructions.arg = &InstContext;

	// decode the node entry
	if (!pb_decode(Stream, Yarn_Program_NodesEntry_fields, &Entry))
	{
		UE_LOG(LogTemp, Error, TEXT("DecodeNode: pb_decode failed for node entry"));
		return false;
	}

	// node name is now a static char array in the value
	FString NodeName = FString(UTF8_TO_TCHAR(Entry.value.name));

	// create ue-friendly node
	FYarnNode UENode(NodeName);
	UENode.Instructions = MoveTemp(Instructions);

	// add to map (use map key, which should match node name)
	Context->Nodes->Add(MapKey, UENode);

	UE_LOG(LogTemp, Log, TEXT("DecodeNode: added node '%s' with %d instructions"), *NodeName, UENode.Instructions.Num());

	return true;
}

// context for decoding initial values map
struct FInitialValueDecodeContext
{
	TMap<FString, FYarnValue>* InitialValues;
};

// decode a single initial value from the map
static bool DecodeInitialValue(pb_istream_t* Stream, const pb_field_iter_t* Field, void** Args)
{
	FInitialValueDecodeContext* Context = (FInitialValueDecodeContext*)(*Args);

	Yarn_Program_InitialValuesEntry Entry = Yarn_Program_InitialValuesEntry_init_zero;

	// still need callback for map key
	FString VarName;
	Entry.key.funcs.decode = &DecodeString;
	Entry.key.arg = &VarName;

	// decode the entry
	if (!pb_decode(Stream, Yarn_Program_InitialValuesEntry_fields, &Entry))
	{
		return false;
	}

	// convert operand to yarnvalue
	FYarnValue Value;
	switch (Entry.value.which_value)
	{
	case Yarn_Operand_string_value_tag:
		Value = FYarnValue(FString(UTF8_TO_TCHAR(Entry.value.value.string_value)));
		break;
	case Yarn_Operand_bool_value_tag:
		Value = FYarnValue(Entry.value.value.bool_value);
		break;
	case Yarn_Operand_float_value_tag:
		Value = FYarnValue(Entry.value.value.float_value);
		break;
	default:
		Value = FYarnValue(false); // default to false
		break;
	}

	Context->InitialValues->Add(VarName, Value);
	return true;
}

UYarnProgram::UYarnProgram()
	: bDeserialized(false)
{
}

void UYarnProgram::DeserializeBytecode()
{
	if (bDeserialized || CompiledBytecode.Num() == 0)
	{
		return;
	}

	// create nanopb input stream from bytecode
	pb_istream_t Stream = pb_istream_from_buffer(
		CompiledBytecode.GetData(),
		CompiledBytecode.Num()
	);

	// create empty program structure with callbacks
	Yarn_Program Program = Yarn_Program_init_zero;

	// set up node decode callback
	FNodeDecodeContext NodeContext{&CachedNodes};
	Program.nodes.funcs.decode = &DecodeNode;
	Program.nodes.arg = &NodeContext;

	// set up initial values decode callback
	FInitialValueDecodeContext InitValueContext{&CachedInitialValues};
	Program.initial_values.funcs.decode = &DecodeInitialValue;
	Program.initial_values.arg = &InitValueContext;

	// decode the program
	UE_LOG(LogTemp, Log, TEXT("starting pb_decode with %d bytes"), CompiledBytecode.Num());
	bool Success = pb_decode(&Stream, Yarn_Program_fields, &Program);

	if (!Success)
	{
		UE_LOG(LogTemp, Error, TEXT("failed to deserialize yarn program: %s"), UTF8_TO_TCHAR(PB_GET_ERROR(&Stream)));
		return;
	}

	bDeserialized = true;

	UE_LOG(LogTemp, Log, TEXT("yarn program deserialized: %d nodes, %d initial values"),
		CachedNodes.Num(), CachedInitialValues.Num());
}

const FYarnNode* UYarnProgram::GetNode(const FString& NodeName)
{
	if (!bDeserialized)
	{
		DeserializeBytecode();
	}

	return CachedNodes.Find(NodeName);
}

bool UYarnProgram::GetInitialValue(const FString& VariableName, FYarnValue& OutValue)
{
	if (!bDeserialized)
	{
		DeserializeBytecode();
	}

	const FYarnValue* Found = CachedInitialValues.Find(VariableName);
	if (Found)
	{
		OutValue = *Found;
		return true;
	}

	return false;
}

FString UYarnProgram::GetLineText(const FString& LineID) const
{
	UE_LOG(LogTemp, Log, TEXT("GetLineText: Looking up LineID='%s' in string table with %d entries"), *LineID, StringTable.Num());

	// Log first few string table keys for debugging
	if (StringTable.Num() > 0)
	{
		int32 Count = 0;
		for (const TPair<FString, FString>& Entry : StringTable)
		{
			UE_LOG(LogTemp, Log, TEXT("  StringTable[%d]: Key='%s', Value='%s'"), Count, *Entry.Key, *Entry.Value);
			if (++Count >= 5) break; // Only log first 5 entries
		}
	}

	const FString* Found = StringTable.Find(LineID);
	if (Found)
	{
		UE_LOG(LogTemp, Log, TEXT("GetLineText: Found! Returning: '%s'"), **Found);
		return *Found;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GetLineText: NOT FOUND! Returning LineID as fallback"));
		return LineID;
	}
}

bool UYarnProgram::HasNode(const FString& NodeName)
{
	if (!bDeserialized)
	{
		DeserializeBytecode();
	}

	return CachedNodes.Contains(NodeName);
}

TArray<FString> UYarnProgram::GetNodeNames()
{
	if (!bDeserialized)
	{
		DeserializeBytecode();
	}

	TArray<FString> Names;
	CachedNodes.GetKeys(Names);
	return Names;
}

void UYarnProgram::BeginDestroy()
{
	CachedNodes.Empty();
	CachedInitialValues.Empty();
	bDeserialized = false;

	Super::BeginDestroy();
}

void UYarnProgram::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// only serialize the bytecode and string table
	// cached data is reconstructed on load
}
