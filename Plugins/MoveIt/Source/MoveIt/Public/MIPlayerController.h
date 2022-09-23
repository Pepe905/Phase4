// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MIPlayerController.generated.h"

/**
 * Exposes SetPawn event to Blueprint for use with MIPlayerCameraManager
 */
UCLASS()
class MOVEIT_API AMIPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void SetPawn(APawn* InPawn) override;
	
	/** Called on all net roles that have a PlayerController whenever Pawn is changed */
	UFUNCTION(BlueprintImplementableEvent, Category = PlayerController, meta = (DisplayName = "Set Pawn"))
	void K2_SetPawn(APawn* InPawn);
};
