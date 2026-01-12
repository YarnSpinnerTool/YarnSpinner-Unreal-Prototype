# Yarn Spinner 3 for Unreal Engine - Quick Start

Paris' Program (a nanopb-based implementation of Yarn Spinner 3 for Unreal Engine..)

It Kinda Works!™

You'll need `ysc` available system-wide.

## Installation

### Option 1: Plugin in Existing Project
1. Copy the `YarnSpinner` folder to your project's `Plugins/` directory
2. Regenerate project files
3. Build your project

### Option 2: New Project
1. Create new UE project
2. Create `Plugins/` folder in project root
3. Copy `YarnSpinner` folder into `Plugins/`
4. Regenerate project files and build

## Quick Usage

### 1. Import Yarn Files

1. Create a `.yarn` file with dialogue.

2. Create a `.yarnproject` file in the same folder.

3. **Import the .yarnproject and .yarn files** into UE Content Browser (drag & drop or right-click → Import)
4. 
5. This creates a `UYarnProgram` asset automatically, assuming it works.

### 2. Basic Blueprint Setup

**Add DialogueRunner component to your actor:**

```
1. Add DialogueRunner component to Blueprint
2. Set YarnProgram reference to your imported asset
3. Set Start Node (default: "Start")
4. Enable "Auto Start" if desired
```

**Handle dialogue events:**

```
Event OnLineReceived (LineID, LineText)
  → Print String (LineText)
  → Wait for input...
  → Call DialogueRunner->Continue()

Event OnOptionsReceived (Options)
  → Show options to player
  → When option selected: Call DialogueRunner->SelectOption(Index)

Event OnCommandReceived (CommandText)
  → Parse and execute game commands
```

## Creating a Custom Presenter

A presenter is a reusable UI component that handles dialogue display. Create one by implementing `IDialoguePresenter`:

### C++ Presenter

```cpp
UCLASS()
class UMyDialoguePresenter : public UObject, public IDialoguePresenter
{
    GENERATED_BODY()

public:
    // IDialoguePresenter interface
    virtual void RunLine_Implementation(const FString& LineID, const FString& LineText) override;
    virtual void RunOptions_Implementation(const TArray<FDialogueOption>& Options) override;
    virtual void DismissLine_Implementation() override;

    // Create and manage your UMG widgets here
};
```

### Blueprint Presenter

1. Create Blueprint class implementing `DialoguePresenter` interface
2. Implement interface functions:
   - `Run Line` - Show dialogue text in widget
   - `Run Options` - Display choice buttons
   - `Dismiss Line` - Hide/clear dialogue UI
3. Assign to DialogueRunner's "Dialogue Presenter" property

**See:** `SimpleDialoguePresenter.h` for a working example

## What's Working, Maybe

- Import .yarnproject files (auto-compiles with `ysc`)
- Deserialise .yarnc bytecode (nanopb)
- Virtual Machine execution (al instruction types)
- Line delivery with string table localization
- Options/choices
- Commands
- Variables (bool, float, string)
- Variable storage (built-in memory storage)
- Function library integration
- Node jumping and flow control
- Detours
- Blueprint-ish API unless I misunderstood
- Presenter system (probably needs work tho)
- Enums (handled as raw types automatically, I think)

## What's Mssing/In Progress

- Saliency system (instructions present, needs testing)..
- Custom function registration (API exists, needs examples)...
- Line view markup (character names, tags, attributes)...
- Persistent variable storage (interface exists, needs save/load integration)..
- Loc support beyond string table...
- Test coverage

## What's Different from the previous implementation

* Per Tim's suggeston, we use nanopb, which makes it like 2MB instead of 45MB
* Direct nanopb structs for the VM

## Files etc.

| File | Purpose |
|------|---------|
| `YarnProgram.h/.cpp` | Asset class, nanopb deserialisation |
| `YarnVirtualMachine.h/.cpp` | Instruction execution engine |
| `DialogueRunner.h/.cpp` | Blueprint component API |
| `DialogueWidget.h/.cpp` | Simple UMG base widget |
| `IDialoguePresenter.h` | Interface for custom UI |
| `YarnAssetFactory.cpp` | Import pipeline (calls ysc) |