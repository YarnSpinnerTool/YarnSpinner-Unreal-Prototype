// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "YarnValue.h"

/**
 * ue-friendly representation of yarn instructions
 * decoded from protobuf format on load, cached for fast access
 *
 * this separates us from protobuf implementation details,
 * making upgrades to yarn/nanopb simple
 */
struct YARNSPINNER_API FYarnInstruction
{
	// instruction type (matches protobuf opcode)
	enum class EOpCode : uint8
	{
		JumpTo = 1,
		RunLine = 3,
		RunCommand = 4,
		AddOption = 5,
		ShowOptions = 6,
		PushString = 7,
		PushFloat = 8,
		PushBool = 9,
		JumpIfFalse = 10,
		Pop = 11,
		CallFunc = 12,
		PushVariable = 13,
		StoreVariable = 14,
		Stop = 15,
		RunNode = 16,
		DetourToNode = 18,
		Return = 20,
		AddSaliencyCandidate = 21,
		SelectSaliencyCandidate = 23,
		PeekAndJump = 2,
		PeekAndRunNode = 17,
		PeekAndDetourToNode = 19,
		AddSaliencyCandidateFromNode = 22
	};

	EOpCode OpCode;

	// operands (decoded from protobuf, stored as ue types)
	FString StringOperand;      // for lineID, commandText, variableName, etc.
	float FloatOperand;          // for float values
	int32 IntOperand;            // for destinations, counts, complexityScore
	int32 IntOperand2;           // for secondary int values (e.g., destination in AddSaliencyCandidate)
	bool BoolOperand;            // for boolean values
	int32 SubstitutionCount;     // for substitutions
	bool HasCondition;           // for options with conditions

	FYarnInstruction()
		: OpCode(EOpCode::Stop)
		, FloatOperand(0.0f)
		, IntOperand(0)
		, IntOperand2(0)
		, BoolOperand(false)
		, SubstitutionCount(0)
		, HasCondition(false)
	{
	}
};

/**
 * ue-friendly representation of a yarn node
 * decoded from protobuf, cached for fast vm execution
 */
struct YARNSPINNER_API FYarnNode
{
	FString Name;
	TArray<FYarnInstruction> Instructions;
	TMap<FString, FString> Headers; // metadata like tags, etc.

	FYarnNode() = default;
	explicit FYarnNode(const FString& InName) : Name(InName) {}
};
