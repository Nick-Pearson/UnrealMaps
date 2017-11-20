#include "MapCanvasRenderTarget2D.h"

void UMapCanvasRenderTarget2D::ForwardFunction(UCanvas* Canvas, int32 Width, int32 Height)
{
	PassthroughEvent.Broadcast(Canvas, Width, Height);
}