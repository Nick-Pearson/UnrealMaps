#pragma once

#include "SCompoundWidget.h"

struct FMapLocation;

class UNREALMAPS_API IMapWidget : public SCompoundWidget
{
public:

	virtual void SetLocation(const FMapLocation& Location) PURE_VIRTUAL(SetLocation, );
	virtual void GetLocation(FMapLocation& Location) const PURE_VIRTUAL(GetLocation, );

	virtual void SetMapScale(int32 NewScale) PURE_VIRTUAL(SetMapScale, );
	virtual int32 GetMapScale() const PURE_VIRTUAL(GetMapScale, return 1;);
};
