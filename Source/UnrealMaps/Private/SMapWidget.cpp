#include "SMapWidget.h"

#include "IUnrealMaps.h"
#include "IMapProvider.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"

void SMapWidget::Construct(const FArguments& InArgs)
{	
	MapProvider = IUnrealMaps::Get().CreateMapProvder();

	RenderCanvas = NewObject<UMapCanvasRenderTarget2D>();
	RenderCanvas->SizeX = 500;
	RenderCanvas->SizeY = 500;
	RenderCanvas->ClearColor = FLinearColor::Black;
	RenderCanvas->PassthroughEvent.AddSP(this, &SMapWidget::CanvasRenderUpdate);

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

int32 SMapWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (bRequiresUpdate) UpdateMapDisplay();

	return IMapWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

FReply SMapWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	int32 Change = MouseEvent.GetWheelDelta() > 0 ? 1 : -1;
	CurrentScale += Change;
	CurrentScale = FMath::Clamp(CurrentScale, 1, 20);
	InvalidateMapDisplay();

	return FReply::Handled();
}

void SMapWidget::UpdateMapDisplay() const
{
	if (!RenderCanvas || !MapProvider.IsValid())
		return;

	RenderCanvas->UpdateResource();
	bRequiresUpdate = false;
}

void SMapWidget::CanvasRenderUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
	const UUnrealMapsSettings* Settings = GetDefault<UUnrealMapsSettings>();

	if (!ensure(Canvas) || !ensure(MapProvider.IsValid()) || !Settings)
		return;
	
	FMapLocation Size;
	MapProvider->GetTileSize(CurrentScale, Size);

	//clamp requested images to a grid space to assist caching of images
	FMapLocation BaseLocation(FMath::CeilToFloat(CurrentLocation.Lat / Size.Lat) * Size.Lat, FMath::FloorToFloat(CurrentLocation.Long / Size.Long) * Size.Long);
	FVector2D BaseLocationOffset((CurrentLocation.Long - BaseLocation.Long) / Size.Long, (CurrentLocation.Lat - BaseLocation.Lat) / Size.Lat);
	BaseLocationOffset *= Settings->TileSize;

	//calc how many tiles fit into a screen
	int32 ScreenTileRadiusX = FMath::CeilToInt(CanvasBrush.ImageSize.X / (Settings->TileSize * 2.0f));
	int32 ScreenTileRadiusY = FMath::CeilToInt(CanvasBrush.ImageSize.Y / (Settings->TileSize * 2.0f));
	FVector2D ScreenCenter = FVector2D(CanvasBrush.ImageSize.X / 2.0f, CanvasBrush.ImageSize.Y / 2.0f);

	for (int32 x = -ScreenTileRadiusX; x <= ScreenTileRadiusX; ++x)
	{
		for (int32 y = -ScreenTileRadiusY; y <= ScreenTileRadiusY; ++y)
		{
			FMapTileDefinition RequiredMapTile = FMapTileDefinition(BaseLocation + FMapLocation(Size.Lat * -y, Size.Long * x), CurrentScale, Settings->TileSize, DisplayType);
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
