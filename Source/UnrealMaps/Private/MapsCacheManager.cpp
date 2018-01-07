#include "MapsCacheManager.h"
#include "UnrealMapsSettings.h"

#include "Paths.h"
#include "FileManager.h"
#include "ModuleManager.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "UnrealMapsHelperFunctions.h"

TSharedPtr<FMapsCacheManager> FMapsCacheManager::Get()
{
	static TWeakPtr<FMapsCacheManager> StaticThis;

	if (StaticThis.IsValid())
		return StaticThis.Pin();

	TSharedPtr<FMapsCacheManager> NewThis(new FMapsCacheManager);
	StaticThis = NewThis;
	return NewThis;
}

FMapsCacheManager::FMapsCacheManager()
{
	Initialise();
}

FMapsCacheManager::~FMapsCacheManager()
{
}

UTexture2D* FMapsCacheManager::LoadFromCache(const FString& CacheKey)
{
	const UUnrealMapsSettings* Settings = GetDefault<UUnrealMapsSettings>();
	if (!ExistsInCache(CacheKey) || !Settings || !Settings->bUseCache)
		return nullptr;

	FString CacheFilename = GetCachePath(CacheKey);
	FArchive* FileReader = IFileManager::Get().CreateFileReader(*CacheFilename);

	TArray<uint8> FileBytes;
	FileBytes.AddUninitialized(FileReader->TotalSize());

	FileReader->Serialize(FileBytes.GetData(), FileBytes.Num());
	FileReader->Close();

	UTexture2D* LoadedTexture = UnrealMapsHelperFunctions::CreateTexture2DFromBytes(FileBytes, EImageFormat::PNG);

	if (!LoadedTexture)
	{
		CacheExistanceMap.Remove(CacheKey);
	}

	return LoadedTexture;
}

void FMapsCacheManager::SaveToCache(const FString& CacheKey, TSharedPtr<IImageWrapper> ImageData)
{
	const UUnrealMapsSettings* Settings = GetDefault<UUnrealMapsSettings>();
	if (!Settings || !Settings->bUseCache)
		return;

	FString CacheFilename = GetCachePath(CacheKey);
	FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*CacheFilename);

	const TArray<uint8> DataPtr = ImageData->GetCompressed();

	CurrentCacheSize += DataPtr.Num() / 1048576.0;

	FileWriter->Serialize((void*)DataPtr.GetData(), DataPtr.Num());
	FileWriter->Close();

	//remove any entries that put us over the limit
	ReduceCacheToFit(Settings->MaxCacheSize);

	CacheExistanceMap.Add(CacheKey);
}

void FMapsCacheManager::Initialise()
{
	FString CacheDir = GetCachePath();
	if (!IFileManager::Get().DirectoryExists(*CacheDir))
	{
		IFileManager::Get().MakeDirectory(*CacheDir);
		return;
	}

	//discover cached images
	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *CacheDir);

	CurrentCacheSize = 0.0;
	for (const FString& FoundFile : FoundFiles)
	{
		//chop off the .umcache extension
		CacheExistanceMap.Add(FoundFile.LeftChop(GetCacheExtension().Len()));

		int64 FileSize = IFileManager::Get().FileSize(*(CacheDir / FoundFile));
		//convert bytes to megabytes
		CurrentCacheSize += FileSize / 1048576.0;
	}

	UE_LOG(UnrealMaps, Log, TEXT("[MapsCacheManager] Initialised cache, current cache size %.2f MB"), CurrentCacheSize);
}

FString FMapsCacheManager::GetCachePath() const
{
	return FPaths::ProjectSavedDir() / "UnrealMaps" / "CachedImages";
}

void FMapsCacheManager::ReduceCacheToFit(int32 TargetCacheSize)
{
	//delete random entries until we are at the correct size
	while (CacheExistanceMap.Num() > 0 && CurrentCacheSize > TargetCacheSize)
	{
		FString SelectedEntry = CacheExistanceMap[FMath::RandHelper(CacheExistanceMap.Num() - 1)];
		
		int64 FileSize = IFileManager::Get().FileSize(*GetCachePath(SelectedEntry));
		//convert bytes to megabytes
		CurrentCacheSize -= FileSize / 1048576.0;

		IFileManager::Get().Delete(*GetCachePath(SelectedEntry));
		CacheExistanceMap.Remove(SelectedEntry);
	}
}
