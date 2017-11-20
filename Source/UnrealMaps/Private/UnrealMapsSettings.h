#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UnrealMapsSettings.generated.h"

UCLASS(config=Game)
class UUnrealMapsSettings : public UObject
{
	GENERATED_BODY()
	
public:

	UPROPERTY(config, EditAnywhere, Category = "General")
	int32 TileSize = 500;

	UPROPERTY(config, EditAnywhere, Category = "General|Cache")
	bool bUseCache = true;

	UPROPERTY(config, EditAnywhere, Category = "General|Cache", meta = (DisplayName = "Max Cache Size (MB)", EditCondition = "bUseCache"))
	int32 MaxCacheSize = 512;

	UPROPERTY(config, EditAnywhere, Category = "Google", meta = (DisplayName = "API Key"))
	FString Google_API_Key;
};
