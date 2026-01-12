// copyright yarn spinner pty ltd
// licensed under the mit license

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * yarn spinner editor module
 *
 * this module provides editor-time functionality:
 * - asset import for .yarnproject files
 * - automatic compilation using ysc
 * - string table generation
 * - preview and debugging tools
 *
 * no heavy dependencies - just calls ysc and reads the output
 */
class FYarnSpinnerEditorModule : public IModuleInterface
{
public:
	/** imoduleinterface implementation */
	virtual void StartupModule() override
	{
		UE_LOG(LogTemp, Log, TEXT("yarn spinner 3 editor module starting up"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogTemp, Log, TEXT("yarn spinner 3 editor module shutting down"));
	}
};

IMPLEMENT_MODULE(FYarnSpinnerEditorModule, YarnSpinnerEditor)
