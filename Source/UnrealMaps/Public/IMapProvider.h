#pragma once

#include "UnrealMapsStructures.h"

struct UNREALMAPS_API FMapTileDefinition
{
	FMapTileDefinition() {}

	FMapTileDefinition(const FMapLocation& inCenter, int32 inZoomLevel, int32 inTileSize, EMapDisplayType inDisplayType) :
		Center(inCenter), ZoomLevel(inZoomLevel), TileSize(inTileSize, inTileSize), DisplayType(inDisplayType)
	{}

	FMapTileDefinition(const FMapLocation& inCenter, int32 inZoomLevel, FIntPoint inTileSize, EMapDisplayType inDisplayType) :
		Center(inCenter), ZoomLevel(inZoomLevel), TileSize(inTileSize), DisplayType(inDisplayType)
	{}

	FMapLocation Center;
	int32 ZoomLevel = 3;
	FIntPoint TileSize = FIntPoint(500, 500);
	EMapDisplayType DisplayType = EMapDisplayType::Roadmap;

	bool operator==(const FMapTileDefinition& other) const
	{
		return other.ZoomLevel == ZoomLevel && other.Center == Center && other.TileSize == TileSize && other.DisplayType == DisplayType;
	}

	FString GetCacheKey() const
	{
		return FString::Printf(TEXT("%.1f.%.1f.%d.%d.%d.%d"), Center.Lat, Center.Long, ZoomLevel, TileSize.X, TileSize.Y, (int32)DisplayType);
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

	virtual void FlushRequests() PURE_VIRTUAL(FlushRequests, );
};