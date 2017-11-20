#pragma once

#include "CoreMinimal.h"
#include "IImageWrapper.h"

namespace UnrealMapsHelperFunctions
{
	UTexture2D* CreateTexture2DFromBytes(const TArray<uint8>& inBytes, EImageFormat Format, TSharedPtr<IImageWrapper>& ImageWrapper);
	FORCEINLINE UTexture2D* CreateTexture2DFromBytes(const TArray<uint8>& inBytes, EImageFormat Format)
	{
		TSharedPtr<IImageWrapper> ImageWrapper;
		return CreateTexture2DFromBytes(inBytes, Format, ImageWrapper);
	}
}