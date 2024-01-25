#pragma once

#include <string>
#include "YarnSpinnerCore/Common.h"
#include <cmath>

namespace Yarn
{
    class YARNSPINNER_API Value
    {

    public:
        enum ValueType
        {
            STRING,
            NUMBER,
            BOOL
        };

        ValueType type;

        std::string stringValue;
        double number;
        bool boolean;

        Value() : type(ValueType::NUMBER), number(0) {}

        Value(const char *string) : type(ValueType::STRING), stringValue(std::string(string)) {}

        Value(const std::string &string) : type(ValueType::STRING), stringValue(string) {}

        Value(float number) : type(ValueType::NUMBER), number(number) {}
        Value(double number) : type(ValueType::NUMBER), number(number) {}
        Value(int number) : type(ValueType::NUMBER), number(number) {}

        Value(bool boolean) : type(ValueType::BOOL), boolean(boolean) {}

        ValueType GetType() const
        {
            return this->type;
        }

        bool IsString() const
        {
            return GetType() == STRING;
        }

        bool IsNumber() const
        {
            return GetType() == NUMBER;
        }

        bool IsBoolean() const
        {
            return GetType() == BOOL;
        }

        const std::string GetStringValue()
        {
            if (this->type == STRING)
            {
                return this->stringValue;
            }
            else
            {
                return "";
            }
        }

        float GetNumberValue()
        {
            if (this->type == NUMBER)
            {
                return this->number;
            }
            else
            {
                return 0;
            }
        }

        float ConvertToNumber()
        {
            if (type == STRING)
            {
                return atof(stringValue.c_str());
            }
            if (type == BOOL)
            {
                return boolean ? 1 : 0;
            }
            return number;
        }

        bool GetBooleanValue()
        {
            if (this->type == BOOL)
            {
                return this->boolean;
            }
            else
            {
                return false;
            }
        }

        const std::string ConvertToString()
        {
            switch (type)
            {
            case STRING:
                return stringValue;
            case BOOL:
                return boolean ? "True" : "False";
            case NUMBER:
                if (trunc(number) == number)
                {
                    return std::to_string((int)number);
                }
                else
                {
                    return std::to_string(number);
                }
            default:
                return "<unknown>";
            }
        }
    };
}
