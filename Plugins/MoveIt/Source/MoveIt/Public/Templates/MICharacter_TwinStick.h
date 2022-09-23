// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MICharacter.h"
#include "MICharacter_TwinStick.generated.h"

class AController;
class APlayerController;

/**
 * 
 */
UCLASS()
class MOVEIT_API AMICharacter_TwinStick : public AMICharacter
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = Character)
	APlayerController* PlayerController;

public:
	AMICharacter_TwinStick(const FObjectInitializer& OI);

	virtual void OnReceiveController(AController* NewController);

	/** Equivalent to SetInputModeGameOnly but uses SetConsumeCaptureMouseDown(false) to prevent blocking mouse click events */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Character)
	void SetCustomGameInputMode();

	/** Called when receiving a controller on both server & local client */
	UFUNCTION(BlueprintImplementableEvent, Category = Character, meta = (DisplayName = "On Receive Controller"))
	void K2_OnReceiveController(AController* NewController);

	/** Called when receiving a player controller on both server & local client */
	UFUNCTION(BlueprintImplementableEvent, Category = Character, meta = (DisplayName = "On Receive Player Controller"))
	void K2_OnReceivePlayerController(APlayerController* NewPlayerController);

	virtual void OnRep_Controller() override;
	virtual void PossessedBy(AController* NewController) override;

	/** Call this from your input event for rotating the character via mouse */
	UFUNCTION(BlueprintCallable, Category = Character)
	void ReceiveMouseTurnInput(float AxisValue);

	/** Call this from your input event for rotating the character via gamepad */
	UFUNCTION(BlueprintCallable, Category = Character)
	void ReceiveGamepadTurnInput(FVector2D AxisValues);

	/**
	 * Override if you want to attempt a line trace or similar to get different results
	 * @return True if NewRotation is valid
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = Character)
	bool GetMouseFacingRotation(FVector MouseWorldLocation, FVector MouseWorldDirection, FRotator& NewRotation) const;

	/**
	 * @return True if NewRotation is valid
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = Character)
	bool GetGamepadFacingRotation(FVector WorldDirection, FRotator& NewRotation) const;

	/**
	 * Override to add interpolation
	 * Sets the control rotation to the final computed result
	 */
	UFUNCTION(BlueprintNativeEvent, Category = Character)
	void UpdateControlRotation(const FRotator& NewControlRotation);
};
