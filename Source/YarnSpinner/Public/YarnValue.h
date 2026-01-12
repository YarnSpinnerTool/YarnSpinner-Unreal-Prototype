// copyright yarn spinner pty ltd
// licensed under the mit license

#pragma once

#include "CoreMinimal.h"
#include "YarnValue.generated.h"

/**
 * represents a value in the yarn type system
 *
 * yarn has three value types: string, float (number), and bool
 * this struct provides conversions and operations between them
 */
USTRUCT(BlueprintType)
struct YARNSPINNER_API FYarnValue
{
	GENERATED_BODY()

public:
	// value type enumeration
	enum class EType : uint8
	{
		String,
		Float,
		Bool
	};

	// the type of this value
	EType Type;

	// storage for the value
	// string is stored separately due to non-trivial constructor
	UPROPERTY()
	FString StringValue;

	float FloatValue;
	bool BoolValue;

	// default constructor
	FYarnValue()
		: Type(EType::Bool)
		, StringValue(TEXT(""))
		, FloatValue(0.0f)
		, BoolValue(false)
	{
	}

	// construct from string
	explicit FYarnValue(const FString& Value)
		: Type(EType::String)
		, StringValue(Value)
		, FloatValue(0.0f)
		, BoolValue(false)
	{
	}

	// construct from float
	explicit FYarnValue(float Value)
		: Type(EType::Float)
		, StringValue(TEXT(""))
		, FloatValue(Value)
		, BoolValue(false)
	{
	}

	// construct from bool
	explicit FYarnValue(bool Value)
		: Type(EType::Bool)
		, StringValue(TEXT(""))
		, FloatValue(0.0f)
		, BoolValue(Value)
	{
	}

	// construct from nanopb operand
	static FYarnValue FromOperand(const struct _Yarn_Operand& Operand);

	// type conversions - follow yarn's conversion rules
	FString ToString() const;
	float ToFloat() const;
	bool ToBool() const;

	// operators for vm operations
	bool operator==(const FYarnValue& Other) const;
	bool operator!=(const FYarnValue& Other) const;
	bool operator<(const FYarnValue& Other) const;
	bool operator<=(const FYarnValue& Other) const;
	bool operator>(const FYarnValue& Other) const;
	bool operator>=(const FYarnValue& Other) const;

	FYarnValue operator+(const FYarnValue& Other) const;
	FYarnValue operator-(const FYarnValue& Other) const;
	FYarnValue operator*(const FYarnValue& Other) const;
	FYarnValue operator/(const FYarnValue& Other) const;
	FYarnValue operator%(const FYarnValue& Other) const;

	FYarnValue operator-() const; // unary negation
	FYarnValue operator!() const; // logical not

	// get type name for debugging
	FString GetTypeName() const;
};
