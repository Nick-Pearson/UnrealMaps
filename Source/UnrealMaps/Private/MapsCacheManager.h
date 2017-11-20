#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class IImageWrapper;

class FMapsCacheManager
{
public:

	static TSharedPtr<FMapsCacheManager> Get();

public:
	FMapsCacheManager();
	~FMapsCacheManager();

	bool ExistsInCache(const FString& CacheKey)
	{
		return CacheExistanceMap.Contains(CacheKey);
	}

	UTexture2D* LoadFromCache(const FString& CacheKey);

	void SaveToCache(const FString& CacheKey, TSharedPtr<IImageWrapper> ImageData);

private:

	void Initialise();

	FString GetCachePath() const;
	FORCEINLINE FString GetCachePath(const FString& Filename) const { return GetCachePath() / Filename + GetCacheExtension(); }
	FORCEINLINE FString GetCacheExtension() const { return ".umcache"; }

private:

	TSet<FString> CacheExistanceMap;
};
