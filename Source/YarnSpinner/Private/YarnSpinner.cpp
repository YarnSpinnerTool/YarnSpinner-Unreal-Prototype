// copyright yarn spinner pty ltd
// licensed under the mit license

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * yarn spinner runtime module
 *
 * this module provides the core yarn spinner functionality:
 * - uyarnprogram asset for storing compiled dialogue
 * - fyarnvirtualmachine for executing dialogue
 * - blueprint integration via udialoguerunner
 *
 * the module has minimal dependencies - just nanopb for protobuf deserialization
 * no google protobuf, no .net runtime - just pure c++ with a tiny (~100kb) dependency
 */
class FYarnSpinnerModule : public IModuleInterface
{
public:
	/** imoduleinterface implementation */
	virtual void StartupModule() override
	{
		UE_LOG(LogTemp, Log, TEXT("yarn spinner 3 runtime module starting up"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogTemp, Log, TEXT("yarn spinner 3 runtime module shutting down"));
	}
};

IMPLEMENT_MODULE(FYarnSpinnerModule, YarnSpinner)
