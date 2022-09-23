// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MITurnInPlaceTools.h"
#include "EditorUtilityLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Subsystems/AssetEditorSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimTools, Log, All);

FTransform UMITurnInPlaceTools::ExtractRootMotion(float Time, UAnimSequence* Anim)
{
	return Anim->ExtractRootMotion(0.f, Time, false);
}

void UMITurnInPlaceTools::MarkDirty(UObject* DirtyObject)
{
	if (DirtyObject && DirtyObject->IsValidLowLevel())
	{
		DirtyObject->MarkPackageDirty();
	}
}

void UMITurnInPlaceTools::ForceCloseAsset(UObject* Asset)
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->CloseAllEditorsForAsset(Asset);
}
