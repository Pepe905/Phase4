// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_Duck.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstanceProxy.h"


void FAnimNode_Duck::UpdateBlendSpace(const FAnimationUpdateContext& Context)
{
	const USkeletalMeshComponent* const SkelMesh = Context.AnimInstanceProxy->GetSkelMeshComponent();
	ACharacter* const Character = (SkelMesh && SkelMesh->GetOwner()) ? Cast<ACharacter>(SkelMesh->GetOwner()) : nullptr;

	if(!Character || !Character->GetWorld()) { return; }
	if (!Character->GetWorld()->IsGameWorld() && !bWorkOutsidePIE)
	{
		return;
	}

	// Cache required vars
	const TArray<AActor*> TraceIgnore { Character };
	const ETraceTypeQuery TraceType = DuckTraceTypeQuery();

	const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const FVector& CharacterLocation = Character->GetActorLocation();
	const FVector CapsuleHeadLocation = CharacterLocation + FVector(0.f, 0.f, CapsuleHalfHeight);

	const FVector HeadLocation = CharacterLocation + FVector(0.f, 0.f, HalfHeight + CapsuleRadius);

	const float MaxDuckDist = (HeadLocation - CapsuleHeadLocation).Size();

	const float AnticipationScale = UKismetMathLibrary::MapRangeClamped(Character->GetVelocity().Size2D(), 0.f, Character->GetCharacterMovement()->GetMaxSpeed(), 0.1f, 1.f);

	// Trace for a ceiling either above or ahead of us (in direction of movement)
	const FVector CeilingTraceStart = HeadLocation - FVector(0.f, 0.f, MaxDuckDist / 2.f);
	const FVector CeilingTraceEnd = (!Character->GetVelocity().GetSafeNormal2D().IsNearlyZero()) ? CeilingTraceStart + Character->GetVelocity().GetSafeNormal2D() * AnticipationDistance * AnticipationScale : CeilingTraceStart + Character->GetActorForwardVector() * AnticipationScale;

	FHitResult CeilingHit(ForceInit);
	UKismetSystemLibrary::CapsuleTraceSingle(
		Character->GetWorld(), 
		CeilingTraceStart, 
		CeilingTraceEnd,
		CapsuleRadius / 2.f,
		MaxDuckDist,
		TraceType,
		false, 
		TraceIgnore,
		EDrawDebugTrace::None,
		CeilingHit,
		false
	);

	// If we hit a potential ceiling
	float Target = 0.f;
	if (CeilingHit.bBlockingHit)
	{
		// Check if there any room for the character under it
		FHitResult WallHit(ForceInit);

		const bool bZeroAccel = Character->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero();
		const bool bZeroVelocity = Character->GetVelocity().IsNearlyZero();
		if (!bZeroAccel || !bZeroVelocity)
		{
			const FVector DirectionOffset = !bZeroAccel ? Character->GetCharacterMovement()->GetCurrentAcceleration() : Character->GetVelocity();
			const FVector VelocityOffset = DirectionOffset.GetSafeNormal() * AnticipationDistance * AnticipationScale;

			const FVector WallTraceStart = CharacterLocation;
			const FVector WallTraceEnd = CharacterLocation + VelocityOffset;

			UKismetSystemLibrary::CapsuleTraceSingle(
				Character->GetWorld(),
				WallTraceStart,
				WallTraceEnd,
				CapsuleRadius * 0.95f,
				CapsuleHalfHeight,
				TraceType,
				false,
				TraceIgnore,
				EDrawDebugTrace::None,
				WallHit,
				false
			);
		}

		if (!WallHit.bBlockingHit)
		{
			// Ceiling trace fits the head, but does not give a great Z impact point (off center), get it from line trace
			FVector CeilingImpactPoint = CeilingHit.ImpactPoint;
			FHitResult PenetrationHit;

			FVector PenetrationTraceStart = CeilingTraceEnd;
			PenetrationTraceStart.Z = CharacterLocation.Z;

			FVector PenetrationTraceEnd = CeilingTraceEnd;
			PenetrationTraceEnd.Z = CharacterLocation.Z + HalfHeight + CapsuleRadius;

			UKismetSystemLibrary::LineTraceSingle(
				Character->GetWorld(),
				PenetrationTraceStart,
				PenetrationTraceEnd, 
				TraceType, 
				false, 
				TraceIgnore, 
				EDrawDebugTrace::None,
				PenetrationHit, 
				false
			);

			if (PenetrationHit.bBlockingHit)
			{
				CeilingImpactPoint = PenetrationHit.ImpactPoint;
			}

			Target = FMath::Min(HalfHeight + CapsuleRadius, CeilingImpactPoint.Z - CharacterLocation.Z);
		}
	}

	if (Target != 0.f)
	{
		Target = UKismetMathLibrary::MapRangeClamped(Target, CapsuleHalfHeight, HalfHeight + CapsuleRadius, 0.f, 1.f);
		Target = 1.f - Target;
	}

	X = FMath::FInterpConstantTo(X, Target, Context.GetDeltaTime(), (X < Target ? DuckRate : ResetRate));
}
