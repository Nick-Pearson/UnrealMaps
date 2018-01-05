#pragma once

#include "CoreMinimal.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"

class IMapWidget;
class IMapProvider;
struct FMapWidgetParams;
struct FMapLocation;

DECLARE_LOG_CATEGORY_EXTERN(UnrealMaps, Log, All);

class IUnrealMaps : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IUnrealMaps& Get()
	{
		return FModuleManager::LoadModuleChecked< IUnrealMaps >( "UnrealMaps" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "UnrealMaps" );
	}

	virtual TSharedPtr<IMapWidget> CreateMapWidget(const FMapWidgetParams& Params, const FMapLocation& InitialLocation) const PURE_VIRTUAL(CreateMapWidget, return nullptr;);

	virtual TSharedPtr<IMapProvider> CreateMapProvder() const PURE_VIRTUAL(CreateMapProvder, return nullptr;);
};

