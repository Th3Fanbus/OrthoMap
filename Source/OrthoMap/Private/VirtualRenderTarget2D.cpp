/* SPDX-License-Identifier: MPL-2.0 */

#include "VirtualRenderTarget2D.h"
#include "Logging/StructuredLog.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "FGMapManager.h"

DEFINE_LOG_CATEGORY(LogVirtualRenderTarget);

static TAutoConsoleVariable<float> CVarOrthoMapTickRate(
	TEXT("OrthoMap.TickRate"),
	1.0f,
	TEXT("Tick rate of the OrthoMap subsystem"));

UVirtualRenderTarget2D::UVirtualRenderTarget2D()
{
}

void UVirtualRenderTarget2D::SetSize(int32 NewX, int32 NewY)
{
	SizeX = NewX;
	SizeY = NewY;
	Data.Reset(SizeX * SizeY);
	Data.SetNum(SizeX * SizeY);
	if (AFGMapManager* MapManager = AFGMapManager::Get(GWorld)) {
		FoWResolution = MapManager->mFogOfWarResolution * 2;
		UE_LOGFMT(LogVirtualRenderTarget, Log, "FoW resolution: {0}", FoWResolution);
	}
}

void UVirtualRenderTarget2D::OverlayRenderTarget2D(UTextureRenderTarget2D* RT2D, int32 AtX, int32 AtY)
{
	if (not RT2D) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D is nullptr");
		return;
	}
	if (RT2D->GetFormat() != PF_B8G8R8A8) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D pixel format {0} is unsupported", GetPixelFormatString(RT2D->GetFormat()));
		return;
	}
	//FRenderTarget* RenderTarget = RT2D->GameThread_GetRenderTargetResource();
	FTextureRenderTarget2DResource* TextureResource = reinterpret_cast<FTextureRenderTarget2DResource*>(RT2D->GetResource());
	if (not TextureResource) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D has no resource");
		return;
	}

	const int32 Padding = 4;
	const int32 MinSize = Padding + Padding;

	const int32 LenX = RT2D->SizeX;
	const int32 LenY = RT2D->SizeY;

	// Let's assume that a too large RT2D would cause other problems first
	if (LenX <= MinSize or LenY <= MinSize) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D is too small ({0}x{1})", LenX, LenY);
		return;
	}

	// Read the render target surface data back.
	struct FReadSurfaceContext
	{
		FTextureRenderTarget2DResource* SrcRenderTarget;
		TArray<FColor> RTData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};

	TSharedRef<FReadSurfaceContext> Context = MakeShared<FReadSurfaceContext>();
	Context->SrcRenderTarget = TextureResource;
	Context->Rect = FIntRect(0, 0, LenX, LenY);
	Context->Flags = FReadSurfaceDataFlags();

	ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand)([this, LenX, LenY, AtX, AtY, Context](FRHICommandListImmediate& RHICmdList) {
		RHICmdList.ReadSurfaceData(
			Context->SrcRenderTarget->GetRenderTargetTexture(),
			Context->Rect,
			Context->RTData,
			Context->Flags
		);

		if (Context->RTData.Num() != Context->Rect.Area()) {
			UE_LOGFMT(LogVirtualRenderTarget, Error, "Could not read surface data: {0} != {1}", Context->RTData.Num(), Context->Rect.Area());
			return;
		}

		AsyncTask(ENamedThreads::AnyThread, [this, Context, LenX, LenY, AtX, AtY]() {
#if 0
			const int32 LowerX = FMath::Max(AtX + Padding, 0);
			const int32 LowerY = FMath::Max(AtY + Padding, 0);
			const int32 UpperX = FMath::Min(AtX - Padding + LenX, SizeX);
			const int32 UpperY = FMath::Min(AtY - Padding + LenY, SizeY);
			for (size_t y = Padding; y < LenY - Padding; y++) {
				for (size_t x = Padding; x < LenX - Padding; x++) {
					Pixel(AtX + x, AtY + y) = Context->RTData[y * LenX + x];
				}
			}
#else
			const int32 LowerX = FMath::Max(-AtX, Padding);
			const int32 LowerY = FMath::Max(-AtY, Padding);
			const int32 UpperX = FMath::Min(-AtX + SizeX, LenX - Padding);
			const int32 UpperY = FMath::Min(-AtY + SizeY, LenY - Padding);
			for (size_t y = LowerY; y < UpperY; y++) {
				for (size_t x = LowerX; x < UpperX; x++) {
					Pixel(AtX + x, AtY + y) = Context->RTData[y * LenX + x];
				}
			}
#endif
		});
	});
}

void UVirtualRenderTarget2D::DrawSubregionToRenderTarget(UTextureRenderTarget2D* RT2D, FVector2D Offset, FVector2D Scale) const
{
	if (not RT2D) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D is nullptr");
		return;
	}
	if (GPixelFormats[RT2D->GetFormat()].BlockBytes != Data.GetTypeSize()) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D pixel format {0} is unsupported", GetPixelFormatString(RT2D->GetFormat()));
		return;
	}
	FTextureRenderTarget2DResource* TextureResource = reinterpret_cast<FTextureRenderTarget2DResource*>(RT2D->GetResource());
	if (not TextureResource) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D has no resource");
		return;
	}
	AsyncTask(ENamedThreads::AnyThread, [this, RT2D, Offset, Scale, TextureResource]() {
		const SIZE_T DataSize = (SIZE_T)RT2D->SizeX * RT2D->SizeY * Data.GetTypeSize();
		FColor* TmpBuffer = reinterpret_cast<FColor*>(FMemory::Malloc(DataSize));
		if (not TmpBuffer) {
			UE_LOGFMT(LogVirtualRenderTarget, Error, "OUT OF MEMORY");
			return;
		}
		FMemory::Memzero(TmpBuffer, DataSize);

		const FVector2D FogOfWarScale = FVector2D(FoWResolution);
		const FVector2D VirtualScale = FVector2D(SizeX, SizeY) / FogOfWarScale;

		const FVector2D TargetExtents = FVector2D(RT2D->SizeX, RT2D->SizeY);
		const FVector2D StarterCorner = (FogOfWarScale - TargetExtents / Scale) / 2 - Offset;

#if 1
		for (size_t y = 0; y < RT2D->SizeY; y++) {
			for (size_t x = 0; x < RT2D->SizeX; x++) {
				const FVector2D SampleOffset = (StarterCorner + FVector2D(x, y) / Scale) * VirtualScale;
				TmpBuffer[y * RT2D->SizeX + x] = PixelChecked(SampleOffset.X, SampleOffset.Y);
			}
		}
#else
		//const FVector2D BazNah = (StarterCorner + FVector2D(x, y) / Scale) * VirtualScale;

		//const FVector2D INPUT = (FooBar / VirtualScale) - StarterCorner * Scale;

		//const FVector2D LowerCorner = FVector2D::Max((StarterCorner + FVector2D(0) / Scale) * VirtualScale, FVector2D(0));
		//const FVector2D UpperCorner = FVector2D::Min((StarterCorner + TargetExtents / Scale) * VirtualScale, FVector2D(SizeX, SizeY));

		const FVector2D LowerCorner = FVector2D::Max((FVector2D(0) / VirtualScale) - StarterCorner * Scale, FVector2D(0));
		const FVector2D UpperCorner = FVector2D::Min((FVector2D(SizeX, SizeY) / VirtualScale) - StarterCorner * Scale, TargetExtents);

		for (size_t y = LowerCorner.Y; y < UpperCorner.Y; y++) {
			for (size_t x = LowerCorner.X; x < UpperCorner.X; x++) {
				const FVector2D SampleOffset = (StarterCorner + FVector2D(x, y) / Scale) * VirtualScale;
				TmpBuffer[y * RT2D->SizeX + x] = Pixel(SampleOffset.X, SampleOffset.Y);//Data[SampleOffset.Y * SizeX + SampleOffset.X];
			}
		}
#endif

		UpdateTextureRegion(RT2D, TextureResource, TmpBuffer);
	});
}

void UVirtualRenderTarget2D::ReadIntoRenderTarget(UTextureRenderTarget2D* RT2D, int32 StartX, int32 StartY) const
{
	if (not RT2D) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D is nullptr");
		return;
	}
	if (GPixelFormats[RT2D->GetFormat()].BlockBytes != Data.GetTypeSize()) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D pixel format {0} is unsupported", GetPixelFormatString(RT2D->GetFormat()));
		return;
	}
	FTextureRenderTarget2DResource* TextureResource = reinterpret_cast<FTextureRenderTarget2DResource*>(RT2D->GetResource());
	if (not TextureResource) {
		UE_LOGFMT(LogVirtualRenderTarget, Error, "RT2D has no resource");
		return;
	}
	AsyncTask(ENamedThreads::AnyThread, [this, RT2D, StartX, StartY, TextureResource]() {
		const SIZE_T DataSize = (SIZE_T)RT2D->SizeX * RT2D->SizeY * Data.GetTypeSize();
		FColor* TmpBuffer = reinterpret_cast<FColor*>(FMemory::Malloc(DataSize));
		if (not TmpBuffer) {
			UE_LOGFMT(LogVirtualRenderTarget, Error, "OUT OF MEMORY");
			return;
		}
		FMemory::Memzero(TmpBuffer, DataSize);

		for (size_t y = 0; y < RT2D->SizeY; y++) {
			for (size_t x = 0; x < RT2D->SizeX; x++) {
				TmpBuffer[y * RT2D->SizeX + x].Bits = Pixel(x + StartX, y + StartY).Bits ^ FColor(0, 0, 0).Bits;
			}
		}

		UpdateTextureRegion(RT2D, TextureResource, TmpBuffer);
	});
}

void UVirtualRenderTarget2D::UpdateTextureRegion(UTextureRenderTarget2D* RT2D, FTextureRenderTarget2DResource* TextureResource, FColor* TmpBuffer) const
{
	const uint32 SrcBpp = Data.GetTypeSize();
	const uint32 SrcPitch = RT2D->SizeX * SrcBpp;
	uint8* SrcData = reinterpret_cast<uint8*>(TmpBuffer);

	const FUpdateTextureRegion2D Region = FUpdateTextureRegion2D(0, 0, 0, 0, RT2D->SizeX, RT2D->SizeY);

	struct FUpdateTextureRegionsData
	{
		FTextureRenderTarget2DResource* TextureResource;
		FUpdateTextureRegion2D Region;
		uint32 SrcPitch;
		uint32 SrcBpp;
		uint8* SrcData;
	};

	FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

	RegionData->TextureResource = TextureResource;
	RegionData->Region = Region;
	RegionData->SrcPitch = SrcPitch;
	RegionData->SrcBpp = SrcBpp;
	RegionData->SrcData = SrcData;

	ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)([RegionData](FRHICommandListImmediate& RHICmdList) {
		// Some RHIs don't support source offsets. Offset source data pointer now and clear source offsets
		const uint8* RegionSourceData = RegionData->SrcData
			+ RegionData->Region.SrcY * RegionData->SrcPitch
			+ RegionData->Region.SrcX * RegionData->SrcBpp;
		RegionData->Region.SrcX = 0;
		RegionData->Region.SrcY = 0;

		RHIUpdateTexture2D(
			RegionData->TextureResource->TextureRHI,
			0,
			RegionData->Region,
			RegionData->SrcPitch,
			RegionSourceData);

		// The deletion of source data may need to be deferred to the RHI thread after the updates occur
		RHICmdList.EnqueueLambda([RegionData](FRHICommandList&) {
			FMemory::Free(RegionData->SrcData);
			delete RegionData;
		});
	});
}
