// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MIAnimInstance.h"
#include "MICharacter.h"
#include "MICharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MIPhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"
#include "MIAnimInstanceProxy.h"
#include "Niagara/Public/NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"


FAnimInstanceProxy* UMIAnimInstance::CreateAnimInstanceProxy()
{
	return new FMIAnimInstanceProxy(this);
}

void UMIAnimInstance::NativeInitializeAnimation()
{
	Character = (TryGetPawnOwner()) ? Cast<AMICharacter>(TryGetPawnOwner()) : nullptr;
	if (Character)
	{
		Movement = Character->GetMICharacterMovement();
		Mesh = Character->GetMesh();

		if (Mesh && Character->GetNetMode() != NM_DedicatedServer)
		{
			if (SurfaceImpactDelegateHandle.IsValid())
			{
				Mesh->UnregisterOnBoneTransformsFinalizedDelegate(SurfaceImpactDelegateHandle);
				SurfaceImpactDelegateHandle.Reset();
			}
			
			SurfaceImpactDelegateHandle = Mesh->RegisterOnBoneTransformsFinalizedDelegate(
				FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(
					this, &UMIAnimInstance::ComputeSurfaceImpacts));
		}

		RootYawOffset = Character->GetActorRotation().Yaw;
		bResetTurn = true;
	}
}

void UMIAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	if (!IsValidToEvaluate(DeltaTime))
	{
		return;
	}

	// ----------------------------------------------------------------------
	// Cache movement values
	// ----------------------------------------------------------------------
	bRagdoll = Character->IsRagdoll() || Character->IsRagdollStandingUp();

	const float WorldTime = GetWorld()->GetTimeSeconds();

	Velocity = Character->GetVelocity();
	TraversalVelocity = Movement->GetTraversalVelocity();
	JumpingVelocity = FVector(0.f, 0.f, Movement->Velocity.Z);
	LandingVelocity = FVector(0.f, 0.f, TraversalVelocity.Z);
	AngularVelocity = Movement->GetAngularVelocity();
	VelocityRotation = Velocity.Rotation();
	RawSpeed = (Movement->IsMovingOnGround()) ? Velocity.Size() : Velocity.Size2D();
	Speed = UKismetMathLibrary::NormalizeToRange(RawSpeed, 0.f, Movement->GetMaxSpeed());
	GaitSpeedRaw = UKismetMathLibrary::NormalizeToRange(RawSpeed, 0.f, Movement->GetMaxGaitSpeed());
	GaitSpeed = (GaitInterpRate > 0.f) ? FMath::FInterpConstantTo(GaitSpeed, GaitSpeedRaw, DeltaTime, GaitInterpRate) : GaitSpeedRaw;
	bPlayingRootMotion = Character->GetRootMotionAnimMontageInstance() != nullptr;
	bIsMoving = Speed > 0.f && !bPlayingRootMotion;
	bIsIdle = !bIsMoving;

	AnimPlayRate = UKismetMathLibrary::MapRangeClamped(Speed, 0.f, 1.f, MinAnimPlayRate, MaxAnimPlayRate);

	if (bPlayingRootMotion && bRootMotionMontageResetsTurn)
	{
		RootYawOffset = FMath::FInterpConstantTo(RootYawOffset, 0.f, DeltaTime, RootMotionMontageTurnRate);
	}

	// ----------------------------------------------------------------------
	// Cache movement states
	// ----------------------------------------------------------------------
	const bool bWasCrouched = bIsCrouched;
	bIsCrouched = Movement->IsCrouching() && !bRagdoll;
	bIsStanding = !bIsCrouched;
	const bool bWasFalling = bIsFalling;
	bIsFalling = Movement->IsFalling() && !bRagdoll;
	bIsJumping = bIsFalling && Velocity.Z > 0.f;
	bOnGround = Movement->IsMovingOnGround() || bRagdoll;
	bCrouchingOnGround = bIsCrouched && bOnGround && !bRagdoll;
	const bool bWasSprinting = bIsSprinting;
	bIsSprinting = IsSprinting() && !bRagdoll;
	const bool bWasCrouchRunning = bIsCrouchRunning;
	bIsCrouchRunning = Movement->IsCrouchRunning() && !bRagdoll;
	bCrouchRunningOnGround = bIsCrouchRunning && bOnGround && !bRagdoll;
	const bool bWasFloorSliding = bIsFloorSliding;
	bIsFloorSliding = Character->bIsFloorSliding && !bRagdoll;
	StrafeOrientation = Character->StrafeOrientation;
	MovementSystem = Character->MovementSystem;
	bAiming = Character->IsAimingWeapon();
	bNotAiming = !bAiming;
	bAimingLocalPlayer = bAiming && Character->IsLocallyControlled();
	bNotAimingLocalPlayer = !Character->IsLocallyControlled() || !bAiming;

	bStrafeEnabled = (MovementSystem == EMIMovementSystem::MS_OrientToView) && !bRagdoll;
	bStrafeEnabled &= !bIsSprinting || bSprintUsesProceduralStrafe || bIsFalling;

	bGaitEnabled = bOnGround && bIsMoving && !bIsFloorSliding;
	bGaitEnabled &= !bIsSprinting || bApplyGaitToSprinting;
	bGaitEnabled &= !bIsFloorSliding || bApplyGaitToFloorSliding;

	GaitMultiplier = BaseGaitMultiplier;
	if (bIsSprinting) { GaitMultiplier = SprintGaitMultiplier; }
	else if (bIsCrouched) { GaitMultiplier = CrouchGaitMultiplier; }

	bFootIK = bOnGround && !bRagdoll;
	FootIKState = (bIsCrouched) ? 1 : 0;
	if (bIsFloorSliding) { FootIKState = 2; }

	// Start or stop floor slide
	if (!bWasFloorSliding && bIsFloorSliding)
	{
		OnStartFloorSliding();
	}
	else if (bWasFloorSliding && !bIsFloorSliding)
	{
		OnStopFloorSliding();
	}

	// Start or stop crouch
	if (!bWasCrouched && bIsCrouched)
	{
		OnStartCrouch();
	}
	else if (bWasCrouched && !bIsCrouched)
	{
		OnStopCrouch();
	}

	// Start or stop sprint
	if (!bWasSprinting && bIsSprinting)
	{
		OnStartSprint();
	}
	else if (bWasSprinting && !bIsSprinting)
	{
		OnStopSprint();
	}

	// Start or stop crouch run
	if (!bWasCrouchRunning && bIsCrouchRunning)
	{
		OnStartCrouchRun();
	}
	else if (bWasCrouchRunning && !bIsCrouchRunning)
	{
		OnStopCrouchRun();
	}

	const FMIWeapon LastWeapon = Weapon;
	Weapon = Character->GetWeaponAnimInfo();
	bool bWasArmed = bArmed;
	bArmed = Weapon.IsValid();
	bUnarmed = !bArmed;

	bEnableWeaponPose = bArmed;
	bEnableHandIK = bArmed && !Weapon.bIsOneHanded;

	bWeaponOneHanded = bArmed && Weapon.bIsOneHanded;
	LeftArmBlendWeight = bWeaponOneHanded ? 1.f : 0.f;

	const bool bLastUseWeaponPose = bUseWeaponPose;
	bUseWeaponPose = Weapon.WeaponPose != nullptr;

	bUseWeaponAimPose = Weapon.AimOffset != nullptr;

	if (bUseWeaponAimPose)
	{
		WeaponAimOffset = Weapon.AimOffset;
	}

	const bool bWasChangingWeapon = bChangingWeapon;

	bool bUnequipping = false;
	if (!bUseWeaponPose && bLastUseWeaponPose)
	{
		// Just unequipped, save snapshot for blending
		bUnequipping = true;
		bChangingWeapon = false;
		SavePoseSnapshot("Unequip");
	}

	if (bChangingWeapon)
	{
		// Lasts for one frame only, just to trigger the transition
		bChangingWeapon = false;
	}
	else
	{
		bChangingWeapon = LastWeapon != Weapon && !bUnequipping && bWasArmed;
	}

	if (bChangingWeapon && !bWasChangingWeapon)
	{
		// Changing weapon, save snapshot for blending
		SavePoseSnapshot("ChangingWeapon");
	}

	bAimOffsetEnabled = bAllowAimOffset && !bEnableLookTarget;

	const bool bStrafeUseVelocity = RawInput.IsZero() && Speed > 0.25f;
	const bool bStrafeUseAcceleration = Speed < 0.1f;
	const EMIStrafeDirectionBasis& StrafeDirectionBasis = (StrafeOrientation == EMIStrafeOrientation::SO_Neutral) ? NeutralStrafeDirectionBasis : RightStrafeDirectionBasis;
	const bool bVelocityBased = (StrafeDirectionBasis == EMIStrafeDirectionBasis::MISB_Velocity && !bStrafeUseAcceleration) || (StrafeDirectionBasis == EMIStrafeDirectionBasis::MISB_Acceleration && bStrafeUseAcceleration);
	const bool bAccelerationBased = !bVelocityBased;
	float BackwardsDegreesTolerance = (bVelocityBased) ? BackwardsVelocityDegreesTolerance : BackwardsAccelerationDegreesTolerance;

	if (bIsFalling)
	{
		if (!bWasFalling)
		{
			FallingDuration = 0.f;
		}

		bIsFallingDownward = Velocity.Z < 0.f;
		DistanceToLandLocation = 99999.f;
		FallingDuration += DeltaTime;
		bFallingTooFar = (FallingDuration >= FallTooFarDuration);
		if (PredictLandingLocation(PredictedLanding))
		{
			bCanLand = Movement->IsWalkable(PredictedLanding.HitResult);
			DistanceToLandLocation = Character->GetActorLocation().Z - PredictedLanding.HitResult.ImpactPoint.Z;
		}
	}
	else
	{
		PredictedLanding = FPredictProjectilePathResult();
		bCanLand = false;
		bIsFallingDownward = false;
		DistanceToLandLocation = 0.f;
		bFallingTooFar = false;

		if (bWasFalling)
		{
			bIsSprintLanding = bIsSprinting && !bArmed;
		}
	}

	// ----------------------------------------------------------------------
	// Ragdoll - Early Exit
	// ----------------------------------------------------------------------
	if (bRagdoll)
	{
		PreviousInput = Input;
		Input = FVector::ZeroVector;
		RawInput = FVector::ZeroVector;
		bWasApplyingInput = bIsApplyingInput;
		bIsApplyingInput = false;
		bIsMovingWithInput = false;
		DirectionInput = FVector::ZeroVector;
		MotionWeight = 0.f;
		Speed = 0.f;
		RawSpeed = 0.f;
		bIsMoving = false;
		bIsIdle = true;

		StoppingInput = FVector::ZeroVector;
		StoppingVelocity = FVector::ZeroVector;
		MovingInput = FVector::ZeroVector;
		StrafeDirection = FVector::ZeroVector;

		ResetTurnInPlace();

		// No longer want to factor standing up as being in ragdoll to ensure blend out plays
		bRagdoll = Character->IsRagdoll();

		PreviousVelocityRotation = VelocityRotation;
		return;
	}

	// ----------------------------------------------------------------------
	// Compute acceleration
	// ----------------------------------------------------------------------

	PreviousInput = Input;
	PreviousRawInput = RawInput;
	RawInput = Character->Input;

	bWasApplyingInput = bIsApplyingInput;
	bIsApplyingInput = !RawInput.IsNearlyZero();
	bIsMovingWithInput = bIsApplyingInput && bIsMoving;

	if (bIsApplyingInput)
	{
		Input = RawInput;
	}
	else
	{
		// Interpolate input to zero to smoothly interpolate gait nodes
		Input = FMath::VInterpConstantTo(Input, FVector::ZeroVector, DeltaTime, InputInterpToZeroRate);
	}

	// Input used for direction changes
	if (bIsIdle)
	{
		DirectionInput = RawInput;
	}
	else
	{
		DirectionInput = FMath::VInterpTo(DirectionInput, RawInput, DeltaTime, DirectionInterpRate);
	}

	if (Movement->bPivotAnimTrigger && !bPivot)
	{
		bPivot = true;
		Movement->bPivotAnimTrigger = false;

		PivotVelocity = Movement->GetLastUpdateRotation().UnrotateVector(Velocity.GetSafeNormal2D());
		PivotInput = RawInput;

		PivotStartTime = WorldTime;

		PivotDirection = Character->GetActorRotation().UnrotateVector(Movement->PivotMovementDirection).Rotation().Yaw;

		OnStartPivot();
	}

	if (bPivot && WorldTime >= (PivotStartTime + PivotDuration))
	{
		bPivot = false;

		OnStopPivot();
	}

	if (bPivot)
	{
		bStrafeEnabled = false;
		BackwardsDegreesTolerance = 0.f;
	}

	if (!bIsApplyingInput)
	{
		FVector MotionWeightVector = Movement->AccelerationDelta / (Movement->GetMaxSpeed() * 3.f);
		MotionWeightVector = Movement->GetLastUpdateRotation().UnrotateVector(MotionWeightVector);

		MotionWeight = MotionWeightVector.X * -1.f;
	}
	else
	{
		MotionWeight = 0.f;
	}

	// ----------------------------------------------------------------------
	// Compute stopping
	// ----------------------------------------------------------------------

	// Should stop if moving, not already stopping, and just stopped applying input
	const bool bShouldStop = !bIsStopping && !bIsApplyingInput && bWasApplyingInput && !bPivot && bOnGround;
	if (bShouldStop)
	{
		bIsStopping = true;
		StoppingSpeed = Speed;
		StoppingInput = RawInput;
		StoppingVelocity = Velocity;
		StoppingRotation = Movement->GetLastUpdateRotation();

		bIsStoppingFromCrouchRunning = bIsCrouchRunning;
		bIsStoppingAtSpeed = bIsStopping && Speed >= 0.8f;
		bIsStoppingBackwards = bIsMovingBackward;  // Still last frame
	}
	else
	{
		// End stopping (going back to moving)
		if (bIsStopping && bIsApplyingInput)
		{
			bIsStopping = false;
		}
	}

	if (!bIsStopping)
	{
		bIsStoppingAtSpeed = false;
	}

	bIsStoppingInAir = bIsStopping && bIsFalling;

	// ----------------------------------------------------------------------
	// Assign relevant input to use for movement anim nodes
	// ----------------------------------------------------------------------
	MovingInput = bIsStopping ? StoppingInput : Input;

	// ----------------------------------------------------------------------
	// Compute strafe direction
	// ----------------------------------------------------------------------

	bool bMaintainStrafeDirection = false;
	bool bStrafingForward = false;
	if (MovementSystem == EMIMovementSystem::MS_OrientToView)
	{
		FVector StrafeTarget = FVector::ZeroVector;
		if (bAccelerationBased)
		{
			StrafeTarget = RawInput;

			if (bPivot)
			{
				StrafeTarget = PivotInput;
			}
		}
		else
		{
			StrafeTarget = Movement->GetLastUpdateRotation().UnrotateVector(Velocity.GetSafeNormal2D());

			if (bPivot)
			{
				StrafeTarget = PivotVelocity;
			}
		}

		const bool bWasStrafingForward = bStrafingForward;
		bStrafingForward = !FMIStatics::IsStrafingBackward(StrafeTarget, StrafeOrientation, BackwardsDegreesTolerance);

		float DirectionRate = DirectionInterpRate;

		// Add an optional delay for switching forward/back direction
		{
			const float SwitchDelay = bIsMovingForward ? ForwardSwitchDelay : BackwardSwitchDelay;

			// Changed forward/back direction
			if (ShouldDelayDirectionSwitch() && bStrafingForward != bWasStrafingForward && bIsMoving && bWasApplyingInput && bIsApplyingInput)
			{
				// Wanting to maintain direction
				if (WorldTime < ForwardBackSwitchTime + SwitchDelay)
				{
					DirectionRate = (bStrafingForward) ? DirectionChangeForwardInterpRate : DirectionChangeBackwardInterpRate;
					bMaintainStrafeDirection = true;
				}
			}
			else
			{
				// If not moving or not delaying then remove switch time
				ForwardBackSwitchTime = WorldTime - SwitchDelay;
			}
		}

		// Nudge the direction forward, because turning backwards looks glitchy
		StrafeTarget += FVector::ForwardVector * 0.01f;

		const FVector FacingDirection = bStrafingForward ? StrafeTarget : -StrafeTarget;

		if (DirectionRate != 0.f)
		{
			StrafeQuat = FMath::QInterpConstantTo(StrafeQuat, FacingDirection.ToOrientationQuat(), DeltaTime, DirectionRate);
		}
		else
		{
			StrafeQuat = FacingDirection.ToOrientationQuat();
		}

		StrafeDirection = StrafeQuat.Vector();
	}
	else
	{
		StrafeDirection = FVector(1.f, 0.f, 0.f);
	}

	// ----------------------------------------------------------------------
	// Compute direction
	// ----------------------------------------------------------------------
	if (Speed > 0.f)
	{
		Direction = DirectionInput.Rotation().Yaw;
	}

	// ----------------------------------------------------------------------
	// Compute physics blending
	// ----------------------------------------------------------------------

	BlendPhysicalAnimation(DeltaTime);

	// ----------------------------------------------------------------------
	// Strafing
	// ----------------------------------------------------------------------

	const FVector UnrotatedVelocity = Movement->GetLastUpdateRotation().UnrotateVector(Velocity.GetSafeNormal2D());

	bIsMovingBackward = bStrafeEnabled && !bStrafingForward;
	bIsMovingForward = !bIsMovingBackward;

	// ----------------------------------------------------------------------
	// Turn in place
	// ----------------------------------------------------------------------

	TurnInPlace(DeltaTime);

	// ----------------------------------------------------------------------
	// Aim Offsets
	// ----------------------------------------------------------------------

	ComputeAimOffsets(DeltaTime);

	// ----------------------------------------------------------------------

	if (bMaintainStrafeDirection)
	{
		bStrafingForward = !bStrafingForward;
		bIsMovingBackward = true;
	}

	LandingSpeed = (bIsMovingForward) ? Speed : 0.f;

	// ----------------------------------------------------------------------
	// Cache end of frame variables
	// ----------------------------------------------------------------------

	PreviousVelocityRotation = VelocityRotation;
}

void UMIAnimInstance::BlendPhysicalAnimation(float DeltaTime)
{
	const bool bImpactsWereInactive = bImpactsInactive;
	bImpactsInactive = true;
	if (Character->HitImpactPhysics.IsActive())
	{
		bImpactsInactive &= Character->HitImpactPhysics.Update(Mesh, DeltaTime);
	}
	if (Character->HitCharacterImpactPhysics.IsActive())
	{
		bImpactsInactive &= Character->HitCharacterImpactPhysics.Update(Mesh, DeltaTime);
	}
	if (Character->HitByCharacterImpactPhysics.IsActive())
	{
		bImpactsInactive &= Character->HitByCharacterImpactPhysics.Update(Mesh, DeltaTime);
	}

	// Detect if any impacts are still active
	if (bImpactsInactive)
	{
		const FMIShotImpact* Shots = Character->ShotBonePhysicsImpacts.Find(Mesh);
		if (Shots)
		{
			TArray<FPhysicsBlend> Blends;
			Shots->BoneMap.GenerateValueArray(Blends);

			for (const FPhysicsBlend& Blend : Blends)
			{
				if (Blend.IsActive())
				{
					bImpactsInactive = false;
					break;
				}
			}
		}
	}

	// No impacts being played on this frame, recreate the physics state
	if (!bImpactsWereInactive && bImpactsInactive)
	{
		Mesh->RecreatePhysicsState();
	}
}

void UMIAnimInstance::TurnInPlace(float DeltaTime)
{
	const float LastYawTick = Yaw;
	Yaw = Character->GetActorRotation().Yaw;
	const float YawChangeOverFrame = LastYawTick - Yaw;

	const bool bWasTurning = bIsTurning;

	float IsTurningValue = 0.f;
	const bool bHasTurnCurve = GetCurveValue(IsTurningCurveName, IsTurningValue);

	float TurnResetRate = 0.f;
	if (bIsApplyingInput || bIsMoving || !IsTurnInPlaceEnabled(TurnResetRate))
	{
		// If we're moving interpolate back to 0
		TurnResetRate = (TurnResetRate != 0.f) ? TurnResetRate : StartMovingTurnRate;
		RootYawOffset = FMath::FInterpConstantTo(RootYawOffset, 0.f, DeltaTime, TurnResetRate);
		bDoTurn = false;
		bIsTurning = false;
		bCanRestartTurn = true;
	}
	else if (!bPlayingRootMotion)
	{
		// Not moving, turn in place
		RootYawOffset = FRotator::NormalizeAxis(YawChangeOverFrame + RootYawOffset);

		bIsTurning = !FMath::IsNearlyEqual(IsTurningValue, 0.f, 0.001f);
		if (bIsTurning)
		{
			if (bResetTurn)
			{
				bResetTurn = false;
				CurveValue = FMath::Abs(GetCurveValue(YawCurveName));
			}

			const float LastCurveValue = CurveValue;
			CurveValue = FMath::Abs(GetCurveValue(YawCurveName));

			const float CurveValueChangeOverFrame = (LastCurveValue - CurveValue) * (bTurnRight ? 1.f : -1.f);
			RootYawOffset -= CurveValueChangeOverFrame;
		}
		else
		{
			bResetTurn = true;
		}
	}
	else
	{
		bIsTurning = false;
		bCanRestartTurn = true;
	}

	// Convert to Quaternion to resolve issues with exceeding -180 to 180 (full circle and beyond)
	const FQuat RootQuatOffset = FRotator(0.f, RootYawOffset, 0.f).Quaternion().GetNormalized();
	RootYawOffset = RootQuatOffset.Rotator().Yaw;

	const bool bWantsTurnRight = RootYawOffset < 0.f;
	TurnAngle = FMath::Abs(RootYawOffset);
	const bool bWantsTurn = TurnAngle >= MinTurnAngle;
	bDoTurn = bCanRestartTurn && bWantsTurn;

	if (bDoTurn)
	{
		// Interrupt stopping animation (required for long stopping anims)
		bIsStopping = false;
	}

	if (!bDoTurn && bWantsTurn && bHasTurnCurve && !bIsTurning)
	{
		// Early exit from turn animation if playing rest frames
		bTurnExit = true;
	}

	if (MaxTurnAngle > 0.f && TurnAngle >= MaxTurnAngle)
	{
		// Max turn angle, clamp the root yaw offset
		RootYawOffset = MaxTurnAngle * (bWantsTurnRight ? -1.f : 1.f);

		TurnPlayRate = TurnRateAtMaxTurnAngle;
	}

	// Set it before starting turning
	if (!bDoTurn)
	{
		bTurnRight = RootYawOffset < 0.f;
	}

	if (bWantsTurnRight != bTurnRight && bIsTurning)
	{
		// Changing direction while playing opposing animation, speed it up to complete it
		TurnPlayRate = TurnRateDirectionChange;
	}
}

void UMIAnimInstance::ComputeAimOffsets(float DeltaTime)
{
	const FRotator MeshRotation = (Mesh->GetComponentRotation() + FRotator(0.f, 90.f, 0.f)).GetNormalized();
	const FRotator AimDelta = (Character->GetBaseAimRotation() - MeshRotation).GetNormalized();

	// Compensation for Turn in Place - usually used to rotate spine bones to match
	const float OldAimOffsetTurn = AimOffsetTurn;
	const bool bRightTurn = RootYawOffset < 0.f;
	const bool bCompensateRight = BodyTurnRightCompensationScale != 1.f && bRightTurn;
	const bool bCompensateLeft = BodyTurnLeftCompensationScale != 1.f && !bRightTurn;
	if (bCompensateRight || bCompensateLeft)
	{
		AimOffsetTurn = FRotator(0.f, -RootYawOffset * (bRightTurn ? BodyTurnRightCompensationScale : BodyTurnLeftCompensationScale), 0.f).GetNormalized().Yaw;
	}
	else
	{
		AimOffsetTurn = -RootYawOffset;
	}

	// Compensate for differences in step size
	if (StepSizes.IsValidIndex(StepSize))
	{
		float Comp = 180.f / StepSizes[StepSize];
		Comp = 1.f / Comp;
		AimOffsetTurn *= Comp;
	}
	else
	{
		AimOffsetTurn = 0.f;
	}

	const float AimYaw = (MovementSystem == EMIMovementSystem::MS_OrientToView) ? RootYawOffset : -AimDelta.Yaw;

	// Yaw
	if (AimOffsetYawRate > 0.f)
	{
		AimOffsetYaw = FMath::FInterpConstantTo(AimOffsetYaw, -AimYaw, DeltaTime, AimOffsetYawRate);
	}
	else
	{
		AimOffsetYaw = -AimYaw;
	}

	// Yaw compensation
	if (AimOffsetYawCompensateRate > 0.f)
	{
		AimOffsetTurn = FMath::FInterpConstantTo(OldAimOffsetTurn, AimOffsetTurn, DeltaTime, AimOffsetYawCompensateRate);
	}

	// Pitch
	if (AimOffsetPitchRate > 0.f)
	{
		AimOffsetPitch = FMath::FInterpConstantTo(AimOffsetPitch, AimDelta.Pitch, DeltaTime, AimOffsetPitchRate);
	}
	else
	{
		AimOffsetPitch = AimDelta.Pitch;
	}
}

void UMIAnimInstance::NativePostEvaluateAnimation()
{
	// Doesn't actually have anything in super
	Super::NativePostEvaluateAnimation();

	FMIAnimInstanceProxy& Proxy = GetProxyOnGameThread<FMIAnimInstanceProxy>();
	OffHandIKTM = Proxy.OffHandIKTM;
}

void UMIAnimInstance::ComputeSurfaceImpacts()
{
	if (!bEnableSurfaceImpacts)
	{
		return;
	}

	if (!IsValid(Character))
	{
		return;
	}

	if (Character->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (!IsValidToEvaluate(DeltaTime))
	{
		return;
	}

	EMISurfaceStateType CurrentState = EMISurfaceStateType::MIST_Default;
	if (bRagdoll)
	{
		CurrentState = EMISurfaceStateType::MIST_Ragdoll;
	}
	else if (bIsFloorSliding)
	{
		CurrentState = EMISurfaceStateType::MIST_FloorSliding;
	}

	// ----------------------------------------------------------------------
	// Compute footsteps
	// ----------------------------------------------------------------------
	for (FMISurfaceImpact& Impact : SurfaceImpacts)
	{
		// Start by invalidating last frame data after caching to prevent significant impacts occurring
		// when becoming valid again
		const FVector PreviousBoneWorldLoc = Impact.PreviousBoneWorldLocation;
		Impact.PreviousBoneWorldLocation = FVector::ZeroVector;

		if (!Impact.bEnabled)
		{
			continue;
		}

		const bool bValidLOD = Impact.LODThreshold == INDEX_NONE || GetLODLevel() <= Impact.LODThreshold;
		if (!bValidLOD)
		{
			continue;
		}

		// Compare allowed states to current states and only proceed if available
		const uint8 MutualStates = ((uint8)CurrentState & Impact.AllowedStates);

		const bool bNone = (MutualStates & (uint8)EMISurfaceStateType::MIST_None) != 0;
		if (bNone)
		{
			continue;
		}
		const bool bAllowDefaultState = (MutualStates & (uint8)EMISurfaceStateType::MIST_Default) != 0;
		const bool bAllowRagdollState = (MutualStates & (uint8)EMISurfaceStateType::MIST_Ragdoll) != 0;
		const bool bAllowFloorSlideState = (MutualStates & (uint8)EMISurfaceStateType::MIST_FloorSliding) != 0;

		if (!bAllowDefaultState && !bAllowRagdollState && !bAllowFloorSlideState)
		{
			// Impact does not play in this state
			continue;
		}

		if (!Mesh->DoesSocketExist(Impact.Socket))
		{
			UE_LOG(LogTemp, Warning, TEXT("Socket { %s } does not exist on { %s }, can not play impact/footstep"), *Impact.Socket.ToString(), *GetNameSafe(Mesh->SkeletalMesh));
			continue;
		}

		if (!Mesh->DoesSocketExist(Impact.Bone.BoneName))
		{
			UE_LOG(LogTemp, Warning, TEXT("Bone { %s } does not exist on { %s }, can not play impact/footstep"), *Impact.Socket.ToString(), *GetNameSafe(Mesh->SkeletalMesh));
			continue;
		}

		const FVector& BoneWorldLoc = Mesh->GetBoneLocation(Impact.Bone.BoneName);
		if (PreviousBoneWorldLoc == FVector::ZeroVector)
		{
			// No valid data from last frame
			Impact.PreviousBoneWorldLocation = BoneWorldLoc;
			continue;
		}

		const FVector& NewVelocity = (BoneWorldLoc - PreviousBoneWorldLoc) / DeltaTime;
		const FVector& ImpactVelocity = Impact.PreviousVelocity;
		Impact.PreviousVelocity = NewVelocity;
		Impact.PreviousBoneWorldLocation = BoneWorldLoc;

		// On cooldown, no need to process further
		if (GetWorld()->GetTimerManager().IsTimerActive(Impact.CooldownTimerHandle))
		{
			continue;
		}

		const float FootSpeed = ImpactVelocity.Size();

		// Insignificant movement
		if (FootSpeed < Impact.MinVelocity)
		{
			continue;
		}

		// Trace to the ground
		FHitResult Hit(ForceInit);
		const FVector TraceStart = Mesh->GetSocketLocation(Impact.Socket) + FVector::UpVector * Impact.TraceStartHeight;

		// Create an array of object types to trace
		TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
		for (TEnumAsByte<ECollisionChannel> ObjChannel : Impact.ObjectsToTrace)
		{
			TraceObjects.Add(UEngineTypes::ConvertToObjectType(ObjChannel));
		}

		// Add 2.4f because UCharacterMovementComponent::MAX_FLOOR_DIST = 2.4f which
		// elevates mesh this far off ground
		const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, Impact.TraceDistFromGround + 2.4f);

		if (
			UKismetSystemLibrary::LineTraceSingleForObjects(
				GetWorld(),
				TraceStart,
				TraceEnd,
				TraceObjects,
				true,
				TArray<AActor*>() = { Character },
#if WITH_EDITORONLY_DATA
				(Impact.bDebugDrawTrace ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None),
#else
				EDrawDebugTrace::None,
#endif
				Hit,
				false
			))
		{
			// Must have left the ground before it can re-trigger
			if (!Impact.bWasHit)
			{
				// Triggered, start cooldown timer
				GetWorld()->GetTimerManager().SetTimer(Impact.CooldownTimerHandle, Impact.MinTriggerInterval, false);

				UMIPhysicalMaterial* PhysMat = Hit.PhysMaterial.IsValid() ? Cast<UMIPhysicalMaterial>(Hit.PhysMaterial.Get()) : nullptr;
				if (!PhysMat) { PhysMat = Character->DefaultPhysicalMaterial; }
				if (Character->GetPhysicalMaterialOverride()) { PhysMat = Character->GetPhysicalMaterialOverride(); }

				if (PhysMat)
				{
					if (Impact.bPlaySound)
					{
						// Play sound effect
						USoundBase* const SoundToPlay = PhysMat->GetBoneImpactSound(Character);
						if (SoundToPlay)
						{
							const FRuntimeFloatCurve& VolumeCurve = PhysMat->BoneImpactVelocityToVolume;
							const FRuntimeFloatCurve& PitchCurve = PhysMat->BoneImpactVelocityToPitch;
							const float Volume = VolumeCurve.GetRichCurveConst()->Eval(FootSpeed);
							const float Pitch = PitchCurve.GetRichCurveConst()->Eval(FootSpeed);

							if (Volume > 0.f && Pitch > 0.f)
							{
								UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, Hit.ImpactPoint, Volume, Pitch);
							}
						}
					}

					if (Impact.bSpawnParticle)
					{
						// Determine correct particle effect to play
						UParticleSystem* const ParticleToPlay = PhysMat != nullptr ? PhysMat->GetBoneImpactParticle(Character) : nullptr;
						UNiagaraSystem* const NiagaraToPlay = PhysMat != nullptr ? PhysMat->GetBoneImpactNiagara(Character) : nullptr;
						const bool bNiagara = (PhysMat->ParticleSystemType == EMIParticleSystemType::MIPST_Niagara);

						// Play particle effect
						if ((bNiagara && NiagaraToPlay) || (!bNiagara && ParticleToPlay))
						{
							const FTransform ParticleTransform((-ImpactVelocity.GetSafeNormal()).Rotation(), Hit.ImpactPoint, FVector::OneVector);
							UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleToPlay, ParticleTransform, true, EPSCPoolMethod::AutoRelease);

							if (bNiagara)
							{
								UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NiagaraToPlay, ParticleTransform.GetLocation(), ParticleTransform.Rotator());
							}
							else
							{
								UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleToPlay, ParticleTransform, true, EPSCPoolMethod::AutoRelease);
							}
						}
					}
				}

				// Check if a callback event should be triggered
				bool bCallback = false;
				uint8& CallbackSettings = (Impact.bUseGlobalImpactCallback) ? GlobalImpactCallback : Impact.Callbacks;
				switch (Character->GetLocalRole())
				{
					case ROLE_Authority:
						if (Character->GetNetMode() == NM_ListenServer)
						{
							bCallback |= (CallbackSettings & (uint8)EMICallbackSetting::MICBS_ListenServer) != 0;
						}
						if (Character->IsLocallyControlled())
						{
							bCallback |= (CallbackSettings & (uint8)EMICallbackSetting::MICBS_Local) != 0;
						}
						break;
					case ROLE_AutonomousProxy:
						if (Character->IsLocallyControlled())  // Shouldn't be needed, just in case
						{
							bCallback |= (CallbackSettings & (uint8)EMICallbackSetting::MICBS_Local) != 0;
						}
						break;
					case ROLE_SimulatedProxy:
						bCallback |= (CallbackSettings & (uint8)EMICallbackSetting::MICBS_Simulated) != 0;
					default:
						break;
				}

				// Trigger callback if settings allow for it and is bound
				if (bCallback && OnSurfaceImpact.IsBound())
				{
					OnSurfaceImpact.Broadcast(Impact.Bone.BoneName, PhysMat, Hit.ImpactPoint, (-ImpactVelocity.GetSafeNormal()).Rotation(), FootSpeed);
				}
			}
		}

		Impact.bWasHit = Hit.bBlockingHit;
	}
}

bool UMIAnimInstance::IsValidToEvaluate(float DeltaTime) const
{
	if (!Character || !Movement || !Mesh)
	{
		return false;
	}

	if (DeltaTime < 1e-6f)
	{
		return false;
	}

	if (!Character->IsRagdoll())
	{
		if (Movement->MovementMode == MOVE_None)
		{
			return false;
		}
	}

	return true;
}

void UMIAnimInstance::StartTurn()
{
	TurnPlayRate = 1.f;

	bCanRestartTurn = false;
	bTurnExit = false;

	// Determine which step to use
	StepAngle = TurnAngle;
	StepSize = 0;

	for (int32 i = 0; i < StepSizes.Num(); i++)
	{
		const int32& TA = StepSizes[i];
		if (FMath::FloorToInt(StepAngle) >= TA)
		{
			StepSize = i;
		}
	}
}

void UMIAnimInstance::StopTurn()
{
	bCanRestartTurn = true;
	bTurnExit = false;
}

bool UMIAnimInstance::IsSprinting_Implementation() const
{
	return Character && Character->IsSprinting();
}

bool UMIAnimInstance::IsTurnInPlaceEnabled_Implementation(float& ResetRate) const
{
	ResetRate = 0.f;
	return bNotAimingLocalPlayer;
}

void UMIAnimInstance::ResetTurnInPlace()
{
	bDoTurn = false;
	RootYawOffset = 0.f;
	Yaw = Character->GetActorRotation().Yaw;
	bResetTurn = true;
	bCanRestartTurn = true;
}

bool UMIAnimInstance::ShouldDelayDirectionSwitch_Implementation() const
{
	return bArmed;
	//return bIsCrouched && !bIsCrouchRunning && !bIsFloorSliding;
}

bool UMIAnimInstance::PredictLandingLocation(FPredictProjectilePathResult& OutPredictResult)
{
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.98f;
	const float MaxSimTime = 2.f;
	const float SimFrequency = 15.f;
	const float GravityZ = Character->GetCharacterMovement()->GetGravityZ();
	const TArray<AActor*> TraceIgnore{ Character };

	FPredictProjectilePathParams Params = FPredictProjectilePathParams(Radius, Character->GetActorLocation(), Character->GetVelocity(), MaxSimTime);
	Params.bTraceWithCollision = true;
	Params.bTraceComplex = false;
	Params.ActorsToIgnore = TraceIgnore;
	Params.DrawDebugType = EDrawDebugTrace::None;
	Params.DrawDebugTime = 1.f;
	Params.SimFrequency = SimFrequency;
	Params.OverrideGravityZ = GravityZ;
	Params.TraceChannel = ECollisionChannel::ECC_Visibility; // Trace by channel

	// Do the trace
	if (!FMIStatics::PredictCapsulePath(Character, HalfHeight, Params, OutPredictResult))
	{
		return false;
	}
	return true;
}
