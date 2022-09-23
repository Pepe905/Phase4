// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "Curves/CurveFloat.h"
#include "MISpringArmComponent.generated.h"

class UCameraComponent;
class ACharacter;
class UCharacterMovementComponent;

UENUM(BlueprintType)
enum class EMIMovementOrbitType : uint8
{
	MOT_Disabled				UMETA(DisplayName = "Disabled", ToolTip = "Do not orbit"),
	MOT_Acceleration			UMETA(DisplayName = "Acceleration", ToolTip = "Orbit based on input"),
	MOT_Velocity				UMETA(DisplayName = "Velocity", ToolTip = "Orbit based on velocity"),
	MOT_MAX = 255				UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMIOrbitVector : uint8
{
	OD_RightInput				UMETA(DisplayName = "Right Input", ToolTip = "Orbit to the direction of right input"),
	OD_FacingDirection			UMETA(DisplayName = "Facing Direction", ToolTip = "Orbit to the direction the character is facing"),
	OD_MAX = 255				UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMIZoomType : uint8
{
	ZT_Disabled					UMETA(DisplayName = "Disabled", ToolTip = "Can not Zoom"),
	ZT_Input					UMETA(DisplayName = "Input", ToolTip = "Zoom using input"),
	ZT_Curve					UMETA(DisplayName = "Curve", ToolTip = "Zoom using curve"),
	ZT_Both						UMETA(DisplayName = "Input and Curve", ToolTip = "Zoom using input and curve"),
	ZT_MAX = 255				UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EMIInputSource : uint8
{
	IS_Character				UMETA(DisplayName = "Character", ToolTip = "Get Input from the Character"),
	IS_PlayerController			UMETA(DisplayName = "Player Controller", ToolTip = "Get Input from the PlayerController")
};

/**
 * 
 */
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent), hideCategories = (Mobility))
class MOVEIT_API UMISpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()
	
public:
	// Orbit

	/** Make sure to look at example character to see how to use this - it will affect every spring arm otherwise! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	bool bEnableOrbit;

	/** Whether to get the input from the character or the controller - if you are getting warning spam then you might need to change this! */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	EMIInputSource InputSource;

	/** The name of the input used to move forward */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	FName MoveForwardAxisName;

	/** The name of the input used to move right */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	FName MoveRightAxisName;

	/** If true, applying forward input dials back influence by right input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	bool bScaleByMoveForwardInput;
	
	/** If true, can orbit when moving forward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	bool bOrbitWhenMovingForward;

	/** If true, can orbit when moving backward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	bool bOrbitWhenMovingBackward;

	/** Multiplier for walking forward to increase the scaling for bScaleByMoveForwardInput */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	float WalkForwardMultiplier;

	/** Multiplier for walking forward to increase the scaling for bScaleByMoveForwardInput (except when moving backwards) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	float WalkBackwardMultiplier;

	/** Orbit moving camera around character at this rate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	float OrbitRate;

	/** How fast orbit direction and speed changes when moving (has input). 0 for instant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	float OrbitInterpRateWithInput;

	/** How fast orbit direction and speed changes when not moving (no input). 0 for instant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Orbit")
	float OrbitInterpRateNoInput;

public:
	// Static Jump

	/** If true won't move camera when character jumps unless delay passes and still going up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Static Jump")
	bool bCameraMoveDuringJump;

	/** How close the lag needs to be to matching the Z height before it reverts to default Z lag */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, meta = (EditCondition = "!bCameraMoveDuringJump"), Category = "MoveItCamera|Static Jump")
	float SwitchZLagThreshold;

	/** If true will tell you in upper-left if it is in the landing lag state in orange */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bCameraMoveDuringJump"), Category = "MoveItCamera|Static Jump")
	uint32 bDrawLandingLagState : 1;

public:
	// Zoom

	/** If true will allow camera zoom functionality */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Zoom")
	EMIZoomType ZoomEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Zoom", meta = (UIMin = "0", ClampMin = "0"))
	float MinTargetLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Zoom", meta = (UIMin = "0", ClampMin = "0"))
	float MaxTargetLength;

	/** Multiplier for TargetArmLength using pitch->zoom */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Zoom")
	FRuntimeFloatCurve PitchToZoomCurve;

	/** The rate at which PitchToZoomCurve is applied (0 to disable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoveItCamera|Zoom", meta = (UIMin = "0", ClampMin = "0"))
	float ZoomCurveRate;

protected:
	UPROPERTY()
	ACharacter* CharacterOwner;

	float LastOrbitYaw;

	float DefaultTargetArmLength;
	float ZoomTargetLength;
	float ZoomRate;
	float CurrentZoomMultiplier;

	// Z Movement

	bool bJumped;
	bool bLanding;

	float JumpZLoc;
	float JumpLagSpeed;

public:
	UMISpringArmComponent();

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* Property) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif  // WITH_EDITOR

	// Z Movement

	/** Tells the system the character jumped, required for Z Movement settings. Will enable camera lag! Settings are only updated if on ground */
	UFUNCTION(BlueprintCallable, Category = MoveItCamera)
	void OnCharacterJumped();

	/** 
	 * Tells the system the character landed, required for Z Movement settings. Will interpolate away camera lag if it wasn't originally enabled! 
	 * @param ZLagSpeed: How fast the lag moves on Z when landing
	 */
	UFUNCTION(BlueprintCallable, Category = MoveItCamera)
	void OnCharacterLanded(float ZLagSpeed /* = 10.f */);

	// Zoom

	UFUNCTION(BlueprintCallable, Category = MoveItCamera)
	void ZoomIn(float StepSize = 100.f, float ZoomRate = 100.f);

	UFUNCTION(BlueprintCallable, Category = MoveItCamera)
	void ZoomOut(float StepSize = 100.f, float ZoomRate = 100.f);

	void Zoom(bool bZoomIn, float StepSize, float ZoomRate);

protected:
	// Helpers

	bool IsFalling() const;
	bool IsOnGround() const;

public:
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;

protected:
	const UCharacterMovementComponent* GetCharacterMovement() const;
};
