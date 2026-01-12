// copyright yarn spinner pty ltd
// licensed under the mit license

#include "YarnValue.h"
#include "yarn_spinner.pb.h"

FYarnValue FYarnValue::FromOperand(const Yarn_Operand& Operand)
{
	// nanopb uses which_value to indicate which oneof field is set
	switch (Operand.which_value)
	{
		case Yarn_Operand_string_value_tag:
		{
			// protobuf strings are stored as pb_callback_t
			// for now, return empty string - will implement callback handling later
			// todo: implement proper string extraction from pb_callback
			return FYarnValue(FString(TEXT("")));
		}

		case Yarn_Operand_float_value_tag:
			return FYarnValue(Operand.value.float_value);

		case Yarn_Operand_bool_value_tag:
			return FYarnValue(Operand.value.bool_value);

		default:
			// unknown type - default to false
			return FYarnValue(false);
	}
}

FString FYarnValue::ToString() const
{
	switch (Type)
	{
		case EType::String:
			return StringValue;

		case EType::Float:
			return FString::SanitizeFloat(FloatValue);

		case EType::Bool:
			return BoolValue ? TEXT("true") : TEXT("false");

		default:
			return TEXT("");
	}
}

float FYarnValue::ToFloat() const
{
	switch (Type)
	{
		case EType::Float:
			return FloatValue;

		case EType::String:
			// attempt to parse string as float
			return FCString::Atof(*StringValue);

		case EType::Bool:
			return BoolValue ? 1.0f : 0.0f;

		default:
			return 0.0f;
	}
}

bool FYarnValue::ToBool() const
{
	switch (Type)
	{
		case EType::Bool:
			return BoolValue;

		case EType::Float:
			// non-zero is true
			return !FMath::IsNearlyZero(FloatValue);

		case EType::String:
			// non-empty string is true
			return !StringValue.IsEmpty();

		default:
			return false;
	}
}

bool FYarnValue::operator==(const FYarnValue& Other) const
{
	// if types differ, convert to common type and compare
	if (Type != Other.Type)
	{
		// if either is string, compare as strings
		if (Type == EType::String || Other.Type == EType::String)
		{
			return ToString() == Other.ToString();
		}

		// otherwise compare as floats
		return FMath::IsNearlyEqual(ToFloat(), Other.ToFloat());
	}

	// same type - direct comparison
	switch (Type)
	{
		case EType::String:
			return StringValue == Other.StringValue;
		case EType::Float:
			return FMath::IsNearlyEqual(FloatValue, Other.FloatValue);
		case EType::Bool:
			return BoolValue == Other.BoolValue;
		default:
			return false;
	}
}

bool FYarnValue::operator!=(const FYarnValue& Other) const
{
	return !(*this == Other);
}

bool FYarnValue::operator<(const FYarnValue& Other) const
{
	// for ordering, we compare as floats
	// strings compare lexicographically
	if (Type == EType::String && Other.Type == EType::String)
	{
		return StringValue < Other.StringValue;
	}

	return ToFloat() < Other.ToFloat();
}

bool FYarnValue::operator<=(const FYarnValue& Other) const
{
	return *this < Other || *this == Other;
}

bool FYarnValue::operator>(const FYarnValue& Other) const
{
	return !(*this <= Other);
}

bool FYarnValue::operator>=(const FYarnValue& Other) const
{
	return !(*this < Other);
}

FYarnValue FYarnValue::operator+(const FYarnValue& Other) const
{
	// string concatenation
	if (Type == EType::String || Other.Type == EType::String)
	{
		return FYarnValue(ToString() + Other.ToString());
	}

	// numeric addition
	return FYarnValue(ToFloat() + Other.ToFloat());
}

FYarnValue FYarnValue::operator-(const FYarnValue& Other) const
{
	return FYarnValue(ToFloat() - Other.ToFloat());
}

FYarnValue FYarnValue::operator*(const FYarnValue& Other) const
{
	return FYarnValue(ToFloat() * Other.ToFloat());
}

FYarnValue FYarnValue::operator/(const FYarnValue& Other) const
{
	float Divisor = Other.ToFloat();

	if (FMath::IsNearlyZero(Divisor))
	{
		UE_LOG(LogTemp, Warning, TEXT("division by zero in yarn value operation"));
		return FYarnValue(0.0f);
	}

	return FYarnValue(ToFloat() / Divisor);
}

FYarnValue FYarnValue::operator%(const FYarnValue& Other) const
{
	return FYarnValue(FMath::Fmod(ToFloat(), Other.ToFloat()));
}

FYarnValue FYarnValue::operator-() const
{
	return FYarnValue(-ToFloat());
}

FYarnValue FYarnValue::operator!() const
{
	return FYarnValue(!ToBool());
}

FString FYarnValue::GetTypeName() const
{
	switch (Type)
	{
		case EType::String: return TEXT("string");
		case EType::Float: return TEXT("number");
		case EType::Bool: return TEXT("bool");
		default: return TEXT("unknown");
	}
}
