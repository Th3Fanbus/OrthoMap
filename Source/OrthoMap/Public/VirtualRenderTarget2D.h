/* SPDX-License-Identifier: MPL-2.0 */

#pragma once

#include "CoreMinimal.h"
#include "Math/Color.h"
#include "Engine/TextureDefines.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTarget2DArray.h"
#include "Rendering/Texture2DResource.h"
#include "VirtualRenderTarget2D.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVirtualRenderTarget, Log, All);

UCLASS(BlueprintType)
class ORTHOMAP_API UVirtualRenderTarget2D : public UObject
{
	GENERATED_BODY()

protected:
	TArray<FColor> Data;

	UPROPERTY(BlueprintReadOnly)
	int32 SizeX = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SizeY = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 FoWResolution = 1024;

	template<typename T, typename U>
	static FORCEINLINE T PositiveMod(const T& i, const U& n)
	{
		return (i % n + T(n)) % n;
	}
public:
	UVirtualRenderTarget2D();

	FORCEINLINE int32 GetSizeX() const
	{
		return SizeX;
	}

	FORCEINLINE int32 GetSizeY() const
	{
		return SizeY;
	}

	FORCEINLINE int32 At(int32 InX, int32 InY) const
	{
		const int32 x = PositiveMod(InX, SizeX);
		const int32 y = PositiveMod(InY, SizeY);
		return y * SizeX + x;
	}

	FORCEINLINE FColor& Pixel(int32 x, int32 y)
	{
		return Data[At(x, y)];
	}

	FORCEINLINE const FColor& Pixel(int32 x, int32 y) const
	{
		return Data[At(x, y)];
	}

	FORCEINLINE FColor PixelChecked(int32 x, int32 y) const
	{
		if (x >= 0 and y >= 0 and x < SizeX and y < SizeY) {
			return Data[At(x, y)];
		} else {
			return FColor(EForceInit::ForceInit);
		}
	}

	UFUNCTION(BlueprintCallable)
	void SetSize(int32 NewX, int32 NewY);

	UFUNCTION(BlueprintCallable)
	void OverlayRenderTarget2D(UTextureRenderTarget2D* RT2D, int32 AtX, int32 AtY);

	UFUNCTION(BlueprintCallable)
	void DrawSubregionToRenderTarget(UTextureRenderTarget2D* RT2D, FVector2D Offset, FVector2D Scale) const;

	UFUNCTION(BlueprintCallable)
	void ReadIntoRenderTarget(UTextureRenderTarget2D* RT2D, int32 StartX, int32 StartY) const;

private:
	void UpdateTextureRegion(UTextureRenderTarget2D* RT2D, FTextureRenderTarget2DResource* TextureResource, FColor* TmpBuffer) const;
};
