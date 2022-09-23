// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blutility/Classes/EditorUtilityWidget.h"
#include "MITurnInPlaceTools.generated.h"

/**
 * 
 */
UCLASS()
class MOVEITEDITORTOOLS_API UMITurnInPlaceTools : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Development|Editor")
	FTransform ExtractRootMotion(float Time, UAnimSequence* Anim);

	UFUNCTION(BlueprintCallable, Category = "Development|Editor")
	void MarkDirty(UObject* DirtyObject);

	UFUNCTION(BlueprintCallable, Category = "Development|Editor")
	void ForceCloseAsset(UObject* Asset);
};
