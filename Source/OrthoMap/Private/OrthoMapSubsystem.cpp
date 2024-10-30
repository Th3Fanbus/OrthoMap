/* SPDX-License-Identifier: MPL-2.0 */

#include "OrthoMapSubsystem.h"
#include "Kismet/GameplayStatics.h"

AOrthoMapSubsystem* AOrthoMapSubsystem::Get(UObject* WorldContext)
{
	return Cast<AOrthoMapSubsystem>(UGameplayStatics::GetActorOfClass(WorldContext, AOrthoMapSubsystem::StaticClass()));
}
