/* SPDX-License-Identifier: MPL-2.0 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OrthoMapBPFL.generated.h"

UCLASS()
class ORTHOMAP_API UOrthoMapBPFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure)
	static FVector2D QuantizeVector2D(FVector2D InVec, double QuantStep);
};
