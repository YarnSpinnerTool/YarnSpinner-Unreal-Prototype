#include "YarnSpinnerCore/VirtualMachine.h"

#include "CoreMinimal.h"
#include "YarnSubsystem.h"

#include <stack>
#include <regex>
#include <string>
#include <sstream>


namespace Yarn
{
    VirtualMachine::VirtualMachine(Yarn::Program program, /*Library& library,*/ IVariableStorage& variableStorage, ILogger& logger)
        : program(program),
          state(State()),
          executionState(STOPPED),
          // library(library),
          logger(logger),
          variableStorage(variableStorage)
    {
        // // Add the 'visited' and 'visited_count' functions, which query the variable
        // // storage for information about how many times a node has been visited.
        // library.AddFunction<bool>(
        //     "visited",
        //     [&variableStorage](std::vector<Value> values)
        //     {
        //         std::string nodeName = values.at(0).GetStringValue();
        //         std::string visitTrackingVariable = Library::GenerateUniqueVisitedVariableForNode(nodeName);
        //         if (variableStorage.HasValue(visitTrackingVariable))
        //         {
        //             int visitCount = variableStorage.GetValue(visitTrackingVariable).GetNumberValue();
        //             return visitCount > 0;
        //         }
        //         else
        //         {
        //             return false;
        //         }
        //     },
        //     1);
        //
        // library.AddFunction<float>(
        //     "visited_count",
        //     [&variableStorage](std::vector<Value> values)
        //     {
        //         std::string nodeName = values.at(0).GetStringValue();
        //         std::string visitTrackingVariable = Library::GenerateUniqueVisitedVariableForNode(nodeName);
        //         if (variableStorage.HasValue(visitTrackingVariable))
        //         {
        //             int visitCount = (int)variableStorage.GetValue(visitTrackingVariable).GetNumberValue();
        //             return visitCount;
        //         }
        //         else
        //         {
        //             return 0;
        //         }
        //     },
        //     1);
    }


    VirtualMachine::~VirtualMachine()
    {
    }


    void VirtualMachine::SetProgram(Yarn::Program newProgram)
    {
        this->program = newProgram;
        SetCurrentExecutionState(STOPPED);
        state.programCounter = 0;
    }


    const Yarn::Program& VirtualMachine::GetProgram()
    {
        return this->program;
    }


    bool VirtualMachine::SetNode(const char* nodeName)
    {
        if (program.nodes().contains(nodeName) == false)
        {
            logger.Log(string_format("No node named %s has been loaded.", nodeName), ILogger::ERROR);
            return false;
        }

        logger.Log(string_format("Running node %s", nodeName));

        currentNode = program.nodes().at(nodeName);

        // Clear our State and return to the Stopped execution state
        state = State();
        SetCurrentExecutionState(ExecutionState::STOPPED);

        state.currentNodeName = nodeName;

        NodeStartHandler(nodeName);

        return true;
    }


    const char* VirtualMachine::GetCurrentNodeName()
    {
        return currentNode.name().c_str();
    }


    VirtualMachine::ExecutionState VirtualMachine::GetCurrentExecutionState()
    {
        return executionState;
    }


    void VirtualMachine::SetCurrentExecutionState(VirtualMachine::ExecutionState newState)
    {
        executionState = newState;
        if (executionState == STOPPED)
        {
            // We've stopped; clear our state.
            state = State();
        }
    }


    bool VirtualMachine::Continue()
    {
        // Perform a safety check to ensure that we're in a ready state to continue
        if (CheckCanContinue() == false)
        {
            return false;
        }

        if (executionState == ExecutionState::DELIVERING_CONTENT)
        {
            // We were delivering a line, option set, or command, and the client has
            // called Continue() on us. We're still inside the stack frame of the
            // client callback, so to avoid recursion, we'll note that our state has
            // changed back to RUNNING; when we've left the callback, we'll continue
            // executing instructions.
            SetCurrentExecutionState(RUNNING);
            return true;
        }

        SetCurrentExecutionState(RUNNING);

        while (GetCurrentExecutionState() == RUNNING)
        {
            Yarn::Instruction currentInstruction = currentNode.instructions().at(state.programCounter);

            bool successfullyRanInstruction = RunInstruction(currentInstruction);

            if (!successfullyRanInstruction)
            {
                // Stop immediately - we ran into a problem when executing our
                // instruction
                SetCurrentExecutionState(VirtualMachine::ExecutionState::ERROR);
                return false;
            }

            state.programCounter += 1;

            if (state.programCounter >= currentNode.instructions_size() && GetCurrentExecutionState() != STOPPED)
            {
                NodeCompleteHandler(currentNode.name());
                SetCurrentExecutionState(STOPPED);
                DialogueCompleteHandler();
                logger.Log("Run complete.", ILogger::INFO);
            }
        }

        return true;
    }


    bool VirtualMachine::RunInstruction(Yarn::Instruction& instruction)
    {
        std::stringstream str;

        str << Yarn::Instruction_OpCode_Name(instruction.opcode());

        for (auto operand : instruction.operands())
        {
            str << " ";
            switch (operand.value_case())
            {
            case Yarn::Operand::kBoolValue:
                str << (operand.bool_value() ? "true" : "false");
                break;
            case Yarn::Operand::kFloatValueFieldNumber:
                str << operand.float_value();
                break;
            case Yarn::Operand::kStringValue:
                str << operand.string_value();
                break;
            default:
                str << "(unknown operand type!)";
            }
        }

        logger.Log(str.str());
        switch (instruction.opcode())
        {
        case Yarn::Instruction_OpCode_RUN_LINE:
            {
                // Fetch line ID
                auto lineID = instruction.operands(0).string_value();

                // Build line struct
                Line line = Line();
                line.LineID = lineID;

                // If we have 2+ operands, get the second operand as a number
                if (instruction.operands_size() > 1)
                {
                    auto expressionCount = (int)instruction.operands(1).float_value();

                    // And get that many expressions off the stack and build the
                    // collection of substitutions (in reverse order)
                    std::vector<std::string> substitutions;

                    for (int expressionIndex = expressionCount - 1; expressionIndex >= 0; expressionIndex--)
                    {
                        auto top = state.PopValue();
                        std::string topAsString = top.ConvertToString();
                        substitutions.push_back(topAsString);
                    }

                    std::reverse(substitutions.begin(), substitutions.end());

                    line.Substitutions = substitutions;
                }

                // Mark that we're currently delivering content
                SetCurrentExecutionState(DELIVERING_CONTENT);

                // Call the line handler
                LineHandler(line);

                // If we're still marked as delivering content, then the line
                // handler didn't call Continue, so we'll wait here
                if (GetCurrentExecutionState() == DELIVERING_CONTENT)
                {
                    SetCurrentExecutionState(WAITING_FOR_CONTINUE);
                }

                break;
            }
        case Yarn::Instruction_OpCode_RUN_COMMAND:
            {
                std::string commandText = instruction.operands(0).string_value();

                // If we have 2+ operands, get the second operand as a number
                if (instruction.operands_size() > 1)
                {
                    auto expressionCount = (int)instruction.operands(1).float_value();

                    // And get that many expressions off the stack and build the
                    // collection of substitutions (in reverse order)
                    std::vector<std::string> substitutions;

                    for (int expressionIndex = expressionCount - 1; expressionIndex >= 0; expressionIndex--)
                    {
                        auto top = state.PopValue();
                        std::string topAsString = top.ConvertToString();

                        // std::string placeholder("\\{" << expressionIndex << "\\}");
                        commandText = std::regex_replace(commandText, std::regex("\\{" + std::to_string(expressionIndex) + "\\}"), topAsString);
                    }
                }

                SetCurrentExecutionState(DELIVERING_CONTENT);

                auto command = Command();
                command.Text = commandText;

                CommandHandler(command);

                if (GetCurrentExecutionState() == DELIVERING_CONTENT)
                {
                    // The client didn't call Continue, so we'll wait here.
                    SetCurrentExecutionState(WAITING_FOR_CONTINUE);
                }

                break;
            }
        case Yarn::Instruction_OpCode_STOP:
            {
                NodeCompleteHandler(currentNode.name());
                DialogueCompleteHandler();
                SetCurrentExecutionState(STOPPED);
                break;
            }
        case Yarn::Instruction_OpCode_PUSH_BOOL:
            {
                bool value = instruction.operands(0).bool_value();
                state.PushValue(value);
                break;
            }
        case Yarn::Instruction_OpCode_PUSH_FLOAT:
            {
                float value = instruction.operands(0).float_value();
                state.PushValue(value);
                break;
            }
        case Yarn::Instruction_OpCode_PUSH_STRING:
            {
                std::string value = instruction.operands(0).string_value();
                state.PushValue(value);
                break;
            }
        case Yarn::Instruction_OpCode_JUMP_IF_FALSE:
            {
                bool topOfStack = state.PeekValue().GetBooleanValue();
                if (topOfStack == false)
                {
                    auto label = instruction.operands(0).string_value();
                    state.programCounter = FindInstructionPointForLabel(label) - 1;
                }
                break;
            }
        case Yarn::Instruction_OpCode_JUMP_TO:
            {
                auto label = instruction.operands(0).string_value();
                state.programCounter = FindInstructionPointForLabel(label) - 1;
                break;
            }
        case Yarn::Instruction_OpCode_JUMP:
            {
                // Jumps to a label whose name is on the stack.
                auto jumpDestination = state.PeekValue().GetStringValue();
                state.programCounter = FindInstructionPointForLabel(jumpDestination) - 1;
                break;
            }
        case Yarn::Instruction_OpCode_ADD_OPTION:
            {
                Line line = Line();

                line.LineID = instruction.operands(0).string_value();
                std::string destination = instruction.operands(1).string_value();

                if (instruction.operands_size() > 2)
                {
                    // The third operand is the number of substitutions present in the
                    // line.

                    auto expressionCount = (int)instruction.operands(2).float_value();

                    // Get that many expressions off the stack and build the collection
                    // of substitutions (in reverse order)
                    std::vector<std::string> substitutions;

                    for (int expressionIndex = expressionCount - 1; expressionIndex >= 0; expressionIndex--)
                    {
                        auto top = state.PopValue();
                        std::string topAsString = top.ConvertToString();
                        substitutions.push_back(topAsString);
                    }

                    std::reverse(substitutions.begin(), substitutions.end());

                    line.Substitutions = substitutions;
                }

                // Indicates whether the VM believes that the option should be shown to
                // the user, based on any conditions that were attached to the option.
                bool lineConditionPassed = true;

                if (instruction.operands_size() > 3)
                {
                    // The fourth operand is a bool that indicates whether this option
                    // had a condition or not. If it does, then a bool value will exist
                    // on the stack indiciating whether the condition passed or not. We
                    // pass that information to the game.

                    bool hasLineCondition = instruction.operands(3).bool_value();

                    if (hasLineCondition)
                    {
                        // This option has a condition. Get it from the stack.
                        lineConditionPassed = state.PopValue().GetBooleanValue();
                    }
                }

                state.AddOption(line, destination.c_str(), lineConditionPassed);
                break;
            }
        case Yarn::Instruction_OpCode_SHOW_OPTIONS:
            {
                // Show all accumulated options to the game.

                // If we have no options to show, immediately stop.

                if (state.currentOptions.size() == 0)
                {
                    SetCurrentExecutionState(STOPPED);
                    DialogueCompleteHandler();
                    break;
                }

                // Present the list of options to the user and let them pick
                auto optionSet = OptionSet();

                optionSet.Options = state.currentOptions;

                // We can't continue until our client tell us which
                // option to pick
                SetCurrentExecutionState(WAITING_ON_OPTION_SELECTION);

                // Pass the options set to the client, as well as a
                // delegate for them to call when the user has made
                // a selection
                OptionsHandler(optionSet);

                if (GetCurrentExecutionState() == WAITING_FOR_CONTINUE)
                {
                    // we are no longer waiting on an option
                    // selection - the options handler must have
                    // called SetSelectedOption! Continue running
                    // immediately.
                    SetCurrentExecutionState(RUNNING);
                }

                break;
            }
        case Yarn::Instruction_OpCode_PUSH_NULL:
            {
                // Push a null value. This is not a valid instruction as of Yarn Spinner
                // 2.0.
                logger.Log("PUSH_NULL is not a valid instruction in Yarn Spinner 2.0+", ILogger::ERROR);
                return false;
                break;
            }
        case Yarn::Instruction_OpCode_POP:
            {
                // Remove a value from the top of the stack and discard it.
                state.PopValue();
                break;
            }
        case Yarn::Instruction_OpCode_CALL_FUNC:
            {
                // Call a named function, with parameters found on the stack, and push
                // the resulting value onto the stack.
                auto functionName = instruction.operands(0).string_value();

                auto actualParamCount = (int)state.PopValue().GetNumberValue();

                if (!DoesFunctionExist(functionName))
                {
                    logger.Log(string_format("Unknown function '%s'", functionName.c_str()), ILogger::ERROR);
                    return false;
                }

                // auto expectedParamCount = library.GetExpectedParameterCount(functionName);
                auto expectedParamCount = GetExpectedFunctionParamCount(functionName);

                if (expectedParamCount >= 0 && expectedParamCount != actualParamCount)
                {
                    logger.Log(string_format("Function '%s' expects %i parameters, but %i were provided", functionName.c_str(), expectedParamCount, actualParamCount), ILogger::ERROR);
                    return false;
                }

                std::vector<Value> parameters;

                for (int param = actualParamCount - 1; param >= 0; param--)
                {
                    auto value = state.PopValue();
                    parameters.push_back(value);
                }
                std::reverse(parameters.begin(), parameters.end());

                auto result = CallFunction(functionName, parameters);
                state.PushValue(result);

                // if (library.HasFunction<std::string>(functionName))
                // {
                //     auto function = library.GetFunction<std::string>(functionName);
                //     auto result = function.Function(parameters);
                //     state.PushValue(result);
                // }
                // else if (library.HasFunction<float>(functionName))
                // {
                //     auto function = library.GetFunction<float>(functionName);
                //     auto result = function.Function(parameters);
                //     state.PushValue(result);
                // }
                // else if (library.HasFunction<bool>(functionName))
                // {
                //     auto function = library.GetFunction<bool>(functionName);
                //     auto result = function.Function(parameters);
                //     state.PushValue(result);
                // }
                // else
                // {
                //     logger.Log(string_format("Unknown function %s", functionName.c_str()), ILogger::ERROR);
                //     return false;
                // }

                logger.Log(string_format("Function call returned \"%s\" (type: %d)", state.PeekValue().ConvertToString().c_str(), state.PeekValue().GetType()));

                break;
            }
        case Yarn::Instruction_OpCode_PUSH_VARIABLE:
            {
                // Get the contents of a variable, and push that onto the stack.
                auto variableName = instruction.operands(0).string_value();

                if (variableStorage.HasValue(variableName))
                {
                    // We found a value for this variable in the storage.
                    Value v = variableStorage.GetValue(variableName);
                    state.PushValue(v);
                }
                else if (program.initial_values().count(variableName) > 0)
                {
                    // We don't have a value for this. The initial value may be found in
                    // the program. (If it's not, then the variable's value is
                    // undefined, which isn't allowed.)
                    auto operand = program.initial_values().at(variableName);
                    switch (operand.value_case())
                    {
                    case Yarn::Operand::ValueCase::kBoolValue:
                        state.PushValue(operand.bool_value());
                        break;
                    case Yarn::Operand::ValueCase::kStringValue:
                        state.PushValue(operand.string_value());
                        break;
                    case Yarn::Operand::ValueCase::kFloatValue:
                        state.PushValue(operand.float_value());
                        break;
                    default:
                        logger.Log(string_format("Unknown initial value type %i for variable %s", operand.value_case(), variableName.c_str()), ILogger::ERROR);
                        return false;
                    }
                }
                else
                {
                    // We didn't find a value for this variable in storage or in the
                    // program's intial values. This is an error - the variable must not
                    // have been defined.
                    logger.Log(string_format("Undefined variable %s", variableName.c_str()), ILogger::ERROR);
                    return false;
                }
                break;
            }
        case Yarn::Instruction_OpCode_STORE_VARIABLE:
            {
                // Store the top value on the stack in a variable.
                auto topValue = state.PeekValue();
                auto destinationVariableName = instruction.operands(0).string_value();

                logger.Log(string_format("Set %s to %s", destinationVariableName.c_str(), topValue.ConvertToString().c_str()));

                switch (topValue.GetType())
                {
                case Value::ValueType::STRING:
                    variableStorage.SetValue(destinationVariableName, topValue.GetStringValue());
                    break;
                case Value::ValueType::NUMBER:
                    variableStorage.SetValue(destinationVariableName, topValue.GetNumberValue());
                    break;
                case Value::ValueType::BOOL:
                    variableStorage.SetValue(destinationVariableName, topValue.GetBooleanValue());
                    break;
                default:
                    logger.Log(string_format("Invalid Yarn value type %i for variable %s", topValue.GetType(), destinationVariableName.c_str()), ILogger::ERROR);
                    return false;
                }
                break;
            }
        case Yarn::Instruction_OpCode_RUN_NODE:
            {
                // Pop a string from the stack, and jump to a node with that name.
                auto nodeName = state.PopValue().GetStringValue();

                NodeCompleteHandler(currentNode.name());

                SetNode(nodeName.c_str());

                // Decrement program counter here, because it will be incremented when
                // this function returns, and would mean skipping the first instruction
                state.programCounter -= 1;

                break;
            }
        default:
            logger.Log(string_format("Unhandled instruction type %i", instruction.opcode()));
            return false;
            break;
        }

        if (GetCurrentExecutionState() != ERROR)
        {
            return true;
        }
        else
        {
            return false;
        }
    }


    int VirtualMachine::FindInstructionPointForLabel(std::string label)
    {
        if (currentNode.labels().count(label) == 0)
        {
            logger.Log(string_format("Unknown label %s in node %s", label.c_str(), currentNode.name().c_str()), ILogger::ERROR);
            SetCurrentExecutionState(ERROR);
            return -1;
        }
        return currentNode.labels().at(label);
    }


    bool VirtualMachine::CheckCanContinue()
    {
        if (executionState == WAITING_ON_OPTION_SELECTION)
        {
            logger.Log("Cannot continue running dialogue. Still waiting on option selection.", ILogger::Type::ERROR);
            return false;
        }

        if (!LineHandler)
        {
            logger.Log("Cannot continue dialogue: LineHandler has not been set.", ILogger::Type::ERROR);
            return false;
        }
        if (!OptionsHandler)
        {
            logger.Log("Cannot continue dialogue: OptionsHandler has not been set.", ILogger::Type::ERROR);
            return false;
        }
        if (!CommandHandler)
        {
            logger.Log("Cannot continue dialogue: CommandHandler has not been set.", ILogger::Type::ERROR);
            return false;
        }
        if (!NodeCompleteHandler)
        {
            logger.Log("Cannot continue dialogue: NodeCompleteHandler has not been set.", ILogger::Type::ERROR);
            return false;
        }
        if (!DialogueCompleteHandler)
        {
            logger.Log("Cannot continue dialogue: DialogueCompleteHandler has not been set.", ILogger::Type::ERROR);
            return false;
        }
        return true;
    }


    void VirtualMachine::SetSelectedOption(int selectedOptionIndex)
    {
        if (GetCurrentExecutionState() != WAITING_ON_OPTION_SELECTION)
        {
            logger.Log("SetSelectedOption was called, but Dialogue wasn't waiting for a selection. This method should only be called after the Dialogue is waiting for the user to select an option.", ILogger::ERROR);
            SetCurrentExecutionState(VirtualMachine::ERROR);
            return;
        }

        if (selectedOptionIndex < 0 || selectedOptionIndex >= (int)state.currentOptions.size())
        {
            logger.Log("SetSelectedOption was called with an invalid option index");
        }

        auto destinationNode = state.currentOptions[selectedOptionIndex].DestinationNode;
        state.PushValue(destinationNode);

        state.currentOptions.clear();

        SetCurrentExecutionState(WAITING_FOR_CONTINUE);
    }


    std::string VirtualMachine::ExpandSubstitutions(std::string templateString, std::vector<std::string> substitutions)
    {
        int i = 0;

        std::string output = templateString;
        for (std::string& sub : substitutions)
        {
            output = std::regex_replace(output, std::regex("\\{" + std::to_string(i) + "\\}"), sub);
            i++;
        }

        return output;
    }
}
