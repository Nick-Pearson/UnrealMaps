#include "UnrealMapsHelperFunctions.h"
#include "ModuleManager.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"

UTexture2D* UnrealMapsHelperFunctions::CreateTexture2DFromBytes(const TArray<uint8>& inBytes, EImageFormat Format, TSharedPtr<IImageWrapper>& ImageWrapper)
{
	ImageWrapper = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper").CreateImageWrapper(Format);

	if (!ImageWrapper.IsValid() || inBytes.Num() == 0)
		return nullptr;

	// Load the raw HTML bytes into an image wrapper
	ImageWrapper->SetCompressed(inBytes.GetData(), inBytes.Num());

	const TArray<uint8>* DataPtr = nullptr;
	ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, DataPtr);

	if (!DataPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to decompress image"));
		return nullptr;
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
	return NewTexture;
}
