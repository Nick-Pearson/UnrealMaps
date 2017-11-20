#include "SMapWidget.h"

#include "IUnrealMaps.h"
#include "IMapProvider.h"

#include "SImage.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"

void SMapWidget::Construct(const FArguments& InArgs)
{	
	MapProvider = IUnrealMaps::Get().CreateMapProvder();

	RenderCanvas = NewObject<UMapCanvasRenderTarget2D>();
	RenderCanvas->SizeX = 1500;
	RenderCanvas->SizeY = 1500;
	RenderCanvas->ClearColor = FLinearColor::Black;
	RenderCanvas->PassthroughEvent.AddSP(this, &SMapWidget::CanvasRenderUpdate);

	CanvasBrush.ImageSize = FVector2D(1500, 1500);
	CanvasBrush.SetResourceObject(RenderCanvas);

	MapCanvasContainer = SNew(SImage).Image(&CanvasBrush);

	ChildSlot
	[
		MapCanvasContainer.ToSharedRef()
	]
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top);

	bRequiresUpdate = true;
}

void SMapWidget::SetLocation(const FMapLocation& Location)
{
	CurrentLocation = Location;

	UpdateMapDisplay();
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

void SMapWidget::UpdateMapDisplay() const
{
	if (!RenderCanvas || !MapProvider.IsValid())
		return;

	RenderCanvas->FastUpdateResource();
	bRequiresUpdate = false;
}

void SMapWidget::CanvasRenderUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (!ensure(Canvas) || !ensure(MapProvider.IsValid()))
		return;
	FMapLocation Size;
	MapProvider->GetTileSize(CurrentScale, Size);

	for (int32 x = -1; x <= 1; ++x)
	{
		for (int32 y = -1; y <= 1; ++y)
		{
			FMapTileDefinition RequiredMapTile = FMapTileDefinition(CurrentLocation + FMapLocation(Size.Lat * x, Size.Long * y), CurrentScale);
			UTexture2D* TileTexture = MapProvider->GetTileTexture(RequiredMapTile);

			if (!TileTexture)
			{
				MapProvider->LoadTile(RequiredMapTile, FOnTileLoadedEvent::FDelegate::CreateSP(this, &SMapWidget::UpdateMapDisplay));
				continue;
			}

			FCanvasTileItem NewItem(FVector2D(500.0f + (500.0f * y), 500.0f + (500.0f * -x)), TileTexture->Resource, FLinearColor::White);
			NewItem.Draw(Canvas->Canvas);
		}
	}
}