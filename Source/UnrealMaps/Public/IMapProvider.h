#pragma once

#include "UnrealMapsStructures.h"

struct UNREALMAPS_API FMapTileDefinition
{
	FMapTileDefinition() {}
	FMapTileDefinition(const FMapLocation& inCenter, int32 inZoomLevel) :
		Center(inCenter), ZoomLevel(inZoomLevel)
	{}

	FMapLocation Center;
	int32 ZoomLevel = 3;

	bool operator==(const FMapTileDefinition& other) const
	{
		return other.ZoomLevel == ZoomLevel && other.Center == Center;
	}
};

class UTexture2D;

typedef FSimpleMulticastDelegate FOnTileLoadedEvent;

class UNREALMAPS_API IMapProvider
{
public:
	virtual void GetTileSize(int32 InScale, FMapLocation& outTileSize) const PURE_VIRTUAL(GetTileSize, );

	virtual void LoadTile(const FMapTileDefinition& Tile, const FOnTileLoadedEvent::FDelegate& OnTileLoaded) PURE_VIRTUAL(LoadTile, );

	virtual UTexture2D* GetTileTexture(const FMapTileDefinition& Tile) const PURE_VIRTUAL(GetTileTexture, return nullptr;);
};