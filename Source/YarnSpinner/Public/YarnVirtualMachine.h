// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "YarnValue.h"
#include "YarnTypes.h"

/**
 * option presented to the player during dialogue
 */
struct FYarnOption
{
	// the line id for this option's text
	FString LineID;

	// the localized text for this option
	FString Text;

	// the destination instruction if this option is selected
	int32 Destination;

	// whether this option is available (condition evaluated to true)
	bool bIsAvailable;

	// index of this option
	int32 Index;
};

/**
 * stack-based virtual machine for executing compiled yarn dialogue
 *
 * future-proof design: uses ue-friendly structures (FYarnNode, FYarnInstruction)
 * - protobuf is only used for deserialization, never touches vm
 * - easy to upgrade yarn spinner or nanopb - just regenerate
 * implements all 23 instruction types from yarn specification
 */
class YARNSPINNER_API FYarnVirtualMachine
{
public:
	FYarnVirtualMachine();
	~FYarnVirtualMachine();

	/**
	 * execution state of the vm
	 */
	enum class EExecutionState : uint8
	{
		Stopped,           // not running
		Running,           // executing instructions
		WaitingForContinue, // delivered a line, waiting for continue()
		WaitingForOption,  // delivered options, waiting for setselectedoption()
		DeliveringContent  // currently delivering content to handlers
	};

	/**
	 * event delegates
	 * these are called when the vm encounters specific instructions
	 */

	// called when a line should be shown
	// params: line id, array of substitution values
	TFunction<void(const FString&, const TArray<FString>&)> OnLine;

	// called when a command should be executed
	// param: command text
	TFunction<void(const FString&)> OnCommand;

	// called when options should be presented
	// param: array of options
	TFunction<void(const TArray<FYarnOption>&)> OnOptions;

	// called when a node starts
	// param: node name
	TFunction<void(const FString&)> OnNodeStart;

	// called when a node completes
	// param: node name
	TFunction<void(const FString&)> OnNodeComplete;

	// called when all dialogue is complete
	TFunction<void()> OnDialogueComplete;

	/**
	 * core vm api
	 */

	// load a compiled program into the vm
	void LoadProgram(class UYarnProgram* InProgram);

	// start execution at the specified node
	void SetStartNode(const FString& NodeName);

	// continue execution after waiting for input
	void Continue();

	// select an option (when in WaitingForOption state)
	void SetSelectedOption(int32 OptionIndex);

	// stop execution
	void Stop();

	// get current execution state
	EExecutionState GetExecutionState() const { return ExecutionState; }

	// check if the vm is currently running
	bool IsRunning() const { return ExecutionState != EExecutionState::Stopped; }

	/**
	 * variable storage
	 */

	// set variable storage provider
	// if null, uses internal map
	void SetVariableStorage(class IYarnVariableStorage* InStorage);

	// get/set variables directly (for testing/debugging)
	FYarnValue GetVariable(const FString& VarName) const;
	void SetVariable(const FString& VarName, const FYarnValue& Value);

	/**
	 * function library
	 */

	// function signature
	typedef TFunction<FYarnValue(const TArray<FYarnValue>&)> FYarnFunction;

	// register a function that can be called from yarn scripts
	// parameter_count specifies how many arguments the function expects (-1 = variable)
	void RegisterFunction(const FString& FunctionName, FYarnFunction Function, int32 ParameterCount = -1);

	// unregister a function
	void UnregisterFunction(const FString& FunctionName);

private:
	/**
	 * vm state
	 */

	// the program being executed
	class UYarnProgram* CurrentProgram;

	// currently executing node (ue-friendly structure)
	const FYarnNode* CurrentNode;

	// program counter (index into current node's instructions)
	int32 ProgramCounter;

	// value stack for expression evaluation
	TArray<FYarnValue> Stack;

	// call stack for node detours/subroutines
	struct FCallFrame
	{
		FString NodeName;
		int32 ReturnAddress;
	};
	TArray<FCallFrame> CallStack;

	// pending options to present to the player
	TArray<FYarnOption> PendingOptions;

	// selected option index
	int32 SelectedOptionIndex;

	// execution state
	EExecutionState ExecutionState;

	/**
	 * variable storage
	 */

	class IYarnVariableStorage* VariableStorage;

	// internal variable storage (used if no external storage provided)
	TMap<FString, FYarnValue> InternalVariables;

	/**
	 * visited node tracking
	 */

	// track how many times each node has been visited
	// maps node name → visit count
	TMap<FString, int32> VisitedNodes;

	/**
	 * saliency system
	 */

	// saliency candidate for content selection
	struct FSaliencyCandidate
	{
		FString ContentID;
		int32 ComplexityScore;
		int32 Destination;
		bool bPassed; // whether the candidate's condition passed
	};

	// list of pending saliency candidates
	TArray<FSaliencyCandidate> SaliencyCandidates;

	/**
	 * function library
	 */

	// function metadata
	struct FFunctionInfo
	{
		FYarnFunction Function;
		int32 ParameterCount; // -1 = variable argument count
	};

	TMap<FString, FFunctionInfo> FunctionLibrary;

	/**
	 * instruction execution
	 */

	// find a node by name in the current program
	const FYarnNode* FindNode(const FString& NodeName) const;

	// execute the next instruction
	void ExecuteInstruction(const FYarnInstruction& Inst);

	// instruction handlers for all 23 opcodes
	void Execute_JumpTo(const FYarnInstruction& Inst);
	void Execute_PeekAndJump(const FYarnInstruction& Inst);
	void Execute_RunLine(const FYarnInstruction& Inst);
	void Execute_RunCommand(const FYarnInstruction& Inst);
	void Execute_AddOption(const FYarnInstruction& Inst);
	void Execute_ShowOptions(const FYarnInstruction& Inst);
	void Execute_PushString(const FYarnInstruction& Inst);
	void Execute_PushFloat(const FYarnInstruction& Inst);
	void Execute_PushBool(const FYarnInstruction& Inst);
	void Execute_JumpIfFalse(const FYarnInstruction& Inst);
	void Execute_Pop(const FYarnInstruction& Inst);
	void Execute_CallFunc(const FYarnInstruction& Inst);
	void Execute_PushVariable(const FYarnInstruction& Inst);
	void Execute_StoreVariable(const FYarnInstruction& Inst);
	void Execute_Stop(const FYarnInstruction& Inst);
	void Execute_RunNode(const FYarnInstruction& Inst);
	void Execute_PeekAndRunNode(const FYarnInstruction& Inst);
	void Execute_DetourToNode(const FYarnInstruction& Inst);
	void Execute_PeekAndDetourToNode(const FYarnInstruction& Inst);
	void Execute_Return(const FYarnInstruction& Inst);
	void Execute_AddSaliencyCandidate(const FYarnInstruction& Inst);
	void Execute_AddSaliencyCandidateFromNode(const FYarnInstruction& Inst);
	void Execute_SelectSaliencyCandidate(const FYarnInstruction& Inst);

	// helper: call a function with arguments from the stack
	FYarnValue CallFunction(const FString& FunctionName, int32 ArgumentCount);

	// helper: register built-in functions
	void RegisterBuiltInFunctions();
};

/**
 * interface for external variable storage
 * implement this to integrate with your save system
 */
class IYarnVariableStorage
{
public:
	virtual ~IYarnVariableStorage() = default;

	virtual FYarnValue GetValue(const FString& VariableName) = 0;
	virtual void SetValue(const FString& VariableName, const FYarnValue& Value) = 0;
	virtual bool Contains(const FString& VariableName) const = 0;
	virtual void Clear() = 0;
};
