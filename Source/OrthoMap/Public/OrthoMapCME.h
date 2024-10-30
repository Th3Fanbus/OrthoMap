/* SPDX-License-Identifier: MPL-2.0 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "OrthoMapCME.generated.h"

UCLASS(MinimalAPI)
class UOrthoMapCME: public UCheatManagerExtension
{
	GENERATED_BODY()
public:
    UOrthoMapCME();

    UFUNCTION(Exec, CheatBoard, Category = "Log")
    void DumpVRT2D();
};
