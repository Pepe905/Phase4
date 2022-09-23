// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "MICharacter.h"
#include "MICharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "MIPhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "MIViewComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "Curves/CurveFloat.h"
#include "MITypes.h"
#include "Niagara/Public/NiagaraFunctionLibrary.h"
#include "MISkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetMathLibrary.h"


DEFINE_LOG_CATEGORY_STATIC(LogMICharacter, Log, All);

AMICharacter::AMICharacter(const FObjectInitializer& OI)
	: Super(OI.SetDefaultSubobjectClass<UMICharacterMovementComponent>(ACharacter::CharacterMovementComponentName).SetDefaultSubobjectClass<UMISkeletalMeshComponent>(ACharacter::MeshComponentName))
{
	MICharacterMovement = Cast<UMICharacterMovementComponent>(GetCharacterMovement());

	StrafeOrientation = EMIStrafeOrientation::SO_Neutral;

	HitWallAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HitWallAudioComponent"));
	HitWallAudioComponent->bAutoActivate = false;
#if WITH_EDITORONLY_DATA
	HitWallAudioComponent->bVisualizeComponent = false;
#endif

	Input = FVector::ZeroVector;
	PreviousInput = FVector::ZeroVector;
	PreviousNonZeroInput = FVector(1.f, 0.f, 0.f);
	PendingInput = FVector::ZeroVector;

	StopRootMotionOrientTime = 0.3f;
	bDisableControlRotationDuringRootMotion = true;

	MovementSystemChangeOrientRate = 4.f;
	MovementSystemViewAlpha = 0.f;

	bIsSprinting = false;
	bCanEverCrouchRun = true;
	AirCrouchBehaviour = EMICrouchInAirBehaviour::CAB_Wait;
	bPendingCrouch = false;

	OrientDefaultSettings.OrientAngleMultiplier = 0.f;

	// Impacts
	ImpactLODThreshold = 2;
	bHitWallEnabled = true;
	bHitCharacterEnabled = true;
	HitWallImpactVelocityThreshold = 200.f;
	HitWallMinInterval = 0.1f;

	// Hit Impact Velocity To Physics Impulse
	HitImpactVelocityToPhysicsImpulse.GetRichCurve()->AddKey(9000, 10.f);
	HitImpactVelocityToPhysicsImpulse.GetRichCurve()->AddKey(15000, 100.f);

	// Hit Impact Physics
	HitImpactPhysics.BoneName = "spine_02";
	HitImpactPhysics.BlendIn.SetBlendTime(0.03f);
	HitImpactPhysics.BlendIn.SetBlendOption(EAlphaBlendOption::CircularOut);
	HitImpactPhysics.BlendOut.SetBlendTime(0.35f);
	HitImpactPhysics.BlendOut.SetBlendOption(EAlphaBlendOption::CircularIn);
	HitImpactPhysics.MaxBlendWeight = 0.5f;

	// Hit Character Impact Physics
	HitCharacterImpactPhysics.BoneName = "spine_02";
	HitCharacterImpactPhysics.BlendIn.SetBlendTime(0.05f);
	HitCharacterImpactPhysics.BlendIn.SetBlendOption(EAlphaBlendOption::CircularOut);
	HitCharacterImpactPhysics.BlendOut.SetBlendTime(0.5f);
	HitCharacterImpactPhysics.BlendOut.SetBlendOption(EAlphaBlendOption::CircularIn);
	HitCharacterImpactPhysics.MaxBlendWeight = 0.4f;

	// Hit By Character Impact Physics
	HitByCharacterImpactPhysics.BoneName = "spine_02";
	HitByCharacterImpactPhysics.ImpulseMultiplier = 15.f;
	HitByCharacterImpactPhysics.BlendIn.SetBlendTime(0.03f);
	HitByCharacterImpactPhysics.BlendIn.SetBlendOption(EAlphaBlendOption::CircularOut);
	HitByCharacterImpactPhysics.BlendOut.SetBlendTime(0.3f);
	HitByCharacterImpactPhysics.BlendOut.SetBlendOption(EAlphaBlendOption::CircularIn);

	HitCharacterVelocityThreshold = 350.f;
	HitByCharacterVelocityThreshold = 350.f;
	HitCharacterMinInterval = 0.5f;
	HitByCharacterMinInterval = 0.5f;
	HitByCharacterMinVoiceInterval = 5.f;

	ScuffWallSoundVelocityThreshold = 80.f;
	ScuffWallSoundMinInterval = 0.2f;
	ScuffWallParticleMinInterval = 0.01f;

	HitMaxUpNormal = 0.f;
	HitMaxMovementNormal = 0.83f;
	ScuffMaxUpNormal = 0.f;
	ScuffMaxMovementNormal = 0.f;

	ShotImpactPhysics.BlendIn.SetBlendTime(0.1f);
	ShotImpactPhysics.BlendIn.SetBlendOption(EAlphaBlendOption::HermiteCubic);
	ShotImpactPhysics.BlendOut.SetBlendTime(0.15f);
	ShotImpactPhysics.BlendOut.SetBlendOption(EAlphaBlendOption::HermiteCubic);
	ShotImpactPhysics.MaxBlendWeight = 0.5f;
	ShotImpactPhysics.MaxImpulseTaken = 500.f;
	ShotImpactBoneRedirects = { { "pelvis", "spine_01"}, { "hand_l", "lowerarm_l"}, { "hand_r", "lowerarm_r"}, {"foot_l", "calf_l"}, {"foot_r", "calf_r"} };

	RagdollPelvisBoneName = TEXT("pelvis");
	bRagdollStandUpTimeFromAnimation = true;
	RagdollStandUpTime = 1.f;
	RagdollAggressiveSyncTime = 10.f;
	RagdollAggressiveSyncRate = 50000.f;
	RagdollNormalSyncRate = 7500.f;
	RagdollSmallDistanceThreshold = 100.f;
	RagdollLargeDistanceThreshold = 5000.f;

	// Replicated property cache
	bWasSimulatingPivot = false;
	SavedRagdollActorLocation = FVector::ZeroVector;
	SavedResponseToPawn = ECR_MAX;
	SavedObjectType = ECC_MAX;
	SavedCollisionEnabled = ECollisionEnabled::NoCollision;

	GetCapsuleComponent()->bReturnMaterialOnMove = true;

	// Make the capsule smaller to allow for procedural ducking
	GetCapsuleComponent()->InitCapsuleSize(34.f, 72.f);

	GetMesh()->SetCollisionObjectType(ECC_PhysicsBody);  // Can't be of type 'pawn' or capsule will block shots meant for the mesh for shot reactions
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);  // Required for shot reactions and physics blend
}

void AMICharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Cache required cameras
	CachedCameras = GetViewComponentStateCameras();

	// If no cameras are cached, do it on construction
	if (CachedCameras.Num() == 0)
	{
		GetComponents<UCameraComponent>(CachedCameras);
	}
}

void AMICharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AMICharacter::OnCapsuleComponentHit);

	MIViewComponent = FindComponentByClass<UMIViewComponent>();
}

FVector AMICharacter::GetVelocity() const
{
	return IsRagdoll() ? GetMesh()->GetPhysicsLinearVelocity(GetMesh()->GetBoneName(1)) : Super::GetVelocity();
}

void AMICharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Don't replicate anything unnecessary
	DOREPLIFETIME_ACTIVE_OVERRIDE(AMICharacter, bIsCrouchRunning, bCanEverCrouchRun);
}

void AMICharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AMICharacter, InitialRootYawOffset, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMICharacter, bIsSprinting, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AMICharacter, bIsCrouchRunning, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AMICharacter, bIsFloorSliding, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AMICharacter, Input, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AMICharacter, bSimulatedPivot, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AMICharacter, bRagdoll, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AMICharacter, RagdollActorLocation, COND_SimulatedOnly);
}

void AMICharacter::PreNetReceive()
{
	Super::PreNetReceive();

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		bWasSimulatingPivot = bSimulatedPivot;
	}
	SavedRagdollActorLocation = RagdollActorLocation;
}

void AMICharacter::PostNetReceive()
{
	Super::PostNetReceive();

	// Received ragdoll update
	if (SavedRagdollActorLocation != RagdollActorLocation)
	{
		UpdateRagdoll();
	}
}

void AMICharacter::OnRep_InitialOffset()
{
	if (UMISkeletalMeshComponent* const MIMesh = Cast<UMISkeletalMeshComponent>(GetMesh()))
	{
		MIMesh->InitRootYawFromReplication(InitialRootYawOffset);
	}
}

FMIWeapon AMICharacter::GetWeaponAnimInfo_Implementation() const
{
	return FMIWeapon();
}

UMIPhysicalMaterial* AMICharacter::GetPhysicalMaterialOverride_Implementation() const
{
	return nullptr;
}

bool AMICharacter::IsViewComponentStateActive_Implementation(uint8 State) const
{
	switch (State)
	{
	case 0:  // Crouch
		return bIsCrouched && !bIsFloorSliding && !bIsCrouchRunning;
	case 1:  // Sprint
		return IsSprinting() && !bIsCrouchRunning;
	case 2:  // Floor Slide
		return bIsFloorSliding;
	case 3:  // Crouch Running
		return bIsCrouchRunning;
	}

	return false;
}

TArray<UCameraComponent*> AMICharacter::GetViewComponentStateCameras_Implementation() const
{
	return CachedCameras;
}

void AMICharacter::OnCharacterStateChanged()
{
	if (GetMIViewComponent()) { GetMIViewComponent()->OnCharacterStateChanged(); }
}

void AMICharacter::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	if (GetMIViewComponent())
	{
		GetMIViewComponent()->CalcCamera(DeltaTime, OutResult);
	}
	else
	{
		Super::CalcCamera(DeltaTime, OutResult);
	}
}

void AMICharacter::SetMovementSystem(EMIMovementSystem NewMovement)
{
	if (MovementSystem != NewMovement && MICharacterMovement)
	{
		MovementSystem = NewMovement;

		// Interpolate a change in the movement system to prevent snapping when going back to orient to view
		bMovementSystemInterp = true;
		MovementSystemControlRotation = GetActorRotation();

		// Change required settings
		switch (MovementSystem)
		{
		case EMIMovementSystem::MS_OrientToView:
			bUseControllerRotationYaw = true;
			MICharacterMovement->bOrientRotationToMovement = false;
			break;
		case EMIMovementSystem::MS_OrientToMovement:
		case EMIMovementSystem::MS_CycleOrientToMovement:
			bUseControllerRotationYaw = false;
			MICharacterMovement->bOrientRotationToMovement = true;
			break;
		default:
			break;
		}
	}
}

void AMICharacter::AddForwardMovementInput(FVector WorldDirection, float ScaleValue, bool bForce /* = false */)
{
	// Rerouting to AddMovementInput instead of calling it directly allows us to store the raw input values, 
	// which is required to drive animations
	if (bForce || !IsMoveInputIgnored())
	{
		PendingInput.X += ScaleValue;
	}
	AddMovementInput(WorldDirection, ScaleValue, bForce);
}

void AMICharacter::AddRightMovementInput(FVector WorldDirection, float ScaleValue, bool bForce /* = false */)
{
	// Rerouting to AddMovementInput instead of calling it directly allows us to store the raw input values,
	// which is required to drive animations
	if (bForce || !IsMoveInputIgnored())
	{
		PendingInput.Y += ScaleValue;
	}
	AddMovementInput(WorldDirection, ScaleValue, bForce);
}

void AMICharacter::HandleConsumeInputVector()
{
	// Called from UMICharacterMovementComponent::ConsumeInputVector()
	// This maintains the same entry points used by CMC to ensure that the raw input values are processed at
	// the exact same time as Acceleration so nothing is ever out of sync

	// Note: This is called on simulated proxies and servers! This check is very important
	if (IsLocallyControlled() && IsPlayerControlled())
	{
		PreviousInput = Input;
		if (!PreviousInput.IsNearlyZero()) { PreviousNonZeroInput = PreviousInput; }
		Input = PendingInput;
		PendingInput = FVector::ZeroVector;

		// IsLocallyControlled is true for authority in standalone; only send the RPC if we're not authority, of course
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			// Compress the inputs to 1 decimal point to save on bandwidth
			const float CompressedX = FMath::RoundToFloat(Input.X * 10.f) / 10.f;
			const float CompressedY = FMath::RoundToFloat(Input.Y * 10.f) / 10.f;
			// Send input to server (to replicate to other clients)
			SendInputToServer(CompressedX, CompressedY);
		}
	}
}

void AMICharacter::SendInputToServer_Implementation(float X, float Y)
{
	PreviousInput = Input;
	if (!PreviousInput.IsNearlyZero()) { PreviousNonZeroInput = PreviousInput; }

	// Assign the received input vector, will replicate to simulated proxies for animation purposes
	Input = FVector(X, Y, 0.f);
}

bool AMICharacter::SendInputToServer_Validate(float X, float Y)
{
	return true;
}

void AMICharacter::OnAnimInstanceChanged(UAnimInstance* const PreviousAnimInstance)
{
	if (IsValid(PreviousAnimInstance))
	{
		PreviousAnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &AMICharacter::OnStopMontage);
	}

	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		// Remove first, because if already bound can cause crash
		GetMesh()->GetAnimInstance()->OnMontageBlendingOut.RemoveDynamic(this, &AMICharacter::OnStopMontage);
		GetMesh()->GetAnimInstance()->OnMontageBlendingOut.AddDynamic(this, &AMICharacter::OnStopMontage);
	}

	K2_OnAnimInstanceChanged();
}

void AMICharacter::OnStopMontage(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage && Montage->HasRootMotion())
	{
		if (bDisableControlRotationDuringRootMotion && StopRootMotionOrientTime > 0.f)
		{
			GetWorldTimerManager().SetTimer(StopRootMotionRotationHandle, StopRootMotionOrientTime, false);
		}
	}
}

void AMICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If moving fast enough to floor slide, save the start time so we can check if min time elapses
	if (MICharacterMovement && MICharacterMovement->FloorSlideConditions != EMIFloorSlide::FSR_NeverUsed)
	{
		if (FloorSlideSpeedStartTime == -1.f)  // Not already set
		{
			// Started moving fast enough this frame, set the start time
			if (GetVelocity().Size2D() >= MICharacterMovement->FloorSlideMinStartSpeed)
			{
				FloorSlideSpeedStartTime = GetWorld()->GetTimeSeconds();
			}
		}
		else if (GetVelocity().Size2D() < MICharacterMovement->FloorSlideMinStartSpeed)
		{
			// Fell below min speed, reset start time
			FloorSlideSpeedStartTime = -1.f;
		}
	}

	// Update last simulated inputs (mainly used for pivoting)
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		PreviousInput = Input;
		if (!PreviousInput.IsNearlyZero()) { PreviousNonZeroInput = PreviousInput; }
	}

	if (!IsPlayerControlled() && GetLocalRole() == ROLE_Authority && GetCharacterMovement())
	{
		PreviousInput = Input;
		Input = GetActorRotation().UnrotateVector(GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal());
	}

	TickRagdoll();

	TickShotImpacts(DeltaTime);
}

void AMICharacter::TickShotImpacts(float DeltaTime)
{
	TArray<USkeletalMeshComponent*> CompletedMeshes;
	// TMap<USkeletalMeshComponent*, FMIShotImpact> ShotBonePhysicsImpacts;
	for (auto& MeshItr : ShotBonePhysicsImpacts)
	{
		FMIShotImpact& ShotImpact = MeshItr.Value;
		TArray<FName> Completed;
		// TMap<FName, FPhysicsBlend> BoneMap;
		for (auto& PhysicsItr : ShotImpact.BoneMap)
		{
			FPhysicsBlend& Blend = PhysicsItr.Value;
			const bool bCompleted = Blend.Update(MeshItr.Key, DeltaTime);
			if (bCompleted)
			{
				Completed.Add(PhysicsItr.Key);
			}
		}

		// Remove all completed impacts
		for (const FName& CompletedName : Completed)
		{
			ShotImpact.BoneMap.Remove(CompletedName);
		}

		if (ShotImpact.BoneMap.Num() == 0)
		{
			CompletedMeshes.Add(MeshItr.Key);
		}
	}

	// Remove all completed meshes
	for (const USkeletalMeshComponent* const CompletedMesh : CompletedMeshes)
	{
		ShotBonePhysicsImpacts.Remove(CompletedMesh);
	}
}

void AMICharacter::TickRagdoll()
{
	if (IsRagdoll())
	{
		// Stop Z gravity if falling too fast to prevent ragdoll falling through the world
		if (GetVelocity().Z < -GetPhysicsVolume()->TerminalVelocity)
		{
			if (GetMesh()->IsGravityEnabled())
			{
				GetMesh()->SetEnableGravity(false);
			}
		}
		else
		{
			if (!GetMesh()->IsGravityEnabled())
			{
				GetMesh()->SetEnableGravity(true);
			}
		}

		// Determine ragdoll location
		if (IsLocallyControlled() || GetTearOff())
		{
			// Use pelvis location as ragdoll location
			RagdollLocation = GetMesh()->GetSocketLocation(RagdollPelvisBoneName);
			RagdollActorLocation = RagdollLocation;

			// Determine actor location (so camera continues moving and gets up in the right spot)
			FHitResult Hit(ForceInit);
			RagdollTraceGround(Hit, RagdollLocation);

			bRagdollOnGround = Hit.bBlockingHit;
			if (bRagdollOnGround)
			{
				RagdollActorLocation.Z = Hit.ImpactPoint.Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.4f;
			}

			// Set location
			SetActorLocation(RagdollActorLocation);

			// Send ragdoll location to server if required
			if (!GetTearOff() && GetNetMode() == NM_Client)
			{
				// Update server unless no significant change has occurred
				if (!LastNetSendRagdollActorLocation.Equals(RagdollActorLocation, 0.01f))
				{
					LastNetSendRagdollActorLocation = RagdollActorLocation;
					ServerReceiveRagdoll(RagdollActorLocation);
				}
			}
		}
		// Update provided location with best effort interpolation
		else
		{
			const FVector PelvisLocation = GetMesh()->GetSocketLocation(RagdollPelvisBoneName);
			const FVector RagdollDiff = (RagdollActorLocation - PelvisLocation);
			if (RagdollDiff.Size() > RagdollLargeDistanceThreshold)
			{
				// Exceeded distance, just teleport the ragdoll
				SetActorLocation(RagdollActorLocation);
			}
			else if (RagdollDiff.Size() > RagdollSmallDistanceThreshold)
			{
				// Within distance, interpolate the ragdoll
				const float Magnitude = (GetWorldTimerManager().IsTimerActive(RagdollCorrectionTimerHandle)) ? RagdollAggressiveSyncRate : RagdollNormalSyncRate;
				GetMesh()->AddForce(RagdollDiff.GetSafeNormal() * Magnitude, RagdollPelvisBoneName, true);
			}
		}
	}
}

void AMICharacter::RagdollTraceGround(FHitResult& Hit, const FVector& InRagdollLocation)
{
	//EDrawDebugTrace::Type TraceType = IsLocallyControlled() ? EDrawDebugTrace::None : EDrawDebugTrace::ForDuration;

	const FVector TraceLocation = InRagdollLocation - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	UKismetSystemLibrary::LineTraceSingle(
		this,
		InRagdollLocation,
		TraceLocation,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		TArray<AActor*>() = {},
		//TraceType,
		EDrawDebugTrace::None,
		Hit,
		true
	);
}

void AMICharacter::CheckJumpInput(float DeltaTime)
{
	// This is an optimal point where inputs are checked before being processed by CMC prediction system
	// It is specific to Jump *only* because jump is the only default input that needs to process them here
	// We use this to process sprint inputs

	Super::CheckJumpInput(DeltaTime);

	if (MICharacterMovement)
	{
		MICharacterMovement->CheckMovementInput(DeltaTime);
	}
}

bool AMICharacter::CanJumpInternal_Implementation() const
{
	// Implementation of Coyote Time (allowing jumping after already walked off ledge)
	if (MICharacterMovement && MICharacterMovement->CoyoteTime > 0.f && MICharacterMovement->IsFalling() && JumpCurrentCount == 1 && GetWorld()->GetTimeSeconds() <= (MICharacterMovement->CoyoteStartTime + MICharacterMovement->CoyoteTime))
	{
		return true;
	}

	return Super::CanJumpInternal_Implementation();
}

void AMICharacter::Jump()
{
	// Implementation of Bunny Time (allowing jumping if input was pressed slightly before we landed)
	if (MICharacterMovement && MICharacterMovement->BunnyTime > 0.f && MICharacterMovement->IsFalling() && !IsJumpProvidingForce())
	{
		MICharacterMovement->BunnyStartTime = GetWorld()->GetTimeSeconds();
	}

	Super::Jump();
}

void AMICharacter::FaceRotation(FRotator NewControlRotation, float DeltaTime /* = 0.f */)
{
	// Disable rotation during root motion if false
	if (bDisableControlRotationDuringRootMotion && HasAnyRootMotion())
	{
		StopRootInitialRotationYaw = GetActorRotation().Yaw;
		return;
	}
	// Interpolate rotation after root motion
	if (GetWorldTimerManager().IsTimerActive(StopRootMotionRotationHandle))
	{
		const float Alpha = UKismetMathLibrary::MapRangeClamped(GetWorldTimerManager().GetTimerRemaining(StopRootMotionRotationHandle), StopRootMotionOrientTime, 0.f, 0.f, 1.f);
		NewControlRotation = FMath::Lerp(FRotator(NewControlRotation.Pitch, StopRootInitialRotationYaw, NewControlRotation.Roll), NewControlRotation, Alpha).GetNormalized();
	}
	// Interpolate a change in the movement system to prevent snapping when going back to orient to view
	else if (bMovementSystemInterp)
	{
		const bool bView = MovementSystem == EMIMovementSystem::MS_OrientToView;
		const float TargetAlpha = bView ? 0.f : 1.f;
		if (bView)
		{
			// Interpolate away
			MovementSystemViewAlpha = FMath::FInterpConstantTo(MovementSystemViewAlpha, TargetAlpha, DeltaTime, MovementSystemChangeOrientRate);
		}
		else
		{
			// No need to interpolate, happens as a natural result of this movement mode
			MovementSystemViewAlpha = TargetAlpha;
		}

		if (MovementSystemViewAlpha == TargetAlpha)
		{
			bMovementSystemInterp = false;
		}

		NewControlRotation = FMath::Lerp(MovementSystemControlRotation, NewControlRotation, 1.f - MovementSystemViewAlpha);
	}

	Super::FaceRotation(NewControlRotation, DeltaTime);
}

void AMICharacter::OnRep_SimulatedPivot()
{
	if (GetLocalRole() == ROLE_SimulatedProxy && MICharacterMovement)
	{
		if (!bWasSimulatingPivot && bSimulatedPivot)
		{
			MICharacterMovement->bPivot = true;
			MICharacterMovement->OnPivotStart();
		}
		else if (bWasSimulatingPivot && !bSimulatedPivot)
		{
			MICharacterMovement->bPivot = false;
			MICharacterMovement->OnPivotStop();
		}
	}
}

void AMICharacter::TakeShotPhysicsImpact(FName BoneName, USkeletalMeshComponent* HitMesh, const FVector& HitNormal, const float HitMagnitude)
{
	if (BoneName.IsNone() || !HitMesh || !HitMesh->DoesSocketExist(BoneName))
	{
		return;
	}

	if (ShotImpactBoneRedirects.Contains(BoneName))
	{
		BoneName = ShotImpactBoneRedirects[BoneName];
	}

	FMIShotImpact& Shot = ShotBonePhysicsImpacts.FindOrAdd(HitMesh);
	FPhysicsBlend* PhysicsBlend = nullptr;
	if (Shot.BoneMap.Contains(BoneName))
	{
		PhysicsBlend = &Shot.BoneMap[BoneName];
	}
	else
	{
		FPhysicsBlend NewBlend = ShotImpactPhysics;
		NewBlend.BoneName = BoneName;
		PhysicsBlend = &Shot.BoneMap.Add(BoneName, NewBlend);
	}
	PhysicsBlend->Impact(HitMesh, HitNormal, HitMagnitude);
}

bool AMICharacter::IsAimingWeapon_Implementation() const
{
	return false;
}

bool AMICharacter::CanStartRagdoll_Implementation() const
{
	return true;
}

bool AMICharacter::CanStopRagdoll_Implementation() const
{
	return !GetTearOff();
}

void AMICharacter::OnRep_Ragdoll()
{
	if (bRagdoll)
	{
		OnStartRagdoll();
	}
	else
	{
		OnStopRagdoll();
	}
}

void AMICharacter::StartRagdoll()
{
	if (CanStartRagdoll())
	{
		bRagdoll = true;
		OnRep_Ragdoll();
	}
}

void AMICharacter::StopRagdoll()
{
	if (CanStopRagdoll())
	{
		bRagdoll = false;
		OnRep_Ragdoll();
	}
}

void AMICharacter::OnStartRagdoll()
{
	// For unposession, changes netmode
	RagdollNetMode = GetNetMode();

	GetWorldTimerManager().ClearTimer(RagdollStandUpTimerHandle);

	SetReplicateMovement(false);

	MICharacterMovement->SetMovementMode(MOVE_None, 0);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SavedCollisionEnabled = GetMesh()->GetCollisionEnabled();
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SavedObjectType = GetMesh()->GetCollisionObjectType();
	GetMesh()->SetCollisionObjectType(ECC_PhysicsBody);

	SavedResponseToPawn = GetMesh()->GetCollisionResponseToChannel(ECC_Pawn);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	GetMesh()->SetSimulatePhysics(true);

	if (RagdollAggressiveSyncTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(RagdollCorrectionTimerHandle, RagdollAggressiveSyncTime, false);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(RagdollCorrectionTimerHandle);
	}

	K2_OnStartRagdoll();
}

void AMICharacter::OnStopRagdoll()
{
	SetReplicateMovement(true);

	const FTransform PelvisTM = GetMesh()->GetSocketTransform(GetMesh()->GetBoneName(1));
	const bool bStandUpOnFace = PelvisTM.Rotator().Roll > 0.f;

	// Simulated and Authority don't know if the ragdoll is on ground or not since its not relevant until now, test
	if (!IsLocallyControlled())
	{
		FHitResult Hit(ForceInit);
		RagdollTraceGround(Hit, RagdollActorLocation - FVector(0.f, 0.f, 10.f));
		bRagdollOnGround = Hit.bBlockingHit;
	}

	UAnimSequenceBase* AnimToPlay = (bStandUpOnFace) ? RagdollStandUpFace : RagdollStandUpBack;
	if (bRagdollOnGround)
	{
		if (!RagdollStandUpBack)
		{
			AnimToPlay = RagdollStandUpFace;
		}
		else if (!RagdollStandUpFace)
		{
			AnimToPlay = RagdollStandUpBack;
		}
	}
	else
	{
		AnimToPlay = nullptr;
	}

	float StandUpTime = 0.f;
	if (bRagdollStandUpTimeFromAnimation && AnimToPlay)
	{
		StandUpTime = AnimToPlay->GetPlayLength() / AnimToPlay->RateScale;
	}
	else if (bRagdollOnGround)
	{
		StandUpTime = RagdollStandUpTime;
	}

	if (GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->SavePoseSnapshot(TEXT("Ragdoll"));
		if (AnimToPlay)
		{
			GetMesh()->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(AnimToPlay, TEXT("DefaultSlot"));
		}
	}

	MICharacterMovement->SetMovementMode(MOVE_Walking, 0);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(SavedObjectType);
	GetMesh()->SetCollisionEnabled(SavedCollisionEnabled);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetPhysicsBlendWeight(0.f);

	if (StandUpTime > 0.f)
	{
		bRagdollStandingUp = true;
		GetWorldTimerManager().SetTimer(RagdollStandUpTimerHandle, this, &AMICharacter::OnRagdollStandUp, StandUpTime, false);
	}
	else
	{
		OnRagdollStandUp();
	}

	if (SavedResponseToPawn < ECR_MAX)
	{
		GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, SavedResponseToPawn);
	}

	K2_OnStopRagdoll();
}

void AMICharacter::OnRagdollStandUp()
{
	bRagdollStandingUp = false;
	K2_OnRagdollStandUp();
}

void AMICharacter::ServerReceiveRagdoll_Implementation(const FVector& NewRagdollActorLocation)
{
	RagdollActorLocation = NewRagdollActorLocation;
	UpdateRagdoll();
}

bool AMICharacter::ServerReceiveRagdoll_Validate(const FVector& NewRagdollActorLocation)
{
	return true;
}

void AMICharacter::UpdateRagdoll()
{
	SetActorLocation(RagdollActorLocation);
}

bool AMICharacter::CanStartSprinting() const
{
	// No missing references or invalid states
	if (!MICharacterMovement || !MICharacterMovement->IsActive() || IsPendingKillPending() || !GetRootComponent() || GetRootComponent()->IsSimulatingPhysics())
	{
		return false;
	}

	// Must be on ground
	if (!MICharacterMovement->IsMovingOnGround())
	{
		return false;
	}

	// Not already sprinting
	return !bIsSprinting;
}

bool AMICharacter::CanSprint_Implementation() const
{
	// No missing references or invalid states
	if (!MICharacterMovement || !MICharacterMovement->IsActive() || IsPendingKillPending() || !GetRootComponent() || GetRootComponent()->IsSimulatingPhysics())
	{
		return false;
	}

	// Can't sprint if floor sliding
	if (IsFloorSliding())
	{
		return false;
	}

	// This check ensures that we are not sprinting backward or sideways, while allowing leeway defined in 
	// UMICharacterMovementComponent::MaxSprintDirectionInputAngle (the default behaviour allows sprinting when
	// holding forward, forward left, forward right, but not left or right or backward)
	if (MICharacterMovement->GetMaxSprintDirectionInputAngle() > 0.f)
	{
		const float Dot = (MICharacterMovement->GetCurrentAcceleration().GetSafeNormal2D() | GetActorForwardVector());
		if (Dot < MICharacterMovement->GetMaxSprintDirectionInputNormal())
		{
			return false;
		}
	}

	// Don't allow sprinting backwards when strafe right is enabled (could never look right unless setup to do so, in which case developer should modify this)
	if (StrafeOrientation == EMIStrafeOrientation::SO_Right && FMIStatics::IsStrafingBackward(Input, StrafeOrientation, 0.f))
	{
		return false;
	}

	return true;
}

bool AMICharacter::IsSprinting_Implementation() const
{
	if (!MICharacterMovement) { return false; }

	// Used in MoveIt versions >= 2.305
	// Enter sprint state even if just started moving
	return MICharacterMovement->IsSprinting();

	// Used in MoveIt versions < 2.305
	// Only use sprinting state when exceeding walk speed
	// To use this, ideally override this function and use the commented logic below, 
	// or if you want to modify the plugin itself, comment the line above and uncomment the lines below

	//const float TestVelocity = (MICharacterMovement->IsMovingOnGround()) ? GetVelocity().Size() : GetVelocity().Size2D();
	//return MICharacterMovement->IsSprinting() && TestVelocity >= (MICharacterMovement->GetBaseMaxSpeed() * 0.97f * (MICharacterMovement->IsMovingBackwards() ? MICharacterMovement->MoveBackwardsSpeedMultiplier : 1.f));
}

void AMICharacter::Sprint()
{
	// Handle sprint input
	if (MICharacterMovement)
	{
		// Hackers could of course change this locally, however it is still checked
		// on the server later, having it here is nicer for the local player
		if (CanStartSprinting())
		{
			MICharacterMovement->bWantsToSprint = true;
		}
	}
}

void AMICharacter::StopSprinting()
{
	// Handle sprint input
	if (MICharacterMovement)
	{
		MICharacterMovement->bWantsToSprint = false;
	}
}

void AMICharacter::OnStartSprint()
{
	SprintStartTime = GetWorld()->GetTimeSeconds();
	OnCharacterStateChanged();
	OnStartSprinting();
	K2_OnStartSprint();
}

void AMICharacter::OnStopSprint()
{
	OnCharacterStateChanged();
	OnStopSprinting();
	K2_OnStopSprint();
}

void AMICharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (GetMIViewComponent())
	{
		GetMIViewComponent()->OnHalfHeightChanged(ScaledHalfHeightAdjust);
	}

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AMICharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (GetMIViewComponent())
	{
		GetMIViewComponent()->OnHalfHeightChanged(ScaledHalfHeightAdjust);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AMICharacter::OnRep_IsSprinting()
{
	// Replication for simulated proxies to change sprint state
	if (MICharacterMovement)
	{
		if (bIsSprinting)
		{
			MICharacterMovement->bWantsToSprint = true;
		}
		else
		{
			MICharacterMovement->bWantsToSprint = false;
		}
		MICharacterMovement->bNetworkUpdateReceived = true;
	}
}

bool AMICharacter::CanCrouchRun_Implementation() const
{
	// Movement deactivated or being destroyed
	if (!MICharacterMovement || !MICharacterMovement->IsActive() || IsPendingKillPending())
	{
		return false;
	}

	if (!bCanEverCrouchRun)
	{
		return false;
	}

	// Can't crouch run if floor sliding
	if (IsFloorSliding())
	{
		return false;
	}

	// Can't crouch run if we're not providing any input
	if (MICharacterMovement->GetCurrentAcceleration().IsNearlyZero())
	{
		return false;
	}

	return true;
}

void AMICharacter::OnStartCrouchRun()
{
	OnCharacterStateChanged();
	K2_OnStartCrouchRun();
}

void AMICharacter::OnStopCrouchRun()
{
	OnCharacterStateChanged();
	K2_OnStopCrouchRun();
}

bool AMICharacter::CanCrouch() const
{
	if (MICharacterMovement && MICharacterMovement->IsFalling())
	{
		switch (AirCrouchBehaviour)
		{
		case EMICrouchInAirBehaviour::CAB_Allow:
			break;
		case EMICrouchInAirBehaviour::CAB_Deny:
		case EMICrouchInAirBehaviour::CAB_Wait:
			return false;
		}
	}

	return Super::CanCrouch();
}

void AMICharacter::Crouch(bool bClientSimulation /*= false*/)
{
	Super::Crouch(bClientSimulation);

	if (!bClientSimulation && MICharacterMovement && !MICharacterMovement->IsCrouching() && MICharacterMovement->IsFalling() && AirCrouchBehaviour == EMICrouchInAirBehaviour::CAB_Wait)
	{
		bPendingCrouch = true;
	}
}

void AMICharacter::UnCrouch(bool bClientSimulation /*= false*/)
{
	if (!bClientSimulation)
	{
		bPendingCrouch = false;
	}

	Super::UnCrouch(bClientSimulation);
}

void AMICharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode /*= 0*/)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// Just landed
	if (PrevMovementMode == MOVE_Falling && MICharacterMovement && MICharacterMovement->IsMovingOnGround())
	{
		if (AirCrouchBehaviour == EMICrouchInAirBehaviour::CAB_Wait && bPendingCrouch && !MICharacterMovement->IsCrouching())
		{
			Crouch();
		}
	}
}

void AMICharacter::OnRep_IsCrouchRunning()
{
	// Replication for simulated proxies to change crouch running state
	if (MICharacterMovement)
	{
		if (bIsCrouchRunning)
		{
			MICharacterMovement->StartCrouchRun(true);
		}
		else
		{
			MICharacterMovement->StopCrouchRun(true);
		}
		MICharacterMovement->bNetworkUpdateReceived = true;
	}
}

bool AMICharacter::CanStartFloorSlide_Implementation() const
{
	if (!MICharacterMovement)
	{
		return false;
	}

	if (MICharacterMovement->FloorSlideConditions == EMIFloorSlide::FSR_NeverUsed)
	{
		UE_LOG(LogMICharacter, Error, TEXT("CanFloorSlide called but FloorSlide is set to NeverUsed"));
		return false;
	}

	if (MICharacterMovement->FloorSlideConditions == EMIFloorSlide::FSR_Disabled)
	{
		return false;
	}

	if (!bIsCrouched)
	{
		return false;
	}

	// If can't floor slide while in air and are falling, can't floor slide
	if (!MICharacterMovement->bFloorSlideCanStartFromAir && MICharacterMovement->IsFalling())
	{
		return false;
	}

	// On cooldown
	if (MICharacterMovement->FloorSlideCooldownTime > 0.f && GetWorld()->GetTimeSeconds() < FloorSlideStopTime + MICharacterMovement->FloorSlideCooldownTime)
	{
		return false;
	}

	const bool bSprinting = bIsSprinting && GetVelocity().Size2D() >= (MICharacterMovement->GetMaxSpeed() * 0.9f);
	const bool bTooSlow = GetVelocity().Size2D() <= MICharacterMovement->FloorSlideMinStartSpeed;
	const bool bSprintNotLongEnough = MICharacterMovement->FloorSlideMinConditionDuration > 0.f && GetWorld()->GetTimeSeconds() < SprintStartTime + MICharacterMovement->FloorSlideMinConditionDuration;
	const bool bSpeedNotLongEnough = MICharacterMovement->FloorSlideMinConditionDuration > 0.f && GetWorld()->GetTimeSeconds() < FloorSlideSpeedStartTime + MICharacterMovement->FloorSlideMinConditionDuration;

	const EMIFloorSlide& Req = MICharacterMovement->FloorSlideConditions;
	switch (Req)
	{
	case EMIFloorSlide::FSR_Sprinting:
		if (!bSprinting)
		{
			return false;
		}
		if (bSprintNotLongEnough)
		{
			return false;
		}
		break;
	case EMIFloorSlide::FSR_SpeedThreshold:
		if (bTooSlow)
		{
			return false;
		}
		if (bSpeedNotLongEnough)
		{
			return false;
		}
		break;
	case EMIFloorSlide::FSR_SprintAndSpeedThreshold:
		if (!bSprinting)
		{
			return false;
		}
		if (bTooSlow)
		{
			return false;
		}
		if (bSprintNotLongEnough)
		{
			return false;
		}
		if (bSpeedNotLongEnough)
		{
			return false;
		}
		break;
	default:
		break;
	}

	return true;
}

bool AMICharacter::CanContinueFloorSlide_Implementation() const
{
	if (!MICharacterMovement)
	{
		return false;
	}

	// Moving too slow
	if (GetVelocity().Size2D() < GetMICharacterMovement()->FloorSlideMinSpeed)
	{
		return false;
	}

	// In air and not allowed to slide in air
	if (!MICharacterMovement->bFloorSlideCanContinueInAir && MICharacterMovement->IsFalling())
	{
		return false;
	}

	// Started sprinting
	if (bIsSprinting)
	{
		return false;
	}

	return true;
}

bool AMICharacter::IsFloorSliding() const
{
	return bIsCrouched && CanStartFloorSlide();
}

void AMICharacter::OnStartFloorSlide()
{
	OnCharacterStateChanged();
	K2_OnStartFloorSlide();
}

void AMICharacter::OnStopFloorSlide()
{
	FloorSlideStopTime = GetWorld()->GetTimeSeconds();

	OnCharacterStateChanged();
	K2_OnStopFloorSlide();
}

bool AMICharacter::ShouldAutoFloorSlide_Implementation() const
{
	return false;
}

bool AMICharacter::ShouldCycleMovement_Implementation() const
{
	if (GetMICharacterMovement())
	{
		return MovementSystem == EMIMovementSystem::MS_CycleOrientToMovement;
	}

	return false;
}

FOrientToFloorSettings AMICharacter::GetOrientToFloorSettings_Implementation() const
{
	if (IsFloorSliding())
	{
		return OrientMatchGround;
	}

	return OrientDefaultSettings;
}

bool AMICharacter::ShouldOrientToFloor_Implementation() const
{
	return IsOnWalkableFloor() && !IsCurrentFloorMovable();
}

void AMICharacter::OnRep_IsFloorSliding()
{
	// Replication for simulated proxies to change floor slide state
	if (MICharacterMovement)
	{
		if (bIsFloorSliding)
		{
			MICharacterMovement->StartFloorSlide(true);
		}
		else
		{
			MICharacterMovement->StopFloorSlide(true);
		}
		MICharacterMovement->bNetworkUpdateReceived = true;
	}
}

bool AMICharacter::IsOnWalkableFloor() const
{
	if (GetMICharacterMovement())
	{
		return GetMICharacterMovement()->CurrentFloor.IsWalkableFloor();
	}
	return false;
}

bool AMICharacter::IsCurrentFloorMovable() const
{
	if (GetMICharacterMovement() && GetMICharacterMovement()->CurrentFloor.HitResult.GetComponent())
	{
		return GetMICharacterMovement()->CurrentFloor.HitResult.GetComponent()->Mobility == EComponentMobility::Movable;
	}
	return false;
}

bool AMICharacter::IsValidLOD(const int32& LODThreshold) const
{
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		const int32 LODLevel = GetMesh()->GetPredictedLODLevel();
		const bool bValidLOD = LODThreshold == INDEX_NONE || LODLevel <= LODThreshold;
		return bValidLOD;
	}
	return true;
}

void AMICharacter::OnCapsuleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Characters are handled by CMC HandleImpact
	if (!OtherActor || !OtherActor->IsA(AMICharacter::StaticClass()))
	{
		if (IsValidLOD(ImpactLODThreshold))
		{
			OnCollideWith(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
		}
	}
}

void AMICharacter::OnCollideWith(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!MICharacterMovement)
	{
		return;
	}

	FVector Velocity = MICharacterMovement->GetLastUpdateVelocity();

	// Use the larger velocity
	if (OtherActor && OtherActor->GetVelocity().Size2D() > Velocity.Size2D())
	{
		Velocity = OtherActor->GetVelocity();
	}

	Velocity.Z = 0.f;

	float Speed = Velocity.Size();

	const bool bIsPlayingMontage = (GetMesh()->GetAnimInstance()) ? GetMesh()->GetAnimInstance()->IsAnyMontagePlaying() : false;
	const bool bMontageBlockHit = !bCanHitWallWhileMontagePlaying && bIsPlayingMontage;

	const bool bIsPlayingRootMotion = HasAnyRootMotion();
	const bool bRootMotionBlockHit = !bCanHitWallWhileRootMotionPlaying && bIsPlayingRootMotion;

	const bool bIsHit = bHitWallEnabled && !bMontageBlockHit && !bRootMotionBlockHit && Speed >= HitWallImpactVelocityThreshold && !GetWorldTimerManager().IsTimerActive(HitWallTimerHandle) && !HitImpactPhysics.IsActive();

	if (bIsHit)
	{
		if (!CanHitWall())
		{
			return;
		}
	}
	else
	{
		if (!CanScuffWall())
		{
			return;
		}
	}

	// Scuffing doesn't use an impact velocity
	if (!bIsHit)
	{
		Speed = Velocity.Size();
	}

	const bool bIsScuff = !bIsHit && Speed >= ScuffWallSoundVelocityThreshold;

	if (!bIsHit && !bIsScuff)
	{
		// Can't do anything
		return;
	}

	// Check the up normal, if its under us its better to play a land sound via other means (this is for running into things only)
	const float BelowDot = (-Hit.Normal | GetActorUpVector());
	const float MaxUpNormal = bIsHit ? HitMaxUpNormal : ScuffMaxUpNormal;
	if (!FMath::IsNearlyZero(Hit.Normal.Z, .01f) && BelowDot < MaxUpNormal)
	{
		return;
	}

	// Check the direction of impact is valid
	{
		// Used to check the direction of the impact
		FVector MovementVector = !MICharacterMovement->GetCurrentAcceleration().IsNearlyZero() ? MICharacterMovement->GetCurrentAcceleration() : Velocity;

		// Insufficient information
		if (MovementVector.IsNearlyZero())
		{
			return;
		}

		// Check the facing normal, if its not in our moving direction then do nothing
		const float MovementDot = (-Hit.ImpactNormal | MovementVector.GetSafeNormal2D());
		const float MaxMovementNormal = (bIsHit) ? HitMaxMovementNormal : ScuffMaxMovementNormal;

		if (MovementDot < MaxMovementNormal)
		{
			// Not sufficiently facing wall
			return;
		}
	}

	// Either hit or scuff succeeded

	UMIPhysicalMaterial* PhysMat = Hit.PhysMaterial.IsValid() ? Cast<UMIPhysicalMaterial>(Hit.PhysMaterial.Get()) : nullptr;
	if (!PhysMat) { PhysMat = DefaultPhysicalMaterial; }
	if (GetPhysicalMaterialOverride())	{ PhysMat = GetPhysicalMaterialOverride(); }

	if (PhysMat)
	{
		// Play sound effect
		USoundBase* const SoundToPlay = bIsHit ? PhysMat->GetHitSound(this) : PhysMat->GetScuffSound(this);
		if (SoundToPlay)
		{
			const FRuntimeFloatCurve& VolumeCurve = bIsHit ? PhysMat->HitVelocityToVolume : PhysMat->ScuffVelocityToVolume;
			const FRuntimeFloatCurve& PitchCurve = bIsHit ? PhysMat->HitVelocityToPitch : PhysMat->ScuffVelocityToPitch;
			const float Volume = VolumeCurve.GetRichCurveConst()->Eval(Speed);
			const float Pitch = PitchCurve.GetRichCurveConst()->Eval(Speed);

			if (Volume > 0.f && Pitch > 0.f)
			{
				if (bIsHit)
				{
					// Modify existing volume if it has increased
					// This is required because initial impact doesn't always have highest velocity
					HitWallAudioComponent->SetVolumeMultiplier(Volume);
					HitWallAudioComponent->SetPitchMultiplier(Pitch);
					if (!HitWallAudioComponent->IsPlaying())
					{
						HitWallAudioComponent->SetSound(SoundToPlay);
						HitWallAudioComponent->Play();
					}
				}
				else if (!GetWorldTimerManager().IsTimerActive(ScuffWallSoundTimerHandle))
				{
					UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, Hit.ImpactPoint, Volume, Pitch);

					if (ScuffWallSoundMinInterval > 0.f)
					{
						GetWorldTimerManager().SetTimer(ScuffWallSoundTimerHandle, ScuffWallSoundMinInterval, false);
					}
				}
			}
		}

		// Determine correct particle effect to play
		UParticleSystem* const ParticleToPlay = bIsHit ? PhysMat->GetHitParticle(this) : PhysMat->GetScuffParticle(this);
		UNiagaraSystem* const NiagaraToPlay = bIsHit ? PhysMat->GetHitNiagara(this) : PhysMat->GetScuffNiagara(this);
		const bool bNiagara = PhysMat->ParticleSystemType == EMIParticleSystemType::MIPST_Niagara;

		// Play particle effect
		if ((bNiagara && NiagaraToPlay) || (!bNiagara && ParticleToPlay))
		{
			const bool bTimerOn = !bIsHit && GetWorldTimerManager().IsTimerActive(ScuffWallParticleTimerHandle);
			if (!bTimerOn)
			{
				const FVector WallNormal = (FVector::UpVector ^ Hit.ImpactNormal) ^ Hit.ImpactNormal;

				const FTransform ParticleTransform(WallNormal.Rotation(), Hit.ImpactPoint, FVector::OneVector);

				if (bNiagara)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NiagaraToPlay, ParticleTransform.GetLocation(), ParticleTransform.Rotator());
				}
				else
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleToPlay, ParticleTransform, true, EPSCPoolMethod::AutoRelease);
				}

				if (ScuffWallParticleMinInterval > 0.f)
				{
					GetWorldTimerManager().SetTimer(ScuffWallParticleTimerHandle, ScuffWallParticleMinInterval, false);
				}
			}
		}
	}

	if (bIsHit)
	{
		// Play physics animation
		HandleMeshImpact(HitImpactPhysics, Hit.ImpactNormal, Velocity);

		GetWorldTimerManager().SetTimer(HitWallTimerHandle, HitWallMinInterval, false);
		GetWorldTimerManager().SetTimer(HitWallSoundTimerHandle, 0.1f, false);
		GetWorldTimerManager().SetTimer(HitWallParticleTimerHandle, 0.1f, false);

		if (OnHitWall.IsBound())
		{
			OnHitWall.Broadcast(Velocity, Hit.ImpactNormal);
		}
	}
	else
	{
		if (OnScuffWall.IsBound())
		{
			OnScuffWall.Broadcast(Velocity, Hit.ImpactNormal);
		}
	}
}

void AMICharacter::HandleMeshImpact(FPhysicsBlend& ImpactPhysics, const FVector& ImpactNormal, const FVector& ImpactVelocity)
{
	ImpactPhysics.Impact(GetMesh(), ImpactNormal, ImpactVelocity.Size());
}

FVector RoundDirectionVector(FVector InVector)
{
	// Match FVector_NetQuantize10 (1 decimal place of precision).
	InVector.X = FMath::RoundToFloat(InVector.X * 10.f) / 10.f;
	InVector.Y = FMath::RoundToFloat(InVector.Y * 10.f) / 10.f;
	InVector.Z = FMath::RoundToFloat(InVector.Z * 10.f) / 10.f;
	return InVector;
}

void AMICharacter::HandleImpactCharacter(AMICharacter* OtherCharacter, const FVector& ImpactNormal, const FVector& ImpactVelocity, bool bInstigator)
{
	const bool bIsPlayingMontage = (GetMesh()->GetAnimInstance()) ? GetMesh()->GetAnimInstance()->IsAnyMontagePlaying() : false;
	const bool bMontageBlockHit = !bCanHitCharacterWhileMontagePlaying && bIsPlayingMontage;

	const bool bIsPlayingRootMotion = HasAnyRootMotion();
	const bool bRootMotionBlockHit = !bCanHitCharacterWhileRootMotionPlaying && bIsPlayingRootMotion;

	if (!bHitCharacterEnabled || bMontageBlockHit || bRootMotionBlockHit || !CanHitCharacter())
	{
		return;
	}

	const FVector NetImpactNormal = RoundDirectionVector(ImpactNormal);

	if (bInstigator)
	{
		if (ImpactVelocity.Size() < HitCharacterVelocityThreshold)
		{
			return;
		}
	}
	else
	{
		if (ImpactVelocity.Size() < HitByCharacterVelocityThreshold)
		{
			return;
		}
	}

	const FVector Velocity = bInstigator ? ImpactVelocity : OtherCharacter->GetMICharacterMovement()->GetLastUpdateVelocity();

	if (bInstigator)
	{
		if (!GetWorldTimerManager().IsTimerActive(HitCharacterTimerHandle))
		{
			GetWorldTimerManager().SetTimer(HitCharacterTimerHandle, HitCharacterMinInterval, false);

			HandleMeshImpact(HitCharacterImpactPhysics, ImpactNormal, ImpactVelocity);
		}
	}
	else
	{
		if (!GetWorldTimerManager().IsTimerActive(HitByCharacterTimerHandle))
		{
			GetWorldTimerManager().SetTimer(HitByCharacterTimerHandle, HitByCharacterMinInterval, false);

			HandleMeshImpact(HitByCharacterImpactPhysics, ImpactNormal, ImpactVelocity);
		}
	}

	USoundBase*& SoundToPlay = HitCharacterGrunt;

	if (!bInstigator)
	{
		if (GetWorldTimerManager().IsTimerActive(HitByCharacterVoiceTimerHandle) || HitByCharacterVoice == nullptr)
		{
			SoundToPlay = HitByCharacterGrunt;
		}
		// If getting hit, can play a voice line every once in a while
		else if (CanPlayHitByCharacterVoice())
		{
			SoundToPlay = HitByCharacterVoice;
			GetWorldTimerManager().SetTimer(HitByCharacterVoiceTimerHandle, HitByCharacterMinVoiceInterval, false);
		}
	}

	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
	}

	if (OnHitCharacter.IsBound())
	{
		OnHitCharacter.Broadcast(OtherCharacter, ImpactVelocity, ImpactNormal);
	}
}

bool AMICharacter::CanPlayHitByCharacterVoice_Implementation() const
{
	return true;
}
