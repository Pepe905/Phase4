// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AlphaBlend.h"
#include "Camera/CameraTypes.h"
#include "MIViewComponent.generated.h"

class UCameraComponent;
class APawn;
class AMICharacter;

DECLARE_LOG_CATEGORY_CLASS(LogView, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMIOnCameraChanged, UCameraComponent*, NewCamera);


USTRUCT(BlueprintType)
struct FMICharacterState : public FAlphaBlend
{
	GENERATED_BODY()

	FMICharacterState()
		: ViewOffset(FVector::ZeroVector)
	{}
	 
	FMICharacterState(const FAlphaBlend& NewBlend)
		: ViewOffset(FVector::ZeroVector)
	{
		FAlphaBlend(NewBlend, NewBlend.GetBlendTime());
	}

	/**
	 * Default Character State will be set to first active spring arm (based on camera that is first used - change this by marking only one camera as "AutoActivate")
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ViewComponent)
	FVector ViewOffset;
};

USTRUCT(BlueprintType)
struct FMICameraViewBlend : public FAlphaBlend
{
	GENERATED_BODY()

	FMICameraViewBlend()
		: bScaleTimeOverViewDistance(true)
		, bIgnoreZDistance(false)
		, DistanceScalar(1000.f)
	{}

	/** Time is affected by how much distance the transition needs to traverse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ViewComponent)
	bool bScaleTimeOverViewDistance;

	/** The distance traversed will ignore the Z (up) axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ViewComponent, meta = (EditCondition = "bScaleTimeByViewDistance"))
	bool bIgnoreZDistance;

	/** Scale the distance to get a desirable transition time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ViewComponent, meta = (EditCondition = "bScaleTimeByViewDistance"))
	float DistanceScalar;

	FORCEINLINE float GetTimeScaledOverDistance(const FVector& CurrentLocation, const FVector& PendingLocation) const
	{
		if (!bScaleTimeOverViewDistance || DistanceScalar == 0.f)
		{
			return GetBlendTime();
		}

		const FVector Difference = (PendingLocation - CurrentLocation);
		const float Distance = (bIgnoreZDistance) ? Difference.Size2D() : Difference.Size();

		return (Distance * (GetBlendTime() / DistanceScalar));
	}
};

/**
 * Handles camera blending and character state changes
 * Allows blending smoothly between any number of cameras
 */
UCLASS(Blueprintable, ClassGroup=(Custom), HideCategories=(Activation, Collision, ComponentReplication), meta=(BlueprintSpawnableComponent, DisplayName = "MIViewComponent") )
class MOVEIT_API UMIViewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** If true, will modify the Pawn's base eye height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ViewComponent)
	bool bHandleBaseEyeHeight;

	/** Usually the character's standing state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ViewComponent)
	FAlphaBlend DefaultCharacterState;

	/**
	 * States available to the character
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ViewComponent)
	TMap<FName, FMICharacterState> CharacterStates;

public:
	/** Delegate for the camera changing */
	UPROPERTY(BlueprintAssignable, Category = ViewComponent)
	FMIOnCameraChanged OnCameraChanged;

protected:
	// Default Character State with no offset
	FMICharacterState BaseCharacterState;

	/** "None" is DefaultState */
	FName CurrentState;
	FName PreviousState;

	FAlphaBlend StateBlend;

	TArray<UCameraComponent*> UpdatedCameras;
	TMap<UCameraComponent*, FVector> UpdatedCameraOffsets;

	float HalfHeight;

	float BaseEyeHeight;
	float StartBaseEyeHeight;

	FVector StateViewOffset;
	FVector StartStateViewOffset;

	APawn* PawnOwner;
	AMICharacter* MICharacterOwner;

protected:
	UCameraComponent* CurrentCamera;
	UCameraComponent* PendingCamera;

	FAlphaBlend CameraBlend;

	/**
	 * Used when starting transition with a transition already in progress to blend views together
	 */
	FMinimalViewInfo StartBlendView;

public:
	UMIViewComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Notify ViewComponent that character state has changed to a new state that it handles */
	UFUNCTION(BlueprintCallable, Category = ViewComponent)
	void OnCharacterStateChanged();

	void OnHalfHeightChanged(float ScaledHalfHeightAdjust);

	/** 
	 * Must return whether each applicable character state is active
	 * @see CharacterStates
	 */
	UFUNCTION(BlueprintNativeEvent, Category = ViewComponent)
	bool IsCharacterStateActive(uint8 State) const;

	/** Must return all cameras used by the character */
	UFUNCTION(BlueprintNativeEvent, Category = ViewComponent)
	TArray<UCameraComponent*> GetCharacterStateCameras() const;

	FORCEINLINE FVector GetViewOffset() const { return StateViewOffset; }
	FORCEINLINE float GetBaseEyeHeightOffset() const { return BaseEyeHeight; }

	FORCEINLINE bool IsDefaultStateActive() const { return CurrentState.IsNone(); }

	FORCEINLINE FMICharacterState& GetCharacterState(const FName& StateName)
	{
		if (!StateName.IsNone() && CharacterStates.Contains(StateName))
		{
			return CharacterStates.FindChecked(StateName);
		}
		return BaseCharacterState;
	}

	FORCEINLINE const FMICharacterState& GetCharacterState(const FName& StateName) const
	{
		if (!StateName.IsNone() && CharacterStates.Contains(StateName))
		{
			return CharacterStates.FindChecked(StateName);
		}
		return BaseCharacterState;
	}

	FORCEINLINE FMICharacterState& CharacterState() { return GetCharacterState(CurrentState); }
	FORCEINLINE const FMICharacterState& CharacterState() const { return GetCharacterState(CurrentState); }
	FORCEINLINE FMICharacterState& PreviousCharacterState() { return GetCharacterState(PreviousState); }
	FORCEINLINE const FMICharacterState& PreviousCharacterState() const { return GetCharacterState(PreviousState); }

	UFUNCTION(BlueprintCallable, Category = ViewComponent, meta = (DisplayName = "Set Camera"))
	void SetCamera(UCameraComponent* NewCamera, UPARAM(ref) const FMICameraViewBlend& CameraSettings);

	UFUNCTION(BlueprintCallable, Category = ViewComponent, meta = (DisplayName = "Set New Camera"))
	void K2_SetNewCamera(UCameraComponent* NewCamera, FMICameraViewBlend CameraSettings, float BlendTime, EAlphaBlendOption BlendOption, UCurveFloat* CustomBlendCurve);

	/**
	* Call this from AActor::CalcCamera (dont call super)
	*/
	void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult);

	/** @return Currently active camera */
	UFUNCTION(BlueprintPure, Category = ViewComponent)
	FORCEINLINE UCameraComponent* GetCurrentCamera() const { return CurrentCamera; }

	/** @return Camera being transitioned to, if a transition is in progress */
	UFUNCTION(BlueprintPure, Category = ViewComponent)
	FORCEINLINE UCameraComponent* GetPendingCamera() const { return PendingCamera; }

protected:
	float ComputeTransitionTime() const;

	FORCEINLINE FVector GetHalfHeight() const { return FVector(0.f, 0.f, HalfHeight); }
};
