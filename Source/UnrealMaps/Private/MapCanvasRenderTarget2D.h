#pragma once

#include "CoreMinimal.h"
#include "Engine/CanvasRenderTarget2D.h"

#include "MapCanvasRenderTarget2D.generated.h"

class UCanvas;

// This passthrough class is required as binding shared pointers to a non-uobject class is not supported
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCanvasRenderTargetUpdateNonDynamic, UCanvas* /*Canvas*/, int32 /*Width*/, int32 /*Height*/);

UCLASS()
class UMapCanvasRenderTarget2D : public UCanvasRenderTarget2D
{
	GENERATED_BODY()

	UMapCanvasRenderTarget2D()
	{
		OnCanvasRenderTargetUpdate.AddDynamic(this, &UMapCanvasRenderTarget2D::ForwardFunction);
	}

	UFUNCTION()
	void ForwardFunction(UCanvas* Canvas, int32 Width, int32 Height);

public:

	FOnCanvasRenderTargetUpdateNonDynamic PassthroughEvent;
};
