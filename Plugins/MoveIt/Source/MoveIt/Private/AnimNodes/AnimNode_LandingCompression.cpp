// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_LandingCompression.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"


void FAnimNode_LandingCompression::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_SpringBase::Initialize_AnyThread(Context);

	const USkeletalMeshComponent* const SkelMesh = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Character = (SkelMesh && SkelMesh->GetOwner()) ? Cast<ACharacter>(SkelMesh->GetOwner()) : nullptr;
}

void FAnimNode_LandingCompression::PreUpdate(const UAnimInstance* InAnimInstance)
{
	const USkeletalMeshComponent* const SkelMesh = InAnimInstance->GetSkelMeshComponent();

	if (!SkelMesh || !Character || !Character->GetCharacterMovement() || !Character->GetWorld())
	{
		return;
	}

	if (!Character->GetWorld()->IsGameWorld() && !bWorkOutsidePIE) { return; }

	FTimerManager& TimerManager = Character->GetWorldTimerManager();

	const bool bJustSpawned = Character->GetWorld()->GetTimeSeconds() < 2.f;

	const bool bWasFalling = bFalling;
	bFalling = Character->GetCharacterMovement()->IsFalling();

	const FVector V = ImpactVelocity * ImpactVelocityScale;

	const bool bJustLanded = !bFalling && bWasFalling && V.Z < -VelocityZThreshold && !bJustSpawned;

	if (bJustLanded)
	{
		const float Duration = UKismetMathLibrary::MapRangeClamped(-V.Z, 0.f, VelocityForMaxLandTime, MinLandTime, MaxLandTime);

		if (Duration > 0.f)
		{
			Velocity = V;
			Velocity = Velocity.GetClampedToMaxSize(MaxImpactVelocity);

			RemainingTime = 0.f;
			BoneVelocity = FVector::ZeroVector;
			bAlphaBoolEnabled = true;

			auto OnDisable = ([&]()
				{
					bAlphaBoolEnabled = false;
				});
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindLambda(OnDisable);
			TimerManager.SetTimer(DisableTimerHandle, TimerDelegate, Duration, false);
		}
	}
	else if (bFalling && Character->GetWorldTimerManager().IsTimerActive(DisableTimerHandle))
	{
		TimerManager.ClearTimer(DisableTimerHandle);
		bAlphaBoolEnabled = false;
	}

	FAnimNode_SpringBase::PreUpdate(InAnimInstance);
}
