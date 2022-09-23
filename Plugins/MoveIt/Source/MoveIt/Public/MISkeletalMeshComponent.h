// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "MISkeletalMeshComponent.generated.h"

class UMIAnimInstance;
class AMICharacter;

/**
 * Skeletal mesh that applies MoveIt's TurnInPlace
 */
UCLASS()
class MOVEIT_API UMISkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Valid on server only for updating replicated value */
	UPROPERTY(transient, NonTransactional)
	AMICharacter* ServerCharacterOwner;

	/** The active animation graph program instance. */
	UPROPERTY(transient, NonTransactional)
	UMIAnimInstance* MIAnimScriptInstance;

	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, Category = Animation)
	bool bEnableTurnInPlaceOutsidePIE;

	/** Cached RootYawOffset (so that it can be removed) */
	float RootYawOffset;

	FORCEINLINE FRotator GetRootOffset() const { return FRotator(0.f, RootYawOffset, 0.f); }
	FORCEINLINE FRotator GetRootOffsetInverse() const { return GetRootOffset().GetInverse(); }

public:
	UMISkeletalMeshComponent(const FObjectInitializer& OI);
	
	virtual void InitAnim(bool bForceReinit) override;
	void InitRootYawFromReplication(float ReplicatedRootYawOffset);
	virtual void TickPose(float DeltaTime, bool bNeedsValidRootMotion) override;

	void SetRootYawOffset(float NewRootYawOffset);
};
