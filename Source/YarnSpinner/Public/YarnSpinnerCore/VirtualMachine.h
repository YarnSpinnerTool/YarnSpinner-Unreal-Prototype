#pragma once

#include <string>
#include <memory>
#include "YarnSpinnerCore/yarn_spinner.pb.h"

#include "YarnSpinnerCore/Common.h"
#include "YarnSpinnerCore/Library.h"
#include "YarnSpinnerCore/State.h"
#include "Value.h"

#include <functional>

namespace Yarn
{

    class YARNSPINNER_API IVariableStorage
    {
    public:
        virtual void SetValue(std::string name, bool value) = 0;
        virtual void SetValue(std::string name, float value) = 0;
        virtual void SetValue(std::string name, std::string value) = 0;

        virtual bool HasValue(std::string name) = 0;
        virtual Value GetValue(std::string name) = 0;

        virtual void ClearValue(std::string name) = 0;
    };

    class YARNSPINNER_API VirtualMachine
    {
    public:
        enum ExecutionState
        {
            /// The VirtualMachine is not running a node.
            STOPPED,

            /// The VirtualMachine is waiting on option selection. Call
            /// SetSelectedOption before calling Continue.
            WAITING_ON_OPTION_SELECTION,

            /// The VirtualMachine has finished delivering content to the client
            /// game, and is waiting for Continue to be called.
            WAITING_FOR_CONTINUE,

            /// The VirtualMachine is delivering a line, options, or a
            /// commmand to the client game.
            DELIVERING_CONTENT,

            /// The VirtualMachine is in the middle of executing code.
            RUNNING,

            /// The VirtualMachine has encountered an error and cannot continue executing.
            ERROR
        };

    private:
        Yarn::Program program;

        Yarn::Node currentNode;

        std::string currentNodeName;

        State state;

        ExecutionState executionState;

        // Library &library;
        ILogger &logger;
        IVariableStorage &variableStorage;

    public:
        VirtualMachine(Yarn::Program program, /*Library &library,*/ IVariableStorage &variableStorage, ILogger &logger);
        ~VirtualMachine();

        void SetProgram(Yarn::Program program);
        const Yarn::Program &GetProgram();

        bool SetNode(const char *nodeName);
        const char *GetCurrentNodeName();

        ExecutionState GetCurrentExecutionState();

        // Begins or continues execution of the virtual machine.
        bool Continue();

        std::function<void(Line &)> LineHandler;
        std::function<void(OptionSet &)> OptionsHandler;
        std::function<void(Command &)> CommandHandler;
        std::function<void(std::string)> NodeStartHandler;
        std::function<void(std::string)> NodeCompleteHandler;
        std::function<void()> DialogueCompleteHandler;
        std::function<bool(std::string)> DoesFunctionExist;
        std::function<int(std::string)> GetExpectedFunctionParamCount;
        std::function<Yarn::Value(std::string, std::vector<Yarn::Value>)> CallFunction;

        void SetSelectedOption(int selectedOptionIndex);

        static std::string ExpandSubstitutions(std::string templateString, std::vector<std::string> substitutions);

    private:
        void SetCurrentExecutionState(ExecutionState state);
        bool CheckCanContinue();
        bool RunInstruction(Yarn::Instruction &instruction);
        int FindInstructionPointForLabel(std::string label);
    };
}
