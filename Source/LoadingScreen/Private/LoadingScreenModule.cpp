#include "LoadingScreenModule.h"
#include "LoadingScreenSettings.h"

#define LOCTEXT_NAMESPACE "FLoadingScreenModule"

void FLoadingScreenModule::StartupModule()
{
    // Subsystem will handle loading screen initialization
}

void FLoadingScreenModule::ShutdownModule()
{
    // Cleanup is handled by subsystem
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLoadingScreenModule, LoadingScreen)
