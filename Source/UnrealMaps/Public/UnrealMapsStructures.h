#pragma once

enum class UNREALMAPS_API EMapDisplayType : uint8
{
	Roadmap,
	Satellite,
	Hybrid,
	Terrain,

	MAX
};

struct UNREALMAPS_API FMapLocation
{
	FMapLocation(float inLat = 0.0f, float inLong = 0.0f) :
		Lat(inLat), Long(inLong)
	{}

	float Lat, Long; 
	
	bool operator==(const FMapLocation& other) const
	{
		return FMath::IsNearlyEqual(Lat, other.Lat) && FMath::IsNearlyEqual(Long, other.Long);
	}

	void operator+=(const FMapLocation& other)
	{
		Lat += other.Lat;
		Long += other.Long;
		Lat = FMath::Fmod(Lat, 360.0f);
		Long = FMath::Fmod(Long, 360.0f);
	}

	FMapLocation operator+(const FMapLocation& other)
	{
		FMapLocation RtnVal = *this;
		RtnVal += other;
		return RtnVal;
	}
};

struct UNREALMAPS_API FMapWidgetParams
{
	EMapDisplayType DisplayType = EMapDisplayType::Roadmap;
};