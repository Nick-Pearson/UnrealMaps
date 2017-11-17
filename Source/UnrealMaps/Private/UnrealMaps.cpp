#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IUnrealMaps.h"


class FUnrealMaps : public IUnrealMaps
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FUnrealMaps, UnrealMaps )


void FUnrealMaps::StartupModule()
{
}


void FUnrealMaps::ShutdownModule()
{
}



