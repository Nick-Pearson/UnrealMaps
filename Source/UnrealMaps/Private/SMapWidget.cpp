#include "SMapWidget.h"

#include "IUnrealMaps.h"
#include "IMapProvider.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"

void SMapWidget::Construct(const SMapWidget::FArguments& InArgs)
{	
	Params = InArgs._inParams;

	MapProvider = IUnrealMaps::Get().CreateMapProvder();

	RenderCanvas = NewObject<UMapCanvasRenderTarget2D>();
	RenderCanvas->SizeX = 500;
	RenderCanvas->SizeY = 500;
	RenderCanvas->ClearColor = FLinearColor::Black;

	if(Params.bUseTiles)
		RenderCanvas->PassthroughEvent.AddSP(this, &SMapWidget::CanvasRenderUpdate_Tiled);
	else
		RenderCanvas->PassthroughEvent.AddSP(this, &SMapWidget::CanvasRenderUpdate_Full);


	CanvasBrush.ImageSize = FVector2D(500, 500);
	CanvasBrush.SetResourceObject(RenderCanvas);

	MapCanvasContainer = SNew(SMapCanvasContainer).Image(&CanvasBrush);
	MapCanvasContainer->ContainerResizedEvent.AddSP(this, &SMapWidget::ResizeCanvas);

	ChildSlot
	[
		MapCanvasContainer.ToSharedRef()
	];

	InvalidateMapDisplay();
}

void SMapWidget::SetLocation(const FMapLocation& Location)
{
	CurrentLocation = Location;

	if (MapProvider.IsValid()) MapProvider->FlushRequests();

	InvalidateMapDisplay();
}

void SMapWidget::GetLocation(FMapLocation& Location) const
{
	Location = CurrentLocation;
}

void SMapWidget::SetMapScale(int32 NewScale)
{
	NewScale = FMath::Clamp(NewScale, 1, 20);

	if (CurrentScale == NewScale) return;

	CurrentScale = NewScale;
	InvalidateMapDisplay();
}

int32 SMapWidget::GetMapScale() const
{
	return CurrentScale;
}

int32 SMapWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (bRequiresUpdate) UpdateMapDisplay();

	return IMapWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

FReply SMapWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	int32 Change = MouseEvent.GetWheelDelta() > 0 ? 1 : -1;
	SetMapScale(CurrentScale + Change);

	return FReply::Handled();
}

void SMapWidget::UpdateMapDisplay() const
{
	if (!RenderCanvas || !MapProvider.IsValid())
		return;

	bRequiresUpdate = false;
	RenderCanvas->UpdateResource();
}


void SMapWidget::CanvasRenderUpdate_Tiled(UCanvas* Canvas, int32 Width, int32 Height)
{
	const UUnrealMapsSettings* Settings = GetDefault<UUnrealMapsSettings>();

	if (!ensure(Canvas) || !ensure(MapProvider.IsValid()) || !Settings)
		return;
	
	FMapLocation Size;
	MapProvider->GetTileSize(CurrentScale, Size);

	FMapLocation CurrLoc = GetCurrentLocationScaled();

	//clamp requested images to a grid space to assist caching of images
	FMapLocation BaseLocation(FMath::CeilToFloat(CurrLoc.Lat / Size.Lat) * Size.Lat, FMath::FloorToFloat(CurrLoc.Long / Size.Long) * Size.Long);
	FVector2D BaseLocationOffset((CurrLoc.Long - BaseLocation.Long) / Size.Long, (CurrLoc.Lat - BaseLocation.Lat) / Size.Lat);
	BaseLocationOffset *= Settings->TileSize;

	//calc how many tiles fit into a screen
	int32 ScreenTileRadiusX = FMath::CeilToInt(CanvasBrush.ImageSize.X / (Settings->TileSize * 2.0f));
	int32 ScreenTileRadiusY = FMath::CeilToInt(CanvasBrush.ImageSize.Y / (Settings->TileSize * 2.0f));
	FVector2D ScreenCenter = FVector2D(CanvasBrush.ImageSize.X / 2.0f, CanvasBrush.ImageSize.Y / 2.0f);

	for (int32 x = -ScreenTileRadiusX; x <= ScreenTileRadiusX; ++x)
	{
		for (int32 y = -ScreenTileRadiusY; y <= ScreenTileRadiusY; ++y)
		{
			FMapTileDefinition RequiredMapTile = FMapTileDefinition(BaseLocation + FMapLocation(Size.Lat * -y, Size.Long * x), CurrentScale, Settings->TileSize, Params.DisplayType);
			UTexture2D* TileTexture = MapProvider->GetTileTexture(RequiredMapTile);

			if (!TileTexture)
			{
				MapProvider->LoadTile(RequiredMapTile, FOnTileLoadedEvent::FDelegate::CreateSP(this, &SMapWidget::InvalidateMapDisplay));
				continue;
			}
			
			FCanvasTileItem NewItem(
				FVector2D(ScreenCenter.X - (Settings->TileSize * 0.5f) + (Settings->TileSize * x) - BaseLocationOffset.X,
					ScreenCenter.Y - (Settings->TileSize * 0.5f) + (Settings->TileSize * y) + BaseLocationOffset.Y),
				TileTexture->Resource, FLinearColor::White);
			NewItem.Draw(Canvas->Canvas);
		}
	}
}

void SMapWidget::CanvasRenderUpdate_Full(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (!ensure(Canvas) || !ensure(MapProvider.IsValid()))
		return;

	FMapLocation CurrLoc = GetCurrentLocationScaled();

	FMapTileDefinition RequiredTexture = FMapTileDefinition(CurrLoc, CurrentScale, FIntPoint((int32)CanvasBrush.ImageSize.X, (int32)CanvasBrush.ImageSize.Y), Params.DisplayType);
	UTexture2D* LoadedTexture = MapProvider->GetTileTexture(RequiredTexture);

	if (!LoadedTexture)
	{
		MapProvider->LoadTile(RequiredTexture, FOnTileLoadedEvent::FDelegate::CreateSP(this, &SMapWidget::InvalidateMapDisplay));
		return;
	}

	FCanvasTileItem NewItem(FVector2D(0.0f, 0.0f), LoadedTexture->Resource, FLinearColor::White);
	NewItem.Draw(Canvas->Canvas);
}

FMapLocation SMapWidget::GetCurrentLocationScaled() const
{
	FMapLocation CurrLoc = CurrentLocation;

	//blend to 0,0 as we zoom out
	if (CurrentScale <= 5.0f)
	{
		const float ScaleFactor = FMath::Clamp((CurrentScale - 3.0f) / 2.0f, 0.0f, 1.0f);
		CurrLoc = FMapLocation(CurrLoc.Lat * ScaleFactor, CurrLoc.Long * ScaleFactor);
	}
	return CurrLoc;
}

void SMapWidget::ResizeCanvas(int32 NewWidth, int32 NewHeight)
{
	CanvasBrush.ImageSize = FVector2D(NewWidth, NewHeight);

	if (RenderCanvas)
	{
		RenderCanvas->SizeX = NewWidth;
		RenderCanvas->SizeY = NewHeight;
		InvalidateMapDisplay();
	}
}
