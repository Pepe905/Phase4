// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MIViewComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Pawn.h"
#include "MICharacter.h"


UMIViewComponent::UMIViewComponent()
	: bHandleBaseEyeHeight(true)
	, CurrentState(NAME_None)
	, PreviousState(NAME_None)
	, HalfHeight(0.f)
	, BaseEyeHeight(0.f)
	, StateViewOffset(FVector::ZeroVector)
	, StartStateViewOffset(FVector::ZeroVector)
{
	PrimaryComponentTick.bCanEverTick = true;

	StateBlend.SetAlpha(1.f);
	CameraBlend.SetAlpha(1.f);

	FMICharacterState& CrouchState = CharacterStates.Add(TEXT("Crouch"), FMICharacterState());
	FMICharacterState& SprintState = CharacterStates.Add(TEXT("Sprint"), FMICharacterState());
	FMICharacterState& FloorSlideState = CharacterStates.Add(TEXT("FloorSlide"), FMICharacterState());
	FMICharacterState& CrouchRunState = CharacterStates.Add(TEXT("CrouchRun"), FMICharacterState());

	CrouchState.ViewOffset = FVector(30.f, 65.f, -32.f);
	CrouchState.SetBlendTime(0.4f);

	SprintState.ViewOffset = FVector(-100.f, 0.f, 30.f);
	SprintState.SetBlendTime(0.6f);
	SprintState.SetBlendOption(EAlphaBlendOption::Sinusoidal);

	FloorSlideState.ViewOffset = FVector(-200.f, 0.f, 100.f);
	FloorSlideState.SetBlendTime(0.3f);
	FloorSlideState.SetBlendOption(EAlphaBlendOption::Sinusoidal);

	CrouchRunState.ViewOffset = FVector(40.f, 0.f, -32.f);
	CrouchRunState.SetBlendTime(0.5f);
}

void UMIViewComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseCharacterState = FMICharacterState(DefaultCharacterState);

	PawnOwner = GetOwner() ? Cast<APawn>(GetOwner()) : nullptr;
	MICharacterOwner = GetOwner() ? Cast<AMICharacter>(GetOwner()) : nullptr;

	StateViewOffset = BaseCharacterState.ViewOffset;
	BaseEyeHeight = (PawnOwner) ? PawnOwner->BaseEyeHeight : 64.f;

	UpdatedCameras = GetCharacterStateCameras();
	UpdatedCameraOffsets.Empty();
	for (UCameraComponent* const Camera : UpdatedCameras)
	{
		USpringArmComponent* const SpringArm = Camera->GetAttachParent() ? Cast<USpringArmComponent>(Camera->GetAttachParent()) : nullptr;
		if (SpringArm)
		{
			UpdatedCameraOffsets.Add(Camera, SpringArm->SocketOffset);
		}
		else
		{
			UpdatedCameraOffsets.Add(Camera, Camera->GetRelativeTransform().GetLocation());
		}
	}

	if (GetOwner() && GetOwner()->bFindCameraComponentWhenViewTarget)
	{
		// Look for the first active camera component and use that for the view
		TInlineComponentArray<UCameraComponent*> Cameras;
		GetOwner()->GetComponents<UCameraComponent>(/*out*/ Cameras);

		for (UCameraComponent* CameraComponent : Cameras)
		{
			if (CameraComponent->IsActive())
			{
				CurrentCamera = CameraComponent;
				break;
			}
		}
	}
}

void UMIViewComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!StateBlend.IsComplete())
	{
		StateBlend.Update(DeltaTime);

		// Compute offset
		StateViewOffset = FMath::Lerp(StartStateViewOffset, CharacterState().ViewOffset, StateBlend.GetBlendedValue());

		if (bHandleBaseEyeHeight && PawnOwner)
		{
			PawnOwner->RecalculateBaseEyeHeight();

			BaseEyeHeight = FMath::Lerp(StartBaseEyeHeight, PawnOwner->BaseEyeHeight, StateBlend.GetBlendedValue());
			PawnOwner->BaseEyeHeight = BaseEyeHeight;
		}

		// Apply result offset
		for (UCameraComponent* Camera : UpdatedCameras)
		{
			if (Camera)
			{
				FVector HHeight = GetHalfHeight();
				USpringArmComponent* const SpringArm = (Camera && Camera->GetAttachParent()) ? Cast<USpringArmComponent>(Camera->GetAttachParent()) : nullptr;

				// Must move the spring arm instead of the camera if present, or it will not account for collision
				if (SpringArm)
				{
					SpringArm->SocketOffset = UpdatedCameraOffsets[Camera] + StateViewOffset;
				}
				else
				{
					Camera->SetRelativeLocation(UpdatedCameraOffsets[Camera] + StateViewOffset + Camera->GetComponentRotation().UnrotateVector(HHeight));
				}
			}
		}
	}
}

void UMIViewComponent::OnCharacterStateChanged()
{
	const FName OldState = CurrentState;
	CurrentState = NAME_None;

	uint8 StateIndex = 0;
	for (auto& StateItr : CharacterStates)
	{
		if (IsCharacterStateActive(StateIndex))
		{
			PreviousState = CurrentState;
			CurrentState = StateItr.Key;
			break;
		}
		StateIndex++;
	}

	if (OldState != CurrentState)
	{
		StateBlend = CharacterState();
		StateBlend.SetBlendTime(ComputeTransitionTime());

		StartStateViewOffset = StateViewOffset;
		StartBaseEyeHeight = BaseEyeHeight;
	}
}

void UMIViewComponent::OnHalfHeightChanged(float ScaledHalfHeightAdjust)
{
	OnCharacterStateChanged();

	HalfHeight = IsDefaultStateActive() ? 0.f : ScaledHalfHeightAdjust;
}

bool UMIViewComponent::IsCharacterStateActive_Implementation(uint8 State) const
{
	if (MICharacterOwner)
	{
		return MICharacterOwner->IsViewComponentStateActive(State);
	}

	UE_LOG(LogView, Error, TEXT("IsCharacterStateActive requires override."));
	return false;
}

TArray<UCameraComponent*> UMIViewComponent::GetCharacterStateCameras_Implementation() const
{
	if (MICharacterOwner)
	{
		return MICharacterOwner->GetViewComponentStateCameras();
	}

	UE_LOG(LogView, Error, TEXT("GetUpdatedCameras requires override."));
	return TArray<UCameraComponent*>();
}

void UMIViewComponent::SetCamera(UCameraComponent* NewCamera, const FMICameraViewBlend& BlendSettings)
{
	if (!NewCamera)
	{
		return;
	}

	CalcCamera(GetWorld()->GetDeltaSeconds(), StartBlendView);
	const float RemainingAlpha = CameraBlend.GetBlendedValue();
	CameraBlend = BlendSettings;

	if (PendingCamera)
	{
		// Blend to the new camera
		if (BlendSettings.bScaleTimeOverViewDistance)
		{
			CameraBlend.SetBlendTime(BlendSettings.GetTimeScaledOverDistance(StartBlendView.Location, NewCamera->GetComponentLocation()));
		}
		else
		{
			CameraBlend.SetAlpha(1.f - RemainingAlpha);
			CameraBlend.Reset();
		}
	}
	else
	{
		// Set the new camera if not amidst blending to another
		if (BlendSettings.bScaleTimeOverViewDistance)
		{
			CameraBlend.SetBlendTime(BlendSettings.GetTimeScaledOverDistance(StartBlendView.Location, NewCamera->GetComponentLocation()));
		}
	}

	PendingCamera = NewCamera;
}

void UMIViewComponent::K2_SetNewCamera(UCameraComponent* NewCamera, FMICameraViewBlend CameraSettings, float BlendTime, EAlphaBlendOption BlendOption, UCurveFloat* CustomBlendCurve)
{
	if (BlendOption == EAlphaBlendOption::Custom)
	{
		if (ensureMsgf(CustomBlendCurve != nullptr, TEXT("BlendOption set to Custom but no CustomBlendCurve provided. Exiting.")))
		{
			return;
		}
	}
	else
	{
		ensureMsgf(CustomBlendCurve == nullptr, TEXT("CustomBlendCurve will not be used as BlendOption is not set to Custom"));
	}

	CameraSettings.SetBlendTime(BlendTime);
	CameraSettings.SetBlendOption(BlendOption);
	CameraSettings.SetCustomCurve(CustomBlendCurve);
	SetCamera(NewCamera, CameraSettings);
}

void UMIViewComponent::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	if (CurrentCamera)
	{
		if (PendingCamera)
		{
			CameraBlend.Update(DeltaTime);

			if (CameraBlend.IsComplete())
			{
				// Transition completed, update properties and notify
				CurrentCamera = PendingCamera;
				PendingCamera = nullptr;
				CurrentCamera->GetCameraView(DeltaTime, OutResult);
				if (OnCameraChanged.IsBound()) { OnCameraChanged.Broadcast(CurrentCamera); }
			}
			else
			{
				// Camera is transitioning, process view blend
				FMinimalViewInfo NewView = StartBlendView;
				FMinimalViewInfo PendingView;
				PendingCamera->GetCameraView(DeltaTime, PendingView);
				NewView.BlendViewInfo(PendingView, CameraBlend.GetBlendedValue());
				OutResult = NewView;
			}
		}
		else
		{
			// Not transitioning, return current camera
			CurrentCamera->GetCameraView(DeltaTime, OutResult);
		}
	}
	else if (PendingCamera)
	{
		// Edge case where there was no camera during BeginPlay
		CurrentCamera = PendingCamera;
		PendingCamera = nullptr;
		CurrentCamera->GetCameraView(DeltaTime, OutResult);
	}
}

float UMIViewComponent::ComputeTransitionTime() const
{
	FName RelevantStateName = NAME_None;
	if (IsDefaultStateActive())
	{
		RelevantStateName = PreviousState;
	}

	const FVector StartingPoint = StateViewOffset;

	const FVector BaseDist = (GetCharacterState(RelevantStateName).ViewOffset - CharacterState().ViewOffset).GetAbs();
	const FVector NewDist = (StartingPoint - CharacterState().ViewOffset).GetAbs();

	// Apply any properties required for scaling the time depending on the basis selected
	const float ScaleBase = (BaseDist.X + BaseDist.Y + BaseDist.Z) / 3.f;	// The original distance relative to the transition time
	const float ScaleBasis = (NewDist.X + NewDist.Y + NewDist.Z) / 3.f;		// The basis for which we compute the new transition time

	// Now that we have the NewDist we need to scale it based on NewDist*Time/BaseDist
	if (ScaleBase != 0.f)
	{
		if (IsDefaultStateActive())
		{
			return ScaleBasis * (PreviousCharacterState().GetBlendTime() / ScaleBase);
		}
		else
		{
			return ScaleBasis * (CharacterState().GetBlendTime() / ScaleBase);
		}
	}

	return CharacterState().GetBlendTime();
}
