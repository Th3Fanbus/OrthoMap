/* SPDX-License-Identifier: MPL-2.0 */

#pragma once

#include "CoreMinimal.h"
#include "VirtualRenderTarget2D.h"
#include "Subsystem/ModSubsystem.h"
#include "OrthoMapSubsystem.generated.h"

UCLASS(Abstract)
class ORTHOMAP_API AOrthoMapSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	static AOrthoMapSubsystem* Get(UObject* WorldContext);

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UVirtualRenderTarget2D> VRT2D;
};
