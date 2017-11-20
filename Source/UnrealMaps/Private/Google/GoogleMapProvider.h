#pragma once

#include "CoreMinimal.h"
#include "IHttpRequest.h"
#include "IMapProvider.h"

struct FLoadedMapTile
{
	FMapTileDefinition Tile;
	TWeakObjectPtr<UTexture2D> Texture;
};

class IHttpResponse;

struct FPendingTileReq
{
	TSharedPtr<IHttpRequest> HttpRequest;
	FMapTileDefinition Tile;
	FOnTileLoadedEvent LoadedEvent;
};

class FGoogleMapProvider : public IMapProvider, public TSharedFromThis<FGoogleMapProvider>
{

public:

	virtual ~FGoogleMapProvider() {};

	/* IMapProvider */
	void GetTileSize(int32 InScale, FMapLocation& outTileSize) const override;

	void LoadTile(const FMapTileDefinition& Tile, const FOnTileLoadedEvent::FDelegate& OnTileLoaded) override;

	UTexture2D* GetTileTexture(const FMapTileDefinition& Tile) const override;

private:

	FPendingTileReq* FindOrAddAPIRequest(const FMapTileDefinition& Tile);

	FString GetRequestURL(const FMapTileDefinition& Tile) const;

	FString GetMapTypeString(EMapDisplayType MapDisplayType) const;

	void CompactLoadedMapTiles();

	void OnPendingRequestComplete(FHttpRequestPtr ReqPtr, FHttpResponsePtr Response,bool Success);

private:

	TArray<FPendingTileReq> PendingMapTiles;
	TArray<FLoadedMapTile> LoadedMapTiles;

	EMapDisplayType MapDisplayType;
};