#pragma once

#include "CoreMinimal.h"
#include "YarnSpinnerCore/Value.h"


class FYarnValueHelpers
{
public:
    static FString GetTypeString(const Yarn::Value& Value)
    {
        return Value.IsString() ? TEXT("string") : (Value.IsNumber() ? TEXT("number") : TEXT("boolean"));
    }
};


