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

	//removes cache entries until it fits the requested target
	void ReduceCacheToFit(int32 TargetCacheSize);
private:

	// size of the cache in MB
	double CurrentCacheSize = 0.0f;
	TArray<FString> CacheExistanceMap;
};
