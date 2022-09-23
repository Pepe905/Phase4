// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MITypes.h"
#include "Curves/CurveFloat.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "MICharacterMovementComponent.generated.h"

class AMICharacter;

UENUM(BlueprintType)
enum class EMIPivotType : uint8
{
	PT_Velocity						UMETA(DisplayName = "Velocity", ToolTip = "Pivot based on Velocity (and turning camera can cause pivot)"),
	PT_Acceleration					UMETA(DisplayName = "Acceleration", ToolTip = "Pivot based on input (camera can not result in pivot"),
	PT_Disabled						UMETA(DisplayName = "Disabled", ToolTip = "Pivot disabled"),
};

UENUM(BlueprintType)
enum class EMIFloorSlide : uint8
{
	FSR_Sprinting					UMETA(DisplayName = "Sprinting", ToolTip = "Must be sprinting to floor slide"),
	FSR_SpeedThreshold				UMETA(DisplayName = "Speed Threshold", ToolTip = "Must exceed speed threshold to floor slide"),
	FSR_SprintAndSpeedThreshold		UMETA(DisplayName = "Sprinting and Speed Threshold", ToolTip = "Must be sprinting and exceeding speed threshold to floor slide"),
	FSR_Disabled					UMETA(Hidden, DisplayName = "Currently Disabled", ToolTip = "Floor slide is currently disabled"),
	FSR_NeverUsed					UMETA(DisplayName = "Never Used", ToolTip = "Floor slide is never used"),
};


/**
 * Adds advanced movement features to CharacterMovementComponent
 */
UCLASS()
class MOVEIT_API UMICharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	/**
	 * How fast the character can turn (results in a minimum turning radius when above MinTurningRadiusSpeedPct)
	 * Can help provide more realistic motion when moving around while oriented to camera
	 * Set to 0.0 to disable turning radius
	 */
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	float TurningRadiusRate;

	/** 
	 * Must be moving faster than this percentage of current max speed to turn in a radius
	 * It is advisable to use this, because if its disabled then turning radius will always apply and make avoiding obstacles you're walking into very difficult
	 */
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	float MinTurningRadiusSpeedPct;

	/** 
	 * Ground Normal [Time] -> Speed Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve UphillSpeedMultiplier;

	/** 
	 * Ground Normal [Time] -> Speed Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve DownhillSpeedMultiplier;

	/** 
	 * Ground Normal [Time] -> Acceleration Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve UphillAccelerationMultiplier;

	/** 
	 * Ground Normal [Time] -> Acceleration Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve DownhillAccelerationMultiplier;

	/** 
	 * Ground Normal [Time] -> Braking Deceleration Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve UphillBrakingDecelerationMultiplier;

	/** 
	 * Ground Normal [Time] -> Braking Deceleration Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve DownhillBrakingDecelerationMultiplier;

	/** 
	 * Ground Normal [Time] -> Friction Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve UphillGroundFrictionMultiplier;

	/** 
	 * Ground Normal [Time] -> Friction Multiplier [Value]
	 * 0 is flat ground
	 * -1 is vertical uphill
	 * 1 is vertical downhill
	 */
	UPROPERTY(Category = "Character Movement: Walking|Slope", EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve DownhillGroundFrictionMultiplier;

	UPROPERTY(Category = "Character Movement: Walking|Cycle", EditAnywhere, BlueprintReadWrite)
	bool bCyclePivotChangesDirection;

	UPROPERTY(Category = "Character Movement: Walking|Cycle", EditAnywhere, BlueprintReadWrite)
	bool bCycleUseRotationRate;

	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	EMIPivotType PivotType;

	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "15", UIMin = "15"))
	bool bPivotUseMaxSpeedMultiplier;
	
	UPROPERTY(Category = "Character Movement: Walking|Ledge", EditAnywhere, BlueprintReadWrite)
	bool bForceWalkOffLedge;

	UPROPERTY(Category = "Character Movement: Walking|Ledge", EditAnywhere, BlueprintReadWrite)
	bool bForceWalkOntoLedge;

	/**
	 * If true, BrakingFriction will be used to slow the character to a stop (when there is no Acceleration).
	 * If false, braking uses the same friction passed to CalcVelocity() (ie GroundFriction when walking), multiplied by BrakingFrictionFactor.
	 * This setting applies to all movement modes; if only desired in certain modes, consider toggling it when movement modes change.
	 * @see BrakingFriction
	 */
	UPROPERTY(Category="Character Movement (General Settings)", EditDefaultsOnly, BlueprintReadWrite)
	uint8 bUseSeparateBrakingFrictionInAir:1;


	/**
	 * If true, when input is released will continue rotating in that direction 
	 * Only applied if bOrientRotationToMovement is true
	 */
	UPROPERTY(Category = "Character Movement (Rotation Settings)", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bOrientRotationToMovement"))
	bool bRotateToLastInputDirection;

	UPROPERTY(Category = "Character Movement (Rotation Settings)", EditAnywhere, BlueprintReadWrite)
	bool bUseSeparateIdleRotationRate;

	/** What conditions are required to floor slide */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere)
	EMIFloorSlide FloorSlideConditions;

	/** If true, can initiate floor sliding while in air */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, meta = (EditCondition = "FloorSlideConditions != EMIFloorSlide::FSR_NeverUsed"))
	bool bFloorSlideCanStartFromAir;
	
	/** If true, can continue floor sliding while in air */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, meta = (EditCondition = "FloorSlideConditions != EMIFloorSlide::FSR_NeverUsed"))
	bool bFloorSlideCanContinueInAir;
	
	/** Speed boosts only apply if we were sprinting */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	bool bFloorSlideGroundSpeedBoostOnlyFromSprint;
	
	/** If true, Character can walk off a ledge when crouching. */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	uint8 bCanWalkOffLedgesWhenFloorSliding:1;

public:
	/** If true, try to sprint (or keep sprinting) on next update. If false, try to stop sprinting on next update. */
	UPROPERTY(Category = "Character Movement: Walking|Sprint", VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bWantsToSprint : 1;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	uint8 bPivot : 1;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bPivotAnimTrigger;

protected:
	UPROPERTY()
	bool bCycle;
	
	/** Previously actually sprinting (not just using input) */
	bool bWasSprintingState;

public:
	/** Speed loss or gain when moving backwards. */
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	float MoveBackwardsSpeedMultiplier;

	/** Speed loss or gain when moving backwards while crouching. */
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	float MoveBackwardsCrouchSpeedMultiplier;

	/** Speed loss or gain when moving backwards while sprinting. */
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	float MoveBackwardsSprintSpeedMultiplier;

	UPROPERTY(Category = "Character Movement: Walking|Cycle", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bCycleUseRotationRate", ClampMin = "0", UIMin = "0"))
	float CycleRotationRate;

	UPROPERTY(Category = "Character Movement (Rotation Settings)", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseSeparateIdleRotationRate"))
	FRotator IdleRotationRate;

	/** The maximum ground speed when sprinting. */
	UPROPERTY(Category="Character Movement: Walking|Sprint", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxWalkSpeedSprinting;
	
	/** Max Acceleration (rate of change of velocity) */
	UPROPERTY(Category="Character Movement: Walking|Sprint", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxAccelerationSprinting;

	/**
	 * Deceleration when walking and not applying acceleration. This is a constant opposing force that directly lowers velocity by a constant value.
	 * @see GroundFriction, MaxAcceleration
	 */
	UPROPERTY(Category="Character Movement: Walking|Sprint", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationSprinting;

	/**
	 * Setting that affects movement control. Higher values allow faster changes in direction.
	 * If bUseSeparateBrakingFriction is false, also affects the ability to stop more quickly when braking (whenever Acceleration is zero), where it is multiplied by BrakingFrictionFactor.
	 * When braking, this property allows you to control how much friction is applied when moving across the ground, applying an opposing force that scales with current velocity.
	 * This can be used to simulate slippery surfaces such as ice or oil by changing the value (possibly based on the material pawn is standing on).
	 * @see BrakingDecelerationWalking, BrakingFriction, bUseSeparateBrakingFriction, BrakingFrictionFactor
	 */
	UPROPERTY(Category="Character Movement: Walking|Sprint", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float GroundFrictionSprinting;

	/** The maximum ground speed when crouch running. */
	UPROPERTY(Category="Character Movement: Walking|Crouch Run", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxWalkSpeedCrouchRun;
	
	/** Max Acceleration (rate of change of velocity) */
	UPROPERTY(Category="Character Movement: Walking|Crouch Run", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxAccelerationCrouchRun;

	/**
	 * Deceleration when crouch running and not applying acceleration. This is a constant opposing force that directly lowers velocity by a constant value.
	 * @see GroundFriction, MaxAcceleration
	 */
	UPROPERTY(Category="Character Movement: Walking|Crouch Run", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationCrouchRun;

	/**
	 * Setting that affects movement control. Higher values allow faster changes in direction.
	 * If bUseSeparateBrakingFriction is false, also affects the ability to stop more quickly when braking (whenever Acceleration is zero), where it is multiplied by BrakingFrictionFactor.
	 * When braking, this property allows you to control how much friction is applied when moving across the ground, applying an opposing force that scales with current velocity.
	 * This can be used to simulate slippery surfaces such as ice or oil by changing the value (possibly based on the material pawn is standing on).
	 * @see BrakingDecelerationWalking, BrakingFriction, bUseSeparateBrakingFriction, BrakingFrictionFactor
	 */
	UPROPERTY(Category="Character Movement: Walking|Crouch Run", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float GroundFrictionCrouchRun;

	/** Minimum velocity vs acceleration difference that is required to pivot. Higher values make it harder to pivot. */
	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "PivotType == EMIPivotType::PT_Velocity"))
	float PivotMinVelocityDifference;

	/** Lower values make it easier to pivot with a controller by only allowing larger values for consideration through. A value that is too low may make it too easy to pivot in general. Best tested with a thumbstick. */
	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.01", UIMin = "0.01", EditCondition = "PivotType == EMIPivotType::PT_Acceleration"))
	float PivotMinInput;

	/** Must be moving this fast (as a percentage of current max speed) to pivot. Values that are too low allow simulated players to pivot from not moving, therefore this will not go below 50.0 */
	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "15", UIMin = "15", EditCondition = "bPivotUseMaxSpeedMultiplier"))
	float PivotMinSpeedPct;
	
	/** Must be moving this fast to pivot. Values that are too low allow simulated players to pivot from not moving, be careful below 50.0 */
	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "15", UIMin = "15", EditCondition = "!bPivotUseMaxSpeedMultiplier"))
	float PivotMinSpeed;

	float PivotMinSpeedTime;

	/** How long character must be at PivotMinSpeed for before they can pivot */
	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float PivotMinSpeedMaintainTime;

	/**
	 * When pivoting, penalty is recovered at this rate
	 * Set to 0.0 to disable
	 */
	UPROPERTY(Category="Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float PivotDuration;

	/**
	 * How long character must wait before they can pivot again
	 */
	UPROPERTY(Category="Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float PivotCooldownTime;

	/**
	 * When pivoting, lock the facing direction for this duration
	 * Not available with OrientToView
	 * Set to 0.0 to disable
	 */
	UPROPERTY(Category="Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float PivotDirectionLockTime;
	
	/**
	 * Lowest speed multiplier when starting a moving turn
	 * Interpolates back to 1.0 using PivotEndRate
	 */
	UPROPERTY(Category="Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float PivotSpeedMultiplier;

	/**
	 * Lowest acceleration multiplier when starting a moving turn
	 * Interpolates back to 1.0 using PivotEndRate
	 */
	UPROPERTY(Category="Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float PivotAccelerationMultiplier;

	/**
	 * Lowest braking deceleration multiplier when starting a moving turn
	 * Interpolates back to 1.0 using PivotEndRate
	 */
	UPROPERTY(Category="Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float PivotBrakingDecelerationMultiplier;
	
	/**
	 * Additional acceleration to apply in the pivot direction when pivot starts
	 * Does not exceed current velocity
	 */
	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float PivotBonusAccelerationOnStart;

	/**
	 * Additional acceleration to apply in the pivot direction when pivot ends
	 * Does not exceed current velocity
	 */
	UPROPERTY(Category = "Character Movement: Walking|Pivot", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float PivotBonusAccelerationOnEnd;

	UPROPERTY(Category = "Character Movement: Walking|Ledge", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ForceWalkOffLedgeImpulse;

	UPROPERTY(Category = "Character Movement: Walking|Ledge", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ForceWalkOntoLedgeImpulse;

private:
	/**
	 * Can only sprint when the angle between the input direction and movement direction is within this value
	 * At 0, can sprint backwards
	 * At 45, can sprint sideways but not backwards
	 * At 90, can only sprint when moving directly forwards only (not recommended, needs some error tolerance)
	 */
	UPROPERTY(Category = "Character Movement: Walking|Sprint", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0"))
	float MaxSprintDirectionInputAngle;

	/**
	 * Can only sprint when the normal between the input direction and movement direction is within this value
	 */
	UPROPERTY(Category = "Character Movement: Walking|Sprint", VisibleAnywhere)
	float MaxSprintDirectionInputNormal;

public:
	/** If crouch pressed while moving at this speed, will slide */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, meta = (EditCondition = "FloorSlideConditions == EMIFloorSlide::FSR_SpeedThreshold || FloorSlideConditions == EMIFloorSlide::FSR_SprintAndSpeedThreshold"))
	float FloorSlideMinStartSpeed;

	/** Must have met conditions for this amount of time before able to floor slide */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, meta = (EditCondition = "FloorSlideConditions != EMIFloorSlide::FSR_NeverUsed"))
	float FloorSlideMinConditionDuration;

	/** Minimum interval between floor sliding re-entry */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, meta = (EditCondition = "FloorSlideConditions != EMIFloorSlide::FSR_NeverUsed"))
	float FloorSlideCooldownTime;

	/** Must remain above this speed to continue floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, meta = (EditCondition = "FloorSlideConditions != EMIFloorSlide::FSR_NeverUsed"))
	float FloorSlideMinSpeed;

	/** The maximum ground speed when floor sliding. */
	UPROPERTY(Category="Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxWalkSpeedFloorSliding;
	
	/** Max Acceleration (rate of change of velocity) */
	UPROPERTY(Category="Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxAccelerationFloorSliding;

	/**
	 * Deceleration when walking and not applying acceleration. This is a constant opposing force that directly lowers velocity by a constant value.
	 * @see GroundFriction, MaxAcceleration
	 */
	UPROPERTY(Category="Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationFloorSliding;

	/**
	 * Setting that affects movement control. Higher values allow faster changes in direction.
	 * If bUseSeparateBrakingFriction is false, also affects the ability to stop more quickly when braking (whenever Acceleration is zero), where it is multiplied by BrakingFrictionFactor.
	 * When braking, this property allows you to control how much friction is applied when moving across the ground, applying an opposing force that scales with current velocity.
	 * This can be used to simulate slippery surfaces such as ice or oil by changing the value (possibly based on the material pawn is standing on).
	 * @see BrakingDecelerationWalking, BrakingFriction, bUseSeparateBrakingFriction, BrakingFrictionFactor
	 */
	UPROPERTY(Category="Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float GroundFrictionFloorSliding;

	/** Apply a speed boost when floor sliding starts while on ground */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideGroundSpeedBoost;

	/** Apply a speed boost when floor sliding starts while in air */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideAirSpeedBoost;

	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideUphillAccelerationMultiplier;
	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideDownhillAccelerationMultiplier;

	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideUphillSpeedMultiplier;
	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideDownhillSpeedMultiplier;

	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideUphillBrakingDecelerationMultiplier;
	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideDownhillBrakingDecelerationMultiplier;

	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideUphillFrictionMultiplier;
	/** Multiplier for slope curve value when floor sliding */
	UPROPERTY(Category = "Character Movement: Walking|Floor Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float FloorSlideDownhillFrictionMultiplier;

public:
	/** How long after walking off a ledge can we still jump normally (leading to more forgiving platforming / jumping) */
	UPROPERTY(Category = "Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float CoyoteTime;

	/** How long before landing does an early jump input still trigger (once landed, leading to more responsive platforming / jumping) */
	UPROPERTY(Category = "Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float BunnyTime;

private:
	/**
	 * The angle starting from the character's forward direction that is considered moving forward
	 * At 0, only moving forward when W (or move forward key) is held and nothing else, will almost never be true with a gamepad thumbstick
	 * At 90, moving forward when strafing also, but with a gamepad will often think its moving backward
	 * At 135, is considered moving forward when holding A+S (or move left + move backward)
	 * At 180, always thinks its moving forward
	 */
	UPROPERTY(Category = "Character Movement (General Settings)", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MoveForwardAngle;

	/**
	 * The normal starting from the character's forward direction that is considered moving forward
	 */
	UPROPERTY(Category = "Character Movement (General Settings)", VisibleAnywhere)
	float MoveForwardNormal;

protected:
	/** Character movement component belongs to */
	UPROPERTY(Transient, DuplicateTransient)
	AMICharacter* MICharacterOwner;

	FAccelerationFrameCache AccelerationFrameCache;

	FAngularVelocityCompute AngularVelocityCompute;

	FAccelerationFrameCache TraversalFrameCache;

	FVector LastCalcVelocityPosition;

	FVector LastNonZeroVelocity;

	FVector LastNonZeroAcceleration;

	UPROPERTY()
	FVector CycleEndDirection;

public:
	float CoyoteStartTime;
	float BunnyStartTime;

public:
	FVector AccelerationDelta;

	FFindFloorResult LastWalkableFloor;

	/** @return Velocity acceleration computed from the past 3 frames */
	UFUNCTION(BlueprintPure, Category = Movement)
	FORCEINLINE FVector GetImpactVelocity() const { return AccelerationFrameCache.GetAcceleration(); }

	/** @return Velocity acceleration computed from the past 3 frames */
	UFUNCTION(BlueprintPure, Category = Movement)
	FORCEINLINE FVector GetTraversalVelocity() const { return TraversalFrameCache.GetAcceleration(); }

	/** @return Angular Velocity */
	UFUNCTION(BlueprintPure, Category = Movement)
	FORCEINLINE FRotator GetAngularVelocity() const { return AngularVelocityCompute.AngularVelocity; }

public:
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	FVector PivotMovementDirection;

	float PivotAlpha;

	FTimerHandle PivotLockTimerHandle;
	FTimerHandle PivotCooldownTimerHandle;

public:
	UMICharacterMovementComponent(const FObjectInitializer& OI);

	void InitCurve(FRuntimeFloatCurve& InCurve, float FirstKeyTime, float FirstKeyValue, float LastKeyTime, float LastKeyValue);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif  // WITH_EDITOR

	// Sprint angle

	/** Set the max angle in degrees for sprint input. Also computes MaxSprintDirectionInputNormal. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
	void SetMaxSprintDirectionInputAngle(float InMaxSprintDirectionInputAngle);

	/** Set the max normal for sprint input. Also computes MaxSprintDirectionInputAngle. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
	void SetMaxSprintDirectionInputNormal(float InMaxSprintDirectionInputNormal);

	/** Get the max angle in degrees for sprint input. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement", meta = (DisplayName = "GetMaxSprintDirectionInputAngle", ScriptName = "GetMaxSprintDirectionInputAngle"))
	FORCEINLINE float K2_GetMaxSprintDirectionInputAngle() const
	{
		return GetMaxSprintDirectionInputAngle();
	}

	/** Set the max normal for sprint input. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement", meta = (DisplayName = "GetMaxSprintDirectionInputNormal", ScriptName = "GetMaxSprintDirectionInputNormal"))
	FORCEINLINE float K2_GetMaxSprintDirectionInputNormal() const
	{
		return GetMaxSprintDirectionInputNormal();
	}

	/** Get the max angle in degrees for sprint input. */
	FORCEINLINE float GetMaxSprintDirectionInputAngle() const { return MaxSprintDirectionInputAngle; }

	/** Set the max normal for sprint input. */
	FORCEINLINE float GetMaxSprintDirectionInputNormal() const { return MaxSprintDirectionInputNormal; }

	// Move forward angle
	
	/** Set the max angle in degrees for moving forward (as opposed to moving backward). When outside this angle, move backward speed is applied. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
	void SetMoveForwardAngle(float InMoveForwardAngle);

	/** Set the max normal in degrees for moving forward (as opposed to moving backward). When outside this normal, move backward speed is applied. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
	void SetMoveForwardNormal(float InMoveForwardNormal);

	/** Get the max angle in degrees for moving forward (as opposed to moving backward). When outside this angle, move backward speed is applied. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement", meta = (DisplayName = "GetMoveForwardAngle", ScriptName = "GetMoveForwardAngle"))
	FORCEINLINE float K2_GetMoveForwardAngle() const
	{
		return GetMoveForwardAngle();
	}

	/** Get the max normal in degrees for moving forward (as opposed to moving backward). When outside this angle, move backward speed is applied. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement", meta = (DisplayName = "GetMoveForwardNormal", ScriptName = "GetMoveForwardNormal"))
	FORCEINLINE float K2_GetMoveForwardNormal() const
	{
		return GetMoveForwardNormal();
	}

	/** Get the max angle in degrees for sprint input. */
	FORCEINLINE float GetMoveForwardAngle() const { return MoveForwardAngle; }

	/** Set the max normal for sprint input. */
	FORCEINLINE float GetMoveForwardNormal() const { return MoveForwardNormal; }

public:
	virtual void OnRegister() override;
	virtual void PostLoad() override;
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode /* = 0 */) override;

	virtual bool HasValidData() const override;

	virtual FVector ConsumeInputVector() override;

	FORCEINLINE bool IsMovingBackwards() const { return (Velocity.GetSafeNormal() | UpdatedComponent->GetForwardVector()) < MoveForwardNormal; }
	float GetMoveBackwardsSpeedMultiplier() const;

	/** @return the Super for GetMaxSpeed (default CMC result instead of custom) */
	FORCEINLINE float GetBaseMaxSpeed() const { return Super::GetMaxSpeed(); }

	virtual float GetMaxSpeed() const override;
	/** 
	 * Override to change result of gait node
	 * @see update logic for MIAnimInstance::AnimSpeed
	 */
	FORCEINLINE virtual float GetMaxGaitSpeed() const { return GetMaxSpeed(); }
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual void ModifyFriction(float& Friction) const;

	virtual void CheckMovementInput(float DeltaTime);

	virtual void PerformMovement(float DeltaTime) override;
	virtual void SimulateMovement(float DeltaTime) override;

	void HandleCycleAcceleration(float DeltaTime, bool bPivotEnd);

	bool HandlePivot(float DeltaTime);
	bool IsPivotOnCooldown() const;
	void OnPivotStart();
	void OnPivotStop();

	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

	float GetHillCurveMultiplier(const FRuntimeFloatCurve& NormalCurve, const bool bUphill) const;
	FORCEINLINE float GetUphillCurveMultiplier(const FRuntimeFloatCurve& NormalCurve) const { return GetHillCurveMultiplier(NormalCurve, true); }
	FORCEINLINE float GetDownhillCurveMultiplier(const FRuntimeFloatCurve& NormalCurve) const { return GetHillCurveMultiplier(NormalCurve, false); }

	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration) override;

	FVector GetCycleAcceleration(float DeltaTime, float CycleRate = -1.f) const;

	virtual bool ShouldCycleOrientToView() const;

	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;

	virtual bool DoJump(bool bReplayingMoves) override;

	virtual void HandleWalkingOffLedge(const FVector& PreviousFloorImpactNormal, const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta) override;
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;

	/** Returns whether this pawn is currently allowed to walk off ledges */
	virtual bool CanWalkOffLedges() const override;

	virtual void Crouch(bool bClientSimulation /* = false */) override;
	virtual void UnCrouch(bool bClientSimulation /* = false */) override;

	virtual void StartFloorSlide(bool bClientSimulation);
	virtual void StopFloorSlide(bool bClientSimulation);

	virtual void StartCrouchRun(bool bClientSimulation);
	virtual void StopCrouchRun(bool bClientSimulation);

	/** True if sprinting or attempting to sprint */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsSprinting() const;

	/** True if crouch running */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsCrouchRunning() const;

	/** True if sliding (on floor or in air) */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsFloorSliding() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Predicted randomization - you don't need to understand how this black magic works, just use the FRandomStream!
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	/** Time server is using for this move, from timestamp passed by client */
	UPROPERTY()
	float CurrentServerMoveTime;

	UPROPERTY()
	float LastSentTimeStamp;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	virtual void CallServerMovePacked(const FSavedMove_Character* NewMove, const FSavedMove_Character* PendingMove, const FSavedMove_Character* OldMove);

	/** Return synchronized time (timestamp currently being used on server, timestamp being sent on client) */
	UFUNCTION(BlueprintPure, Category = Replication)
	float GetCurrentSyncTime() const;

	/** Return synchronized random stream */
	UFUNCTION(BlueprintCallable, Category = Replication)
	void SyncRandomSeed(UPARAM(ref) FRandomStream& Stream);

	/** Return player ping - requires player state */
	UFUNCTION(BlueprintPure, Category = Replication)
	float GetPlayerPing() const;

	/** Return world time on client, CurrentClientTimeStamp on server */
	virtual float GetCurrentMovementTime() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	/** Get prediction data for a client game. Should not be used if not running as a client. Allocates the data on demand and can be overridden to allocate a custom override if desired. Result must be a FNetworkPredictionData_Client_Character. */
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
};

class MOVEIT_API FSavedMove_Character_MoveIt : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	FSavedMove_Character_MoveIt() {}

	uint32 bWantsToSprint : 1;
	uint32 bPivot : 1;
	float PivotAlpha;

	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void Clear() override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;
	virtual bool IsImportantMove(const FSavedMovePtr& LastAckedMove) const override;
};

class MOVEIT_API FNetworkPredictionData_Client_Character_MoveIt : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	FNetworkPredictionData_Client_Character_MoveIt(const UCharacterMovementComponent& ClientMovement)
		: Super(ClientMovement)
	{}

	virtual FSavedMovePtr AllocateNewMove() override;
};
