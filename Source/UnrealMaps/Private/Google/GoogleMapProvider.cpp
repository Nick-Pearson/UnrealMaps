#include "GoogleMapProvider.h"
#include "../UnrealMapsSettings.h"
#include "../MapsCacheManager.h"
#include "../UnrealMapsHelperFunctions.h"
#include "IUnrealMaps.h"

#include "HttpModule.h"
#include "IHttpResponse.h"
#include "IImageWrapperModule.h"
#include "ModuleManager.h"
#include "IImageWrapper.h"
#include "Engine/Texture2D.h"

FGoogleMapProvider::FGoogleMapProvider()
{
	CacheManager = FMapsCacheManager::Get();
}

void FGoogleMapProvider::GetTileSize(int32 InScale, FMapLocation& outTileSize) const
{
	float LongVal = 1.0f;
	float LatVal = 1.0f;

	switch (InScale)
	{
	case 1:
		LatVal = 180.0f;
		LongVal = 360.0f;
		break;
	case 2:
		LatVal = 90.0f;
		LongVal = 180.0f;
		break;
	case 3:
		LatVal = 66.4f;
		LongVal = 90.0f;
		break;
	case 4:
		LatVal = 33.2f;
		LongVal = 45.0f;
		break;
	case 5:
		LatVal = 12.45f;
		LongVal = 22.5;
		break;
	case 6:
		LatVal = 6.9f;
		LongVal = 11.25f;
		break;
	case 7:
		LatVal = 3.520f;
		LongVal = 5.625f;
		break;
	}

	if (InScale > 12)
	{
		LatVal = 3.510f / (2 << (InScale - 8));
		LongVal = 5.625f / (2 << (InScale - 8));
	}

	if (InScale > 7)
	{
		LatVal = 3.510f / (2 << (InScale - 8));
		LongVal = 5.625f / (2 << (InScale - 8));
	}

	outTileSize = FMapLocation(LatVal, LongVal);
}

void FGoogleMapProvider::LoadTile(const FMapTileDefinition& Tile, const FOnTileLoadedEvent::FDelegate& OnTileLoaded)
{
	FMapTileDefinition Tile_cpy = Tile;
	Tile_cpy.TileSize.X = FMath::Min(Tile_cpy.TileSize.X, 640);
	Tile_cpy.TileSize.Y = FMath::Min(Tile_cpy.TileSize.Y, 640);

	UTexture2D* LoadedTile = GetTileTexture(Tile_cpy);
	
	if (!LoadedTile)
	{
		LoadedTile = CacheManager->LoadFromCache(Tile_cpy.GetCacheKey());

		if (LoadedTile)
		{
			FLoadedMapTile* LoadedTilePtr = &LoadedMapTiles[LoadedMapTiles.AddDefaulted()];
			LoadedTilePtr->Tile = Tile_cpy;
			LoadedTilePtr->Texture = LoadedTile;
		}
	}

	if (LoadedTile)
	{
		OnTileLoaded.ExecuteIfBound();
		return;
	}

	FPendingTileReq* PendingReq = FindOrAddAPIRequest(Tile_cpy);

	if (PendingReq)
		PendingReq->LoadedEvent.Add(OnTileLoaded);
}

UTexture2D* FGoogleMapProvider::GetTileTexture(const FMapTileDefinition& Tile) const
{
	const FLoadedMapTile* LoadedTile = LoadedMapTiles.FindByPredicate([&](const FLoadedMapTile& QueryTile) {
		return QueryTile.Tile == Tile;
	});

	if (LoadedTile) return LoadedTile->Texture.Get();
	return nullptr;
}

void FGoogleMapProvider::FlushRequests()
{
	for (FPendingTileReq& PendingReq : PendingMapTiles)
	{
		if(PendingReq.HttpRequest.IsValid())
			PendingReq.HttpRequest->CancelRequest();
	}

	PendingMapTiles.Empty();
}

FPendingTileReq* FGoogleMapProvider::FindOrAddAPIRequest(const FMapTileDefinition& Tile)
{
	FPendingTileReq* ReqPtr = PendingMapTiles.FindByPredicate([&](const FPendingTileReq& PendingReq) {
		return PendingReq.Tile == Tile;
	});

	if (ReqPtr) return ReqPtr;

	TSharedRef<IHttpRequest> HttpReq = FHttpModule::Get().CreateRequest();
	HttpReq->SetVerb("GET");
	FString ReqURL = GetRequestURL(Tile);
	
	if (ReqURL.IsEmpty())
		return nullptr;

	HttpReq->SetURL(ReqURL);

	HttpReq->OnProcessRequestComplete().BindSP(this, &FGoogleMapProvider::OnPendingRequestComplete);

	HttpReq->ProcessRequest();

	// Add new request
	ReqPtr = &PendingMapTiles[PendingMapTiles.AddDefaulted()];
	ReqPtr->HttpRequest = HttpReq;
	ReqPtr->Tile = Tile;
	return ReqPtr;
}

FString FGoogleMapProvider::GetRequestURL(const FMapTileDefinition& Tile) const
{
	const UUnrealMapsSettings* Settings = GetDefault<UUnrealMapsSettings>();
	if (!Settings || Settings->Google_API_Key.IsEmpty()) return "";

	return FString::Printf(TEXT("https://maps.googleapis.com/maps/api/staticmap?center=%.3f,%.3f&zoom=%d&size=%dx%d&maptype=%s&key=%s"),
		Tile.Center.Lat,
		Tile.Center.Long,
		Tile.ZoomLevel,
		Tile.TileSize.X, Tile.TileSize.Y,
		*GetMapTypeString(Tile.DisplayType),
		*Settings->Google_API_Key);
}

FString FGoogleMapProvider::GetMapTypeString(EMapDisplayType MapDisplayType) const
{
	switch (MapDisplayType)
	{
	case EMapDisplayType::Roadmap:
		return "roadmap";
	case EMapDisplayType::Satellite:
		return "satellite";
	case EMapDisplayType::Hybrid:
		return "hybrid";
	case EMapDisplayType::Terrain:
		return "terrain";
	}

	return "roadmap";
}

void FGoogleMapProvider::CompactLoadedMapTiles()
{
	// remove any loaded tiles that have had thier texture GC'd
	for (int32 idx = LoadedMapTiles.Num(); idx >= 0; --idx)
	{
		UTexture2D* LoadedTexture = LoadedMapTiles[idx].Texture.Get();

		if (!LoadedTexture)
		{
			LoadedMapTiles.RemoveAtSwap(idx);
		}
	}
}

void FGoogleMapProvider::OnPendingRequestComplete(FHttpRequestPtr ReqPtr, FHttpResponsePtr Response, bool Success)
{
	int32 PendingReqIdx = PendingMapTiles.FindLastByPredicate([&](const FPendingTileReq& PendingReq) {
		return PendingReq.HttpRequest == ReqPtr;
	});

	// check if the response is valid
	if (PendingReqIdx == INDEX_NONE)
	{
		UE_LOG(UnrealMaps, Warning, TEXT("Failed to find pending request for recieved response"));
		return;
	}

	if (ReqPtr->GetStatus() != EHttpRequestStatus::Succeeded || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		UE_LOG(UnrealMaps, Warning, TEXT("HTTP Request failed"));
		return;
	}

	//determine the image format based on the reponse headers
	FString ContentType = Response->GetHeader("content-type");
	EImageFormat RecievedFormat;

	if (ContentType == "image/png")
		RecievedFormat = EImageFormat::PNG;
	else if (ContentType == "image/jpg")
		RecievedFormat = EImageFormat::JPEG;
	else
	{
		UE_LOG(UnrealMaps, Warning, TEXT("Unsupported image format"));
		return;
	}

	TSharedPtr<IImageWrapper> ImageWrapper;
	UTexture2D* NewTexture = UnrealMapsHelperFunctions::CreateTexture2DFromBytes(Response->GetContent(), RecievedFormat, ImageWrapper);

	if (NewTexture)
	{
		CacheManager->SaveToCache(PendingMapTiles[PendingReqIdx].Tile.GetCacheKey(), ImageWrapper);

		//create a loaded tile record
		FLoadedMapTile* LoadedTilePtr = &LoadedMapTiles[LoadedMapTiles.AddDefaulted()];
		LoadedTilePtr->Texture = NewTexture;
		LoadedTilePtr->Tile = PendingMapTiles[PendingReqIdx].Tile;

		PendingMapTiles[PendingReqIdx].LoadedEvent.Broadcast();
	}

	PendingMapTiles.RemoveAtSwap(PendingReqIdx);
}
