#include "IUnrealMaps.h"

#include "SMapWidget.h"
#include "UnrealMapsSettings.h"
#include "Google/GoogleMapProvider.h"

#include "ISettingsModule.h"
#include "ISettingsSection.h"

#define LOCTEXT_NAMESPACE "UnrealMaps"

class FUnrealMaps : public IUnrealMaps
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
public:
	TSharedPtr<IMapWidget> CreateMapWidget(const FMapWidgetParams& Params, const FMapLocation& InitialLocation) const override;

	TSharedPtr<IMapProvider> CreateMapProvder() const override;

};

IMPLEMENT_MODULE( FUnrealMaps, UnrealMaps )

void FUnrealMaps::StartupModule()
{
#if WITH_EDITOR
	// register settings
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	
	ISettingsSectionPtr SettingsSection = SettingsModule.RegisterSettings("Project", "Plugins", "UnrealMaps",
		LOCTEXT("UMSettingsName", "Unreal Maps"),
		LOCTEXT("UMSettingsDescription", "Configure the Unreal Maps plugins."),
		GetMutableDefault<UUnrealMapsSettings>()
	);
#endif // WITH_EDITOR
}


void FUnrealMaps::ShutdownModule()
{
#if WITH_EDITOR
	// unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "UnrealMaps");
	}
#endif
}

TSharedPtr<IMapWidget> FUnrealMaps::CreateMapWidget(const FMapWidgetParams& Params, const FMapLocation& InitialLocation) const
{
	TSharedPtr<SMapWidget> Widget;
	SAssignNew(Widget, SMapWidget);

	Widget->SetLocation(InitialLocation);

	return StaticCastSharedPtr<IMapWidget, SMapWidget>(Widget);
}

TSharedPtr<IMapProvider> FUnrealMaps::CreateMapProvder() const
{
	return TSharedPtr<IMapProvider>(new FGoogleMapProvider);
}

#undef LOCTEXT_NAMESPACE