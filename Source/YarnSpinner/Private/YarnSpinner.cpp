// Copyright Epic Games, Inc. All Rights Reserved.

#include "YarnSpinner.h"

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/MessageDialog.h"
#include "Misc/YSLogging.h"

THIRD_PARTY_INCLUDES_START
#include <google/protobuf/stubs/logging.h>
THIRD_PARTY_INCLUDES_END


#define LOCTEXT_NAMESPACE "FYarnSpinnerModule"



void UnrealLogHandler(google::protobuf::LogLevel level, const char* filename, int line,
                        const std::string& message) {
	UE_LOG(LogYarnSpinner, Warning, TEXT("Protobuf: %hs"), message.c_str());
}


void FYarnSpinnerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	SetLogHandler(UnrealLogHandler);
	
	UE_LOG(LogYarnSpinner, Display, TEXT("Installed Protobuf log handler"));
}

void FYarnSpinnerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

bool FYarnSpinnerModule::SupportsDynamicReloading()
{
    return true;
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYarnSpinnerModule, YarnSpinner)
