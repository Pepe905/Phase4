// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MIPlayerCameraManager.generated.h"

class UMIAnimInstance;
class AMICharacter;

/**
 * Optionally used to clamp camera angle to turn in place limit while root motion is playing to prevent teleport after it ends
 */
UCLASS()
class MOVEIT_API AMIPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PlayerCameraManager)
	bool bClampYawDuringRootMotion;

protected:
	UPROPERTY()
	bool bInitialized;

	UPROPERTY()
	float DefaultViewYawMin;

	UPROPERTY()
	float DefaultViewYawMax;

	UPROPERTY()
	UMIAnimInstance* CurrentAnimInstance;

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = PlayerCameraManager)
	AMICharacter* MICharacter;

public:
	AMIPlayerCameraManager()
		: bClampYawDuringRootMotion(true)
		, bInitialized(false)
		, DefaultViewYawMin(0.f)
		, DefaultViewYawMax(0.f)
	{}

	virtual void BeginPlay() override;
	virtual void InitializeFor(class APlayerController* PC) override;

	/** Cache default limits so they can be restored later */
	void InitDefaults();

	/**
	 * Call this if anim instance or player controller's pawn is changed 
	 * @see : AMIPlayerController::SetPawn()
	 * @see : AMICharacter::OnAnimInstanceChanged()
	 */
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	void OnCharacterUpdated();

	/** 
	 * Use this to unbind from the anim instance, will cease functionality until OnCharacterUpdated() is called again 
	 * This is not intended for enable / disable however, bClampYawDuringRootMotion does that
	 * and during possession changes OnCharacterUpdated() will do this anyway
	 */
	UFUNCTION(BlueprintCallable, Category = PlayerCameraManager)
	void ClearAnimInstance();

	/** Bound function for montage starting */
	UFUNCTION()
	void OnStartMontage(UAnimMontage* Montage);

	/** Bound function for montage stopping */
	UFUNCTION()
	void OnStopMontage(UAnimMontage* Montage, bool bInterrupted);

	bool HasValidData() const;

	/** For optional usage if pawn not properly set - not used by default */
	UFUNCTION(BlueprintNativeEvent, Category = PlayerCameraManager)
	void OnFailSanityCheck();
};
