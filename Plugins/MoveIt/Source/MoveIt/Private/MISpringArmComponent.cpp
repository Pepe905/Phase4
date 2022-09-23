// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MISpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Logging/MessageLog.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Misc/UObjectToken.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogCamera, Log, All);

#define LOCTEXT_NAMESPACE "MISpringArmComponent"

UMISpringArmComponent::UMISpringArmComponent()
{
	// Orbit
	bEnableOrbit = false;
	InputSource = EMIInputSource::IS_Character;
	MoveForwardAxisName = TEXT("MoveForward");
	MoveRightAxisName = TEXT("MoveRight");
	bScaleByMoveForwardInput = true;
	bOrbitWhenMovingForward = true;
	bOrbitWhenMovingBackward = false;
	WalkForwardMultiplier = 1.f;
	WalkBackwardMultiplier = 1.f;
	OrbitRate = 15.f;
	OrbitInterpRateWithInput = 5.f;
	OrbitInterpRateNoInput = 5.f;

	LastOrbitYaw = 0.f;

	// Z Movement
	bCameraMoveDuringJump = true;
	SwitchZLagThreshold = 2.f;
	bDrawLandingLagState = false;

	JumpZLoc = 0.f;
	bJumped = false;
	bLanding = false;
	JumpLagSpeed = 0.f;

	// Zoom
	ZoomEnabled = EMIZoomType::ZT_Disabled;
	MinTargetLength = 100.f;
	MaxTargetLength = 1000.f;
	ZoomRate = 0.f;

	// PitchToZoomCurve
	{
		const FKeyHandle& FirstKey = PitchToZoomCurve.GetRichCurve()->AddKey(-45.f, 0.1f);
		const FKeyHandle& SecondKey = PitchToZoomCurve.GetRichCurve()->AddKey(0.f, 1.f);
		const FKeyHandle& ThirdKey = PitchToZoomCurve.GetRichCurve()->AddKey(45.f, 1.5f);
		PitchToZoomCurve.GetRichCurve()->SetKeyInterpMode(FirstKey, RCIM_Cubic);
		PitchToZoomCurve.GetRichCurve()->SetKeyInterpMode(SecondKey, RCIM_Cubic);
		PitchToZoomCurve.GetRichCurve()->SetKeyInterpMode(ThirdKey, RCIM_Cubic);
	}
}

void UMISpringArmComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = GetOwner() ? Cast<ACharacter>(GetOwner()) : nullptr;

	DefaultTargetArmLength = TargetArmLength;
	ZoomTargetLength = DefaultTargetArmLength;

	JumpLagSpeed = CameraLagSpeed;
}

#if WITH_EDITOR
bool UMISpringArmComponent::CanEditChange(const FProperty* Property) const
{
	if (Property->GetFName().IsEqual(GET_MEMBER_NAME_CHECKED(UMISpringArmComponent, MinTargetLength)))
	{
		return (ZoomEnabled == EMIZoomType::ZT_Both || ZoomEnabled == EMIZoomType::ZT_Input);
	}
	else if (Property->GetFName().IsEqual(GET_MEMBER_NAME_CHECKED(UMISpringArmComponent, MaxTargetLength)))
	{
		return (ZoomEnabled == EMIZoomType::ZT_Both || ZoomEnabled == EMIZoomType::ZT_Input);
	}
	else if (Property->GetFName().IsEqual(GET_MEMBER_NAME_CHECKED(UMISpringArmComponent, PitchToZoomCurve)))
	{
		return (ZoomEnabled == EMIZoomType::ZT_Both || ZoomEnabled == EMIZoomType::ZT_Curve);
	}
	else if (Property->GetFName().IsEqual(GET_MEMBER_NAME_CHECKED(UMISpringArmComponent, ZoomCurveRate)))
	{
		return (ZoomEnabled == EMIZoomType::ZT_Both || ZoomEnabled == EMIZoomType::ZT_Curve);
	}

	return Super::CanEditChange(Property);
}

void UMISpringArmComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName().IsEqual(GET_MEMBER_NAME_CHECKED(UMISpringArmComponent, bEnableOrbit)))
	{
		if (!bUsePawnControlRotation && bEnableOrbit && GetOwner())
		{
			bUsePawnControlRotation = true;

			FMessageLog("MapCheck").Warning()
				->AddToken(FUObjectToken::Create(GetOwner()))
				->AddToken(FTextToken::Create(LOCTEXT("ControlRotation_Message_EnabledAuto", "bUsePawnControlRotation was automatically enabled because orbit requires it")));
		}
	}
}
#endif  // WITH_EDITOR

void UMISpringArmComponent::OnCharacterJumped()
{
	if (!bCameraMoveDuringJump)
	{
		bJumped = true;
		JumpZLoc = (GetComponentLocation() + TargetOffset).Z;
	}
}

void UMISpringArmComponent::OnCharacterLanded(float ZLagSpeed /* = 10.f */)
{
	if (!bCameraMoveDuringJump || bJumped)
	{
		bJumped = false;
		bLanding = true;
		JumpLagSpeed = (ZLagSpeed >= 0.f) ? ZLagSpeed : CameraLagSpeed;
	}
}

void UMISpringArmComponent::ZoomIn(float StepSize /* = 100.f */, float NewZoomRate /* = 100.f */)
{
	Zoom(true, StepSize, NewZoomRate);
}

void UMISpringArmComponent::ZoomOut(float StepSize /* = 100.f */, float NewZoomRate /* = 100.f */)
{
	Zoom(false, StepSize, NewZoomRate);
}

void UMISpringArmComponent::Zoom(bool bZoomIn, float StepSize, float NewZoomRate)
{
	switch (ZoomEnabled)
	{
	case EMIZoomType::ZT_Both:
	case EMIZoomType::ZT_Input:
		ZoomRate = NewZoomRate;

		ZoomTargetLength = (bZoomIn) ? (ZoomTargetLength - StepSize) : (ZoomTargetLength + StepSize);
		ZoomTargetLength = FMath::Clamp(ZoomTargetLength, MinTargetLength, MaxTargetLength);
		return;
	case EMIZoomType::ZT_Curve:
		return;
	default:
		ZoomTargetLength = DefaultTargetArmLength;
		return;
	}
}

bool UMISpringArmComponent::IsFalling() const
{
	if (GetCharacterMovement())
	{
		return GetCharacterMovement()->IsFalling();
	}
	return false;
}

bool UMISpringArmComponent::IsOnGround() const
{
	if (GetCharacterMovement())
	{
		return GetCharacterMovement()->IsMovingOnGround();
	}
	return true;
}

void UMISpringArmComponent::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	if (!CharacterOwner)
	{
		Super::UpdateDesiredArmLocation(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
		return;
	}

	// For player characters this just ends up being the control rotation, need to add to it to orbit
	if (bEnableOrbit)
	{
		if (bUsePawnControlRotation && CharacterOwner->GetController())
		{
			const float RightInput = (InputSource == EMIInputSource::IS_Character) ? CharacterOwner->GetInputAxisValue(MoveRightAxisName) : CharacterOwner->GetController()->GetInputAxisValue(MoveRightAxisName);
			float ForwardInput = (InputSource == EMIInputSource::IS_Character) ? CharacterOwner->GetInputAxisValue(MoveForwardAxisName) : CharacterOwner->GetController()->GetInputAxisValue(MoveForwardAxisName);

			ForwardInput /= 2.f;

			const bool bWalkForward = ForwardInput >= 0.1f;
			const bool bWalkBackward = ForwardInput < -0.1f;

			if (ForwardInput >= 0.1f && WalkForwardMultiplier != 0.f) { ForwardInput /= WalkForwardMultiplier; }
			if (ForwardInput < -0.1f && WalkBackwardMultiplier != 0.f) { ForwardInput *= WalkBackwardMultiplier; }

			const bool bAbortMovingForwards = ForwardInput >= 0.1f && !bOrbitWhenMovingForward;
			const bool bAbortMovingBackwards = ForwardInput < -0.1f && !bOrbitWhenMovingBackward;

			float OrbitInput = 0.f;
			if (!bAbortMovingForwards && !bAbortMovingBackwards)
			{
				OrbitInput = (bScaleByMoveForwardInput) ? RightInput * (1.f - FMath::Abs(ForwardInput)) : RightInput;
			}

			// Round to 2 decimal places of precision
			OrbitInput = FMath::RoundToFloat(OrbitInput * 100.f) / 100.f;

			float OrbitYaw = OrbitRate * OrbitInput * DeltaTime;

			// Interpolate yaw
			const float InterpRate = (FMath::IsNearlyZero(OrbitInput)) ? OrbitInterpRateNoInput : OrbitInterpRateWithInput;
			if (InterpRate != 0.f)
			{
				OrbitYaw = FMath::FInterpTo(LastOrbitYaw, OrbitYaw, DeltaTime, InterpRate);

				// Eventually find absolute end points
				if (FMath::IsNearlyZero(OrbitYaw))
				{
					OrbitYaw = 0.f;
				}
				else if (FMath::IsNearlyEqual(OrbitYaw, 1.f))
				{
					OrbitYaw = 1.f;
				}
			}

			CharacterOwner->AddControllerYawInput(OrbitYaw);

			LastOrbitYaw = OrbitYaw;
		}
		// else { Can't orbit otherwise... }
	}

	FRotator DesiredRot = GetTargetRotation();

	// Zoom Curve Multiplier
	if (ZoomEnabled == EMIZoomType::ZT_Curve || ZoomEnabled == EMIZoomType::ZT_Both)
	{
		const float ZoomMultiplierTarget = (ZoomEnabled == EMIZoomType::ZT_Curve || ZoomEnabled == EMIZoomType::ZT_Both) ? PitchToZoomCurve.GetRichCurveConst()->Eval(-DesiredRot.GetNormalized().Pitch) : 1.f;
		CurrentZoomMultiplier = FMath::FInterpTo(CurrentZoomMultiplier, ZoomMultiplierTarget, DeltaTime, ZoomCurveRate);

		// Squash the interpolation when values are too small to notice
		if (FMath::IsNearlyEqual(CurrentZoomMultiplier, ZoomMultiplierTarget, 0.01f))
		{
			CurrentZoomMultiplier = ZoomMultiplierTarget;
		}
	}
	else
	{
		CurrentZoomMultiplier = 1.f;
	}

	// Zoom Input
	if (!FMath::IsNearlyEqual(TargetArmLength, DefaultTargetArmLength) || !FMath::IsNearlyEqual(ZoomTargetLength * CurrentZoomMultiplier, TargetArmLength))
	{
		if (ZoomEnabled == EMIZoomType::ZT_Disabled)
		{
			// Don't keep triggering second condition if we aren't using zoom
			ZoomTargetLength = DefaultTargetArmLength;
		}

		const float ZoomTarget = CurrentZoomMultiplier * ((ZoomEnabled != EMIZoomType::ZT_Disabled) ? ZoomTargetLength : DefaultTargetArmLength);
		TargetArmLength = FMath::FInterpTo(TargetArmLength, ZoomTarget, DeltaTime, ZoomRate);

		// Squash the interpolation when values are too small to notice
		if (FMath::IsNearlyEqual(TargetArmLength, ZoomTarget, 0.01f))
		{
			TargetArmLength = ZoomTarget;
		}
	}

	// Apply 'lag' to rotation if desired
	if (bDoRotationLag)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
		{
			const FRotator ArmRotStep = (DesiredRot - PreviousDesiredRot).GetNormalized() * (1.f / DeltaTime);
			FRotator LerpTarget = PreviousDesiredRot;
			float RemainingTime = DeltaTime;
			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmRotStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(LerpTarget), LerpAmount, CameraRotationLagSpeed));
				PreviousDesiredRot = DesiredRot;
			}
		}
		else
		{
			DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(DesiredRot), DeltaTime, CameraRotationLagSpeed));
		}
	}
	PreviousDesiredRot = DesiredRot;

	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	// We lag the target, not the actual camera position, so rotating the camera around does not have lag
	FVector DesiredLoc = ArmOrigin;

	if (bJumped)
	{
		DesiredLoc.Z = JumpZLoc;
	}

	if (bDoLocationLag || bLanding)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraLagSpeed > 0.f)
		{
			const FVector ArmMovementStep = (DesiredLoc - PreviousDesiredLoc) * (1.f / DeltaTime);
			FVector LerpTarget = PreviousDesiredLoc;

			float RemainingTime = DeltaTime;
			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmMovementStep * LerpAmount;
				RemainingTime -= LerpAmount;

				const float DesiredLocZ = DesiredLoc.Z;

				if (bDoLocationLag)
				{
					DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, LerpTarget, LerpAmount, CameraLagSpeed);
				}

				if (bLanding)
				{
					DesiredLoc.Z = DesiredLocZ;
					DesiredLoc.Z = FMath::FInterpTo(PreviousDesiredLoc.Z, LerpTarget.Z, LerpAmount, JumpLagSpeed);
				}

				PreviousDesiredLoc = DesiredLoc;
			}
		}
		else
		{
			const float DesiredLocZ = DesiredLoc.Z;

			if (bDoLocationLag)
			{
				DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, DesiredLoc, DeltaTime, CameraLagSpeed);
			}

			if (bLanding)
			{
				DesiredLoc.Z = DesiredLocZ;
				DesiredLoc.Z = FMath::FInterpTo(PreviousDesiredLoc.Z, DesiredLoc.Z, DeltaTime, JumpLagSpeed);
			}
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawLandingLagState && bLanding && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(64971, 0.f, FColor::Orange, "Landing");
		}
#endif

		// Clamp distance if requested
		bool bClampedDist = false;
		const FVector FromOrigin = DesiredLoc - ArmOrigin;
		const float LagDistance = FromOrigin.Z;

		if (bLanding && FMath::IsNearlyZero(LagDistance, SwitchZLagThreshold))
		{
			bLanding = false;
		}

		if (CameraLagMaxDistance > 0.f)
		{
			if (FromOrigin.SizeSquared() > FMath::Square(CameraLagMaxDistance))
			{
				DesiredLoc = ArmOrigin + FromOrigin.GetClampedToMaxSize(CameraLagMaxDistance);
				bClampedDist = true;
			}
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugLagMarkers)
		{
			DrawDebugSphere(GetWorld(), ArmOrigin, 5.f, 8, FColor::Green);
			DrawDebugSphere(GetWorld(), DesiredLoc, 5.f, 8, FColor::Yellow);

			const FVector ToOrigin = ArmOrigin - DesiredLoc;
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc, DesiredLoc + ToOrigin * 0.5f, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc + ToOrigin * 0.5f, ArmOrigin, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
		}
#endif
	}

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	// Now offset camera position back along our rotation
	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	// Do a sweep to ensure we are not penetrating the world
	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f))
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		UnfixedCameraPosition = DesiredLoc;

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);

		if (ResultLoc == DesiredLoc)
		{
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	// Form a transform for new world transform for camera
	FTransform WorldCamTM(DesiredRot, ResultLoc);
	// Convert to relative to component
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();
}

const UCharacterMovementComponent* UMISpringArmComponent::GetCharacterMovement() const
{
	if (CharacterOwner)
	{
		return CharacterOwner->GetCharacterMovement();
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE