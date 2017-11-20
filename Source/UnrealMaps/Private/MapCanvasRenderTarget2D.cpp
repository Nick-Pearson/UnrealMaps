#include "MapCanvasRenderTarget2D.h"

void UMapCanvasRenderTarget2D::ForwardFunction(UCanvas* Canvas, int32 Width, int32 Height)
{
	PassthroughEvent.Broadcast(Canvas, Width, Height);
}

int32 SMapCanvasContainer::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FVector2D Size = AllottedGeometry.GetAbsoluteSize();
	FIntPoint NewRes = FIntPoint(FMath::FloorToInt(Size.X), FMath::FloorToInt(Size.Y));

	if (NewRes != CurrentRes)
	{
		ContainerResizedEvent.Broadcast(NewRes.X, NewRes.Y);
		CurrentRes = NewRes;
	}

	return SImage::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}
