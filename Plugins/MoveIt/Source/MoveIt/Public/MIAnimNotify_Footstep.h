// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "MIAnimNotify_Footstep.generated.h"

class UParticleSystem;

/**
 * Anim notify to use for playing footsteps. MoveIt does not use this by default, it is here to force footsteps from animations if required.
 */
UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Footstep"))
class MOVEIT_API UMIAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	/** Socket to play footstep from - REQUIRED */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FName SocketName;
	
#if WITH_EDITORONLY_DATA
	// Particle System to Spawn when setting up anim notify
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(DisplayName="Preview Particle System"))
	UParticleSystem* PreviewPSTemplate;

	// Sound to Play when setting up anim notify
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	USoundBase* PreviewSound;
#endif

	// Volume Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	float VolumeMultiplier;

	// Pitch Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	float PitchMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	bool bEnableOutsideGameWorld;

public:
	UMIAnimNotify_Footstep()
		: VolumeMultiplier(1.f)
		, PitchMultiplier(1.f)
	{}

	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif
	// End UAnimNotify interface
};
