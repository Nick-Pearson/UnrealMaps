#include "GoogleMapProvider.h"
#include "../UnrealMapsSettings.h"

#include "HttpModule.h"
#include "IHttpResponse.h"
#include "IImageWrapperModule.h"
#include "ModuleManager.h"
#include "IImageWrapper.h"
#include "Engine/Texture2D.h"

void FGoogleMapProvider::GetTileSize(int32 InScale, FMapLocation& outTileSize) const
{
	float LongVal = 1.0f;
	float LatVal = 1.0f;

	switch (InScale)
	{
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		break;
	case 8:
		break;
	case 9:
		break;
	case 10:
		LongVal = 0.7f;
		LatVal = 0.425f;
		break;
	case 11:
		break;
	case 12:
		break;
	case 13:
		break;
	}

	outTileSize = FMapLocation(LatVal, LongVal);
}

void FGoogleMapProvider::LoadTile(const FMapTileDefinition& Tile, const FOnTileLoadedEvent::FDelegate& OnTileLoaded)
{
	UTexture2D* LoadedTile = GetTileTexture(Tile);

	if (LoadedTile)
	{
		OnTileLoaded.ExecuteIfBound();
		return;
	}

	FPendingTileReq* PendingReq = FindOrAddAPIRequest(Tile);

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
		Settings->TileSize, Settings->TileSize,
		*GetMapTypeString(MapDisplayType),
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

void FGoogleMapProvider::OnPendingRequestComplete(FHttpRequestPtr ReqPtr, FHttpResponsePtr Response,bool Success)
{
	FPendingTileReq* PendingReqPtr = PendingMapTiles.FindByPredicate([&](const FPendingTileReq& PendingReq) {
		return PendingReq.HttpRequest == ReqPtr;
	});

	// check if the response is valid
	if (!PendingReqPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find pending request for recieved response"));
		return;
	}

	if (ReqPtr->GetStatus() != EHttpRequestStatus::Succeeded || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("HTTP Request failed"));
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
		UE_LOG(LogTemp, Warning, TEXT("Unsupported image format"));
		return;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper").CreateImageWrapper(RecievedFormat);

	if (!ImageWrapper.IsValid())
		return;

	// Load the raw HTML bytes into an image wrapper
	TArray<uint8> ResponseBytes = Response->GetContent();
	ImageWrapper->SetCompressed(ResponseBytes.GetData(), ResponseBytes.Num());

	const TArray<uint8>* DataPtr = nullptr;
	ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, DataPtr);

	if (!DataPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to decompress image"));
		return;
	}

	//create a UTexture2D container
	UTexture2D* NewTexture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight());
	
	FTexture2DMipMap& MipData = NewTexture->PlatformData->Mips[0];
	MipData.SizeX = ImageWrapper->GetWidth();
	MipData.SizeY = ImageWrapper->GetHeight();

	// memcpy the mip data
	void* TextureDataPtr = MipData.BulkData.Lock(EBulkDataLockFlags::LOCK_READ_WRITE);

	FMemory::Memcpy(TextureDataPtr, (void*)(DataPtr->GetData()), DataPtr->Num());

	MipData.BulkData.Unlock();

	NewTexture->UpdateResource();
	
	//create a loaded tile record
	FLoadedMapTile* LoadedTilePtr = &LoadedMapTiles[LoadedMapTiles.AddDefaulted()];
	LoadedTilePtr->Texture = NewTexture;
	LoadedTilePtr->Tile = PendingReqPtr->Tile;

	PendingReqPtr->LoadedEvent.Broadcast();
}
