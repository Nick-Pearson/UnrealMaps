#pragma once

#include "CoreMinimal.h"

#include "IMapWidget.h"
#include "UnrealMapsStructures.h"

struct FMapLocation;

class IMapProvider;
class UCanvas;
class UMapCanvasRenderTarget2D;
class SMapCanvasContainer;

class SMapWidget : public IMapWidget
{
public:

	SLATE_BEGIN_ARGS(SMapWidget)
	{}
	SLATE_ARGUMENT(FMapWidgetParams, inParams)
	SLATE_END_ARGS()

	void Construct(const SMapWidget::FArguments& InArgs);

	// IMapWidget
	virtual void SetLocation(const FMapLocation& Location) override;
	virtual void GetLocation(FMapLocation& Location) const override;

	virtual void SetMapScale(int32 NewScale) override;
	virtual int32 GetMapScale() const override;
	
	FORCEINLINE void InvalidateMapDisplay() { bRequiresUpdate = true; Invalidate(EInvalidateWidget::LayoutAndVolatility); }
	
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;


private:

	void UpdateMapDisplay() const;

	void CanvasRenderUpdate_Tiled(UCanvas* Canvas, int32 Width, int32 Height);
	void CanvasRenderUpdate_Full(UCanvas* Canvas, int32 Width, int32 Height);

	FMapLocation GetCurrentLocationScaled() const;

	void ResizeCanvas(int32 NewWidth, int32 NewHeight);

private:
	FMapWidgetParams Params;

	FMapLocation CurrentLocation;
	int32 CurrentScale = 3;

	TSharedPtr<SMapCanvasContainer> MapCanvasContainer;
	UMapCanvasRenderTarget2D* RenderCanvas;
	FSlateBrush CanvasBrush;

	mutable bool bRequiresUpdate = false;

	TSharedPtr<IMapProvider> MapProvider;
};