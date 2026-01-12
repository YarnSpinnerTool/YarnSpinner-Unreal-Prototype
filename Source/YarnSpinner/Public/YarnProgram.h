// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "YarnTypes.h"
#include "YarnValue.h"

#include "YarnProgram.generated.h"

/**
 * uyarnprogram is a unreal asset that stores compiled yarn dialogue
 *
 * future-proof architecture:
 * - stores raw .yarnc bytes (standard protobuf wire format)
 * - deserializes once on first access using nanopb callbacks
 * - caches decoded data as ue-friendly structures (FYarnNode, FYarnValue)
 * - vm uses cached structures only - never touches protobuf
 *
 * benefits:
 * - upgrading yarn spinner = just regenerate from new .proto
 * - upgrading nanopb = just regenerate, no code changes
 * - no arbitrary size limits
 * - clean separation between serialization and runtime
 */
UCLASS(BlueprintType)
class YARNSPINNER_API UYarnProgram : public UObject
{
	GENERATED_BODY()

public:
	UYarnProgram();

	// raw compiled bytecode from ysc compiler (.yarnc file contents)
	UPROPERTY()
	TArray<uint8> CompiledBytecode;

	// string table mapping line ids to localized text (from ysc csv output)
	UPROPERTY()
	TMap<FString, FString> StringTable;

	// metadata for lines (optional, from metadata csv)
	UPROPERTY()
	TMap<FString, FString> LineMetadata;

	// get a node by name (deserializes on first call, returns cached thereafter)
	const FYarnNode* GetNode(const FString& NodeName);

	// get initial value for a variable
	bool GetInitialValue(const FString& VariableName, FYarnValue& OutValue);

	// get localized text for a line id
	FString GetLineText(const FString& LineID) const;

	// check if a node exists
	bool HasNode(const FString& NodeName);

	// get list of all node names
	TArray<FString> GetNodeNames();

	// check if deserialized
	bool IsDeserialized() const { return bDeserialized; }

	// ue lifecycle
	virtual void BeginDestroy() override;
	virtual void Serialize(FArchive& Ar) override;

private:
	// cached decoded data (ue-friendly structures, not protobuf)
	TMap<FString, FYarnNode> CachedNodes;
	TMap<FString, FYarnValue> CachedInitialValues;

	// deserialization state
	bool bDeserialized = false;

	// deserialize bytecode using nanopb callbacks (called once)
	void DeserializeBytecode();
};
