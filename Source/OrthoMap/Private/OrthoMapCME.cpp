/* SPDX-License-Identifier: MPL-2.0 */

#include "OrthoMapCME.h"
#include "OrthoMap.h"
#include "OrthoMapSubsystem.h"
#include "FGCheatManager.h"
#include "Logging/StructuredLog.h"
#include "Kismet/KismetRenderingLibrary.h"

UOrthoMapCME::UOrthoMapCME()
{
	UE_LOGFMT(LogOrthoMapCpp, Display, "Hello World {0}", GetPathName());
	if (HasAnyFlags(RF_ClassDefaultObject)) {
		UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda([](UCheatManager* CheatManager) {
			UE_LOGFMT(LogOrthoMapCpp, Display, "Registering OrthoMapCME into CheatManager");
			CheatManager->AddCheatManagerExtension(NewObject<UOrthoMapCME>(CheatManager));
		}));
	}
}

void UOrthoMapCME::DumpVRT2D()
{
	UE_LOGFMT(LogOrthoMapCpp, Warning, "RUNNING!");

	const int32 RTSize = 4096;

	AOrthoMapSubsystem* Subsystem = AOrthoMapSubsystem::Get(this);
	if (not IsValid(Subsystem) or not IsValid(Subsystem->VRT2D)) {
		return;
	}

	UTextureRenderTarget2D* RT2D = UKismetRenderingLibrary::CreateRenderTarget2D(Subsystem, RTSize, RTSize, RTF_RGBA8, FLinearColor::Black, false, true);
	if (not IsValid(RT2D)) {
		return;
	}

	const FString Timestamp = FDateTime::UtcNow().ToFormattedString(TEXT("%Y.%m.%d-%H.%M.%S"));
	const FString Folder = FPaths::ProjectSavedDir() / TEXT("VRT2D") / Timestamp;

	const int32 SizeX = Subsystem->VRT2D->GetSizeX();
	const int32 SizeY = Subsystem->VRT2D->GetSizeY();

	UE_LOGFMT(LogOrthoMapCpp, Warning, "Saving {0}x{1} VRT2D contents to {2}", SizeX, SizeY, Folder);

	for (int32 y = 0; y < SizeY; y += RTSize) {
		for (int32 x = 0; x < SizeX; x += RTSize) {
			Subsystem->VRT2D->ReadIntoRenderTarget(RT2D, x, y);
			const FString Filename = FString::Printf(TEXT("%u_%u.png"), x / RTSize, y / RTSize);
			UE_LOGFMT(LogOrthoMapCpp, Warning, "Saving {0}", Filename);
			UKismetRenderingLibrary::ExportRenderTarget(Subsystem, RT2D, Folder, Filename);
		}
	}
}
