// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MITypes.h"
#include "Engine/EngineTypes.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "MIAnimInstance.generated.h"

class AMICharacter;
class UMICharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FMIOnSurfaceImpact, FName, BoneName, UPhysicalMaterial*, PhysMat, FVector, Location, FRotator, Rotation, float, BoneSpeed);

UENUM(BlueprintType)
enum class EMIStrafeDirectionBasis : uint8
{
	MISB_Acceleration			UMETA(DisplayName = "Acceleration", ToolTip = "Strafe direction based on acceleration (input direction)"),
	MISB_Velocity				UMETA(DisplayName = "Velocity", ToolTip = "Strafe direction based on velocity (movement direction)"),
};

/**
 * Used to mask surface impacts (footsteps) based on movement state
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EMISurfaceStateType : uint8
{
	MIST_None			= 0					UMETA(Hidden),
	MIST_Default		= 1 << 1			UMETA(DisplayName = "Default"),
	MIST_Ragdoll		= 1 << 2			UMETA(DisplayName = "Ragdoll"),
	MIST_FloorSliding	= 1 << 3			UMETA(DisplayName = "Floor Sliding"),
};
ENUM_CLASS_FLAGS(EMISurfaceStateType);

/**
 * Used to determine who receives a callback for impacts (footsteps) based on net role
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EMICallbackSetting : uint8
{
	MICBS_None				= 0					UMETA(Hidden),
	MICBS_Local				= 1 << 1			UMETA(DisplayName = "Local", ToolTip = "Locally controlled characters receive callback"),
	MICBS_Simulated			= 1 << 2			UMETA(DisplayName = "Simulated", ToolTip = "Simulated (other players) receive callback"),
	MICBS_ListenServer		= 1 << 3			UMETA(DisplayName = "Authority", ToolTip = "Authority receives callback if playing as listen server"),
};
ENUM_CLASS_FLAGS(EMICallbackSetting);

USTRUCT(BlueprintType)
struct FMISurfaceImpact
{
	GENERATED_BODY()

	FMISurfaceImpact()
		: Bone(FBoneReference())
		, Socket(NAME_None)
		, MinVelocity(30.f)
		, TraceStartHeight(5.f)
		, TraceDistFromGround(5.f)
		, MinTriggerInterval(0.2f)
		, AllowedStates((uint8)(EMISurfaceStateType::MIST_Default | EMISurfaceStateType::MIST_FloorSliding | EMISurfaceStateType::MIST_Ragdoll))
		, bUseGlobalImpactCallback(true)
		, Callbacks((uint8)(EMICallbackSetting::MICBS_Local | EMICallbackSetting::MICBS_Simulated | EMICallbackSetting::MICBS_ListenServer))
		, bPlaySound(true)
		, bSpawnParticle(true)
		, LODThreshold(2)
		, bEnabled(true)
		, bWasHit(false)
		, PreviousVelocity(0.f)
		, PreviousBoneWorldLocation(FVector::ZeroVector)
	{
		ObjectsToTrace.Add(ECollisionChannel::ECC_WorldStatic);
		ObjectsToTrace.Add(ECollisionChannel::ECC_WorldDynamic);
	}

	FMISurfaceImpact(const FBoneReference& InBone, const FName& InSocket, const float InMinVelocity = 30.f, const EMISurfaceStateType InActiveStates = EMISurfaceStateType::MIST_Default | EMISurfaceStateType::MIST_FloorSliding | EMISurfaceStateType::MIST_Ragdoll, const float InTraceDistFromGround = 5.f, const float InTraceStartHeight = 5.f)
		: Bone(InBone)
		, Socket(InSocket)
		, MinVelocity(InMinVelocity)
		, TraceStartHeight(InTraceStartHeight)
		, TraceDistFromGround(InTraceDistFromGround)
		, MinTriggerInterval(0.2f)
		, AllowedStates((uint8)InActiveStates)
		, bUseGlobalImpactCallback(true)
		, Callbacks((uint8)(EMICallbackSetting::MICBS_Local | EMICallbackSetting::MICBS_Simulated | EMICallbackSetting::MICBS_ListenServer))
		, bPlaySound(true)
		, bSpawnParticle(true)
		, LODThreshold(2)
		, bEnabled(true)
		, bWasHit(false)
		, PreviousVelocity(FVector::ZeroVector)
		, PreviousBoneWorldLocation(FVector::ZeroVector)
	{
		ObjectsToTrace.Add(ECollisionChannel::ECC_WorldStatic);
		ObjectsToTrace.Add(ECollisionChannel::ECC_WorldDynamic);
	}

	/** The bone that is used to compute velocity */
	UPROPERTY(EditAnywhere, Category = Impact)
	FBoneReference Bone;

	/** The socket that is used to trace to the ground and to spawn effects from */
	UPROPERTY(EditAnywhere, Category = Impact)
	FName Socket;

	/** How fast the bone must be moving to trigger a Impact */
	UPROPERTY(EditAnywhere, Category = Impact)
	float MinVelocity;

	/** How far above the socket to start the trace from */
	UPROPERTY(EditAnywhere, Category = Impact)
	float TraceStartHeight;

	/** How far above the ground the bone can be while still triggering an impact */
	UPROPERTY(EditAnywhere, Category = Impact)
	float TraceDistFromGround;

	/** How often an impact can be triggered by this bone */
	UPROPERTY(EditAnywhere, Category = Impact)
	float MinTriggerInterval;

	/** Will only trigger when one of these states are active */
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "EMISurfaceStateType"), Category = Impact)
	uint8 AllowedStates;

	/**
	 * If true will use GlobalImpactCallback instead of Callbacks to determine callback options
	 */
	UPROPERTY(EditAnywhere, Category = Impact)
	bool bUseGlobalImpactCallback;

	/** 
	 * Used to determine who receives a callback for impacts (footsteps) based on net role
	 * Will never be called on dedicated server
	 * Will be LOD'd out if the impact is
	 */
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "EMICallbackSetting", EditCondition = "!bUseGlobalImpactCallback"), Category = Impact)
	uint8 Callbacks;

	/** If using a callback to play a sound based on your custom settings, you might want to disable it here */
	UPROPERTY(EditAnywhere, Category = Impact)
	bool bPlaySound;

	/** If using a callback to spawn a particle based on your custom settings, you might want to disable it here */
	UPROPERTY(EditAnywhere, Category = Impact)
	bool bSpawnParticle;

	/*
	* Max LOD that this impact is allowed to run
	* For example if you have LODThreadhold to be 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact, meta = (DisplayName = "LOD Threshold"))
	int32 LODThreshold;

	/** Toggle surface impact */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	bool bEnabled;

	UPROPERTY(EditAnywhere, Category = Impact)
	TArray<TEnumAsByte<ECollisionChannel>> ObjectsToTrace;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = Impact)
	bool bDebugDrawTrace = false;
#endif

	bool bWasHit;

	FTimerHandle CooldownTimerHandle;

	FVector PreviousVelocity;
	FVector PreviousBoneWorldLocation;
};

/**
 * 
 */
UCLASS(AutoCollapseCategories=(States, Weapon, AimOffset, LookTarget, Movement, TurnInPlace, Input, RootMotion), AutoExpandCategories=(AnimationSettings, "AnimationSettings|Turn In Place", "AnimationSettings|Strafe", "AnimationSettings|Gait"))
class MOVEIT_API UMIAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = References)
	AMICharacter* Character;

	UPROPERTY(BlueprintReadWrite, Category = References)
	UMICharacterMovementComponent* Movement;

	UPROPERTY(BlueprintReadWrite, Category = References)
	USkeletalMeshComponent* Mesh;

public:
	/** Character is not crouched */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsStanding;

	/** Character is crouched */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsCrouched;

	/** Character is crouched and on the ground */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bCrouchingOnGround;

	/** Character is jumping (falling and Velocity.Z > 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsJumping;

	/** Character is in the air (can be both jumping and falling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsFalling;

	/** Character is on the ground (not falling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bOnGround;

	/** Character is playing root motion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bPlayingRootMotion;

	/** Character is sprinting and over 50% of sprint speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsSprinting;

	/** Character is sprinting and over 50% of sprint speed (and not armed) when landing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsSprintLanding;

	/** Character is crouch running */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsCrouchRunning;

	/** Character is crouch running and on the ground */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bCrouchRunningOnGround;

	/** Character is floor sliding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bIsFloorSliding;

	/** Should apply procedural strafing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bStrafeEnabled;

	/** Gait is enabled when on ground and moving */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bGaitEnabled;

	/** Character is aiming weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bAiming;

	/** Character is not aiming weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bNotAiming;

	/** Character is a local player who is aiming weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bAimingLocalPlayer;
	
	/** Is not a local player or is not aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bNotAimingLocalPlayer;

	/** Character is in ragdoll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bRagdoll;

	/** FootIK is enabled when on ground */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	bool bFootIK;

public:
	/** Character has no weapon out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bUnarmed;

	/** Character has a weapon out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bArmed;

	/** Character is changing weapon (used to detect changes in weapon requiring a new pose snapshot for blending) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bChangingWeapon;
	
	/** True if Weapon contains animation sequence to layer over animation states */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bUseWeaponPose;

	/** True if Weapon contains aim offset to apply */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bUseWeaponAimPose;

	/** True if armed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bEnableWeaponPose;

	/** True if armed and weapon is not one handed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bEnableHandIK;

	/** True if current weapon is one handed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	bool bWeaponOneHanded;

	/** Aim offset is disabled when look target is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Aim Offset")
	bool bAimOffsetEnabled;

	/** This should be set externally by the character to enable or disable look targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Look Target")
	bool bEnableLookTarget;
	
	/** Is pivoting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bPivot;

	/** Speed is over 0.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsMoving;

	/** Speed is 0.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsIdle;

	/** Has velocity but is not providing input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsStopping;

	/** Stopping after >= moving at 80% of max speed - this is used to provide a different transition duration for faster movement (as the stop animation can be much more exaggerated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsStoppingAtSpeed;

	/** If true, was crouch running when stopping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsStoppingFromCrouchRunning;

	/** Has velocity but is not providing input (while in air) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsStoppingInAir;

	/** If was moving backwards when stopping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsStoppingBackwards;

	/** (bIsApplyingInput & bIsMoving) - Used as "has started moving after stopping" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsMovingWithInput;

	/** Movement direction, also applicable to strafing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsMovingBackward;

	/** Movement direction, also applicable to strafing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsMovingForward;
	
	/** Used by anim graph to determine left or right turn animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	bool bTurnRight;

	/** Used by anim graph to trigger Turn state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	bool bDoTurn;

	/** Used by anim graph to exit turn during recovery period (to restart turn faster) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	bool bTurnExit;

	/** Used by anim graph to restart turn after re-entering idle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	bool bCanRestartTurn;

	/** Gate to update curve value at start of turn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	bool bResetTurn;
	
	/**
	 * Current state used for Foot IK
	 * 0 if Standing
	 * 1 if Crouched
	 * 2 if Floor Sliding
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	uint8 FootIKState;

	/** Current orientation for strafing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	EMIStrafeOrientation StrafeOrientation;
	
	/** Current anim info for weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	FMIWeapon Weapon;

	/** Always Weapon.AimOffset if it is valid, if not valid, will be the last valid WeaponAimOffset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	UBlendSpace* WeaponAimOffset;

	/** Blends in left arm animation if weapon is one handed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon")
	float LeftArmBlendWeight;

	/** Transform applied using FABRIK to place offhand on weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Weapon", meta = (DisplayName = "OffHand IK"))
	FTransform OffHandIKTM;

public:
	/** Because Turn in Place prevents mesh from turning, need to rotate something to compensate by this amount (usually spine) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Aim Offset")
	float AimOffsetTurn;

	/** Horizontal axis for aim offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Aim Offset")
	float AimOffsetYaw;

	/** Vertical axis for aim offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Aim Offset")
	float AimOffsetPitch;

	/** Current movement system (affects strafing behaviour) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|States")
	EMIMovementSystem MovementSystem;

	/** Globally toggle all surface impacts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Impacts")
	bool bEnableSurfaceImpacts;

	/** 
	 * Callback settings for all surface impacts that don't define their own
	 * Will never be called on dedicated server
	 * Will be LOD'd
	 */
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "EMICallbackSetting", EditCondition = "bEnableSurfaceImpacts"), Category = "AnimationSettings|Impacts")
	uint8 GlobalImpactCallback;

protected:
	bool bImpactsInactive;

public:
	/** This should be set externally by the character to assign current look target */
	UPROPERTY(BlueprintReadWrite, Category = "AnimationState|Look Target")
	AActor* LookTarget;

public:
	/** How fast the character is moving and the direction they are moving in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FVector Velocity;

	/** How fast the character has been moving over the past few frames */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FVector TraversalVelocity;

	/** Velocity of character jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FVector JumpingVelocity;

	/** Traversal velocity of character impact when landing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FVector LandingVelocity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FRotator VelocityRotation;

	/** Rotational velocity, used to drive turn in place and leaning, etc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FRotator AngularVelocity;

	/** Non-normalized horizontal speed (original raw value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float RawSpeed;

	/** @see MICharacterMovement->GetMaxGaitSpeed() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float GaitSpeedRaw;

	/** Horizontal speed normalized to a 0-1 range (up to CharacterMovement->GetMaxSpeed()) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float Speed;

	/** Interpolated speed applied to the gate allowing for slower changes to the gate to cause start/stop animations and to blend nicely to and from idle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float GaitSpeed;
	
	/** Current gait multiplier based on character state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float GaitMultiplier;

	/** Speed when stopping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float StoppingSpeed;

	/** Yaw direction that we are moving in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float Direction;

	/** Motion weight is carried momentum from starting/stopping movement that is applied via the MotionWeight anim graph node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float MotionWeight;
	
	/** Pivot Yaw Direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float PivotDirection;

	/** Plug into animations to scale their play rate based on movement speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float AnimPlayRate;

	/** Velocity when beginning pivoting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FVector PivotVelocity;

	/** Input resulting in pivoting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FVector PivotInput;

public:
	/** Used to negate rotation from mesh (to prevent mesh turning until turn in place triggers) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	float RootYawOffset;

	/** Character Actor Rotation Yaw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	float Yaw;

	/** Value of the yaw curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	float CurveValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	float TurnAngle;

	/** Angle of current turn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	float StepAngle;

	/** Which step animation index to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	int32 StepSize;

	/** Play rate for turn animations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Turn In Place")
	float TurnPlayRate;

	/** Curve Name for Yaw Curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	FName YawCurveName;

	/** Curve Name for whether we are turning at this part of the animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	FName IsTurningCurveName;

	/** Yaw angles where different step animations occur */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	TArray<int32> StepSizes = { 60, 90, 135 };

	/** Angle at which turn in place will trigger */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	float MinTurnAngle;

	/** Maximum angle at which point the character will turn to maintain this value (hard clamp on angle) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	float MaxTurnAngle;

	/** Turn rate multiplied when we're at max turn angle, commonly used to make animation play faster in an attempt to keep up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	float TurnRateAtMaxTurnAngle;

	/** Turn rate multiplied when we're changing from one turn animation to another (in the opposing direction), commonly used to make animation play faster in an attempt to keep up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	float TurnRateDirectionChange;

	/** Rate at which character turns when starting to move from idle (or partial turn) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	float StartMovingTurnRate;

	/** Rate at which character turns when starting a root motion montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place")
	bool bRootMotionMontageResetsTurn;
	
	bool bIsTurning;

	/** Is falling downwards rather than upwards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bIsFallingDownward;

	/** Whether character is headed to a surface they can land on or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bCanLand;

	/** Rate at which character turns when starting a root motion montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Turn In Place", meta = (EditCondition = "bRootMotionMontageResetsTurn"))
	float RootMotionMontageTurnRate;

public:
	/** Input as obtained from the character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector RawInput;

	/** Input interpolated to zero (applied to gait) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector Input;

	/** Applies stopping input if stopping otherwise applies Input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector MovingInput;

	/** Interpolated input for smooth direction changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector DirectionInput;

	/** Interpolated input direction. X is forward, Y is right, Z is not used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector StrafeDirection;

	/** Input supplied when initially triggering stopping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector StoppingInput;

	/** Velocity when initially triggering stopping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FVector StoppingVelocity;

	/** Rotation when initially triggering stopping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FRotator StoppingRotation;

protected:
	float ForwardBackSwitchTime;

public:
	/** Predicted landing motion, only valid when falling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	FPredictProjectilePathResult PredictedLanding;

	/** How far from landing location (as predicted, not mere distance to ground) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float DistanceToLandLocation;

	/** How long character has been falling for */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float FallingDuration;

	/** 0 if moving backward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	float LandingSpeed;

public:
	/** Input from the previous frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector PreviousInput;

	/** Raw input from the previous frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FVector PreviousRawInput;
	
	/** Velocity rotation from the previous frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	FRotator PreviousVelocityRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	bool bIsApplyingInput;

	/** Was applying input on the previous frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Input")
	bool bWasApplyingInput;

public:
	/** Whether strafing is based on Input (Acceleration) or Velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	EMIStrafeDirectionBasis NeutralStrafeDirectionBasis;

	/** Whether strafing is based on Input (Acceleration) or Velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	EMIStrafeDirectionBasis RightStrafeDirectionBasis;

	/** When neutral strafing, when checking the direction will check for an additional degree of change required to be considered "backwards" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	float BackwardsVelocityDegreesTolerance;

	/** When neutral strafing, when checking the direction will check for an additional degree of change required to be considered "backwards" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	float BackwardsAccelerationDegreesTolerance;

	/** How fast we can change direction (strafe/blendspace direction rate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	float DirectionInterpRate;

	/** How fast we can change direction when switching from moving backward->forward (strafe/blendspace direction rate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	float DirectionChangeForwardInterpRate;

	/** How fast we can change direction when switching from moving forward->backward (strafe/blendspace direction rate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	float DirectionChangeBackwardInterpRate;

	/** Time delay to switch from backwards to forward strafing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	float ForwardSwitchDelay;

	/** Time delay to switch from forward to backwards strafing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	float BackwardSwitchDelay;

	/** Sprint may look bad with procedural strafe and using traditional strafe animations could be preferable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Strafe")
	bool bSprintUsesProceduralStrafe;

	/** Whether to apply gait while sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Gait")
	bool bApplyGaitToSprinting;

	/** Whether to apply gait while floor sliding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Gait")
	bool bApplyGaitToFloorSliding;
	
	/** Enter extended falling state (flailing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationState|Movement")
	bool bFallingTooFar;

	/** Applied when not sprinting or crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Gait")
	float BaseGaitMultiplier;

	/** Applied when sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Gait", meta = (EditCondition = "bApplyGaitToSprinting"))
	float SprintGaitMultiplier;

	/** Applied when crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Gait")
	float CrouchGaitMultiplier;

	/** How fast gait speed catches up to actual speed - Set to 0 to disable interpolation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Gait")
	float GaitInterpRate;
		
	/** How fast input interpolates to zero when idle (used to smoothly transition gait node from moving to idle) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Gait")
	float InputInterpToZeroRate;

	/** How long the pivot animation runs for */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimationSettings, meta = (DisplayPriority = "1"))
	float PivotDuration;

	/** Used when starting from idle to begin at a later point in the animation (good for reducing excessive build-up based on your movement settings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimationSettings)
	float StartPositionOffset;
	
	/** How long must character fall for before entering flailing state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimationSettings, meta = (DisplayPriority = "2"))
	float FallTooFarDuration;
	
	/** Animation rate based on character speed. Not used by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimationSettings)
	float MinAnimPlayRate;

	/** Animation rate based on character speed. Not used by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimationSettings)
	float MaxAnimPlayRate;

public:
	/** If true, aim offset can be used - primarily used to enable/disable aim offset externally */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Aim Offset", meta = (UIMin = "0", ClampMin = "0"))
	bool bAllowAimOffset;

	/** How fast the aim offset interpolates for yaw. Set to 0 to disable interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Aim Offset", meta = (UIMin="0", ClampMin="0"))
	float AimOffsetYawRate;

	/** How fast the aim offset interpolates for pitch. Set to 0 to disable interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Aim Offset", meta = (UIMin="0", ClampMin="0"))
	float AimOffsetPitchRate;

	/** How fast the aim offset interpolates for yaw when step size is changing. Set to 0 to disable interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Aim Offset", meta = (UIMin = "0", ClampMin = "0"))
	float AimOffsetYawCompensateRate;

	/** Scale the turn right aim offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Aim Offset")
	float BodyTurnRightCompensationScale;

	/** Scale the turn left aim offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Aim Offset")
	float BodyTurnLeftCompensationScale;

protected:
	float PivotStartTime;

	/** Interpolated */
	FQuat StrafeQuat;
	FRotator OrientRotation;
	
	FDelegateHandle SurfaceImpactDelegateHandle;

public:
	UPROPERTY(BlueprintAssignable, Category = "Impacts")
	FMIOnSurfaceImpact OnSurfaceImpact;

	/** A physically accurate footstep system that can handle any and all other bones hitting the ground */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationSettings|Impacts", meta = (EditCondition = "bEnableSurfaceImpacts"))
	TArray<FMISurfaceImpact> SurfaceImpacts;

public:
	UMIAnimInstance()
		: bImpactsInactive(true)
		, ForwardBackSwitchTime(0.f)
		, NeutralStrafeDirectionBasis(EMIStrafeDirectionBasis::MISB_Velocity)
		, RightStrafeDirectionBasis(EMIStrafeDirectionBasis::MISB_Acceleration)
		, BackwardsVelocityDegreesTolerance(20.f)
		, BackwardsAccelerationDegreesTolerance(0.f)
		, DirectionInterpRate(7.f)
		, DirectionChangeForwardInterpRate(0.03f)
		, DirectionChangeBackwardInterpRate(0.03f)
		, ForwardSwitchDelay(2.f)
		, BackwardSwitchDelay(2.f)
		, BaseGaitMultiplier(1.f)
		, SprintGaitMultiplier(1.f)
		, CrouchGaitMultiplier(1.f)
		, GaitInterpRate(1.6f)
		, InputInterpToZeroRate(2.f)
		, PivotDuration(0.3f)
		, StartPositionOffset(0.25f)
		, FallTooFarDuration(3.f)
		, MinAnimPlayRate(0.7f)
		, MaxAnimPlayRate(1.f)
		, bAllowAimOffset(true)
		, AimOffsetYawRate(350.f)
		, AimOffsetPitchRate(350.f)
		, AimOffsetYawCompensateRate(60.f)
		, PivotStartTime(0.f)
		, StrafeQuat(FQuat::Identity)
		, OrientRotation(FRotator::ZeroRotator)
	{
		AnimPlayRate = 1.f;

		bUnarmed = true;

		bAimOffsetEnabled = true;
		bEnableLookTarget = false;

		RawSpeed = 1.f;
		Speed = 1.f;
		GaitSpeed = 1.f;
		bIsMovingForward = true;

		AngularVelocity = FRotator::ZeroRotator;

		bEnableSurfaceImpacts = true;
		GlobalImpactCallback = 0;
		const FMISurfaceImpact LeftFootImpact = FMISurfaceImpact(FBoneReference("foot_l"), TEXT("footstep_l"));
		const FMISurfaceImpact RightFootImpact = FMISurfaceImpact(FBoneReference("foot_r"), TEXT("footstep_r"));
		const FMISurfaceImpact LeftHandImpact = FMISurfaceImpact(FBoneReference("hand_l"), TEXT("hand_l"), 5.f, EMISurfaceStateType::MIST_Ragdoll | EMISurfaceStateType::MIST_FloorSliding, 15.f);
		const FMISurfaceImpact RightHandImpact = FMISurfaceImpact(FBoneReference("hand_r"), TEXT("hand_r"), 5.f, EMISurfaceStateType::MIST_Ragdoll | EMISurfaceStateType::MIST_FloorSliding, 15.f);
		const FMISurfaceImpact HeadImpact = FMISurfaceImpact(FBoneReference("head"), TEXT("head"), 5.f, EMISurfaceStateType::MIST_Ragdoll, 25.f);
		SurfaceImpacts = { LeftFootImpact, RightFootImpact, LeftHandImpact, RightHandImpact, HeadImpact };

		bResetTurn = true;
		YawCurveName = TEXT("Yaw");
		IsTurningCurveName = TEXT("IsTurning");
		BodyTurnRightCompensationScale = 1.f;
		BodyTurnLeftCompensationScale = 1.f;
		MinTurnAngle = 60.f;
		MaxTurnAngle = 90.f;
		TurnRateAtMaxTurnAngle = 1.2f;
		TurnRateDirectionChange = 1.4f;
		StartMovingTurnRate = 250.f;
		bRootMotionMontageResetsTurn = true;
		RootMotionMontageTurnRate = 500.f;
		bIsTurning = false;
		bCanRestartTurn = true;
		TurnPlayRate = 1.f;
	}

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	void BlendPhysicalAnimation(float DeltaTime);

	void TurnInPlace(float DeltaTime);

	void ComputeAimOffsets(float DeltaTime);

	virtual void NativePostEvaluateAnimation() override;

	/** A physically accurate footstep system that can handle any and all other bones hitting the ground */
	UFUNCTION()
	void ComputeSurfaceImpacts();

	/** Don't evaluate anim graph if missing references, delta too small, or movement is disabled */
	UFUNCTION(BlueprintPure, Category = Animation)
	bool IsValidToEvaluate(float DeltaTime) const;

	/** Must be called when a turn animation starts via notify */
	UFUNCTION(BlueprintCallable, Category = "AnimationState|Turn In Place")
	void StartTurn();

	/** Must be called when a turn animation stops via notify */
	UFUNCTION(BlueprintCallable, Category = "AnimationState|Turn In Place")
	void StopTurn();

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Animation)
	bool IsSprinting() const;

	/** 
	 * Use ResetRate if over 0.0, otherwise will use StartMovingTurnRate
	 * By default checks bNotAimingLocalPlayer is true (disables turning if local player is aiming)
	 * @return true if turn in place is enabled
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "AnimationState|Turn In Place")
	bool IsTurnInPlaceEnabled(float& ResetRate) const;

	/** Completely reset the turn in place (this will cause a snap) */
	UFUNCTION(BlueprintCallable, Category = "AnimationState|Turn In Place")
	virtual void ResetTurnInPlace();
	
	/** If true, delay settings for strafe will be taken into effect */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Strafe)
	bool ShouldDelayDirectionSwitch() const;

	/** Predicted where character will land while in air */
	bool PredictLandingLocation(FPredictProjectilePathResult& PredictResult);

	/** Started crouch (overlaps floor slide event) */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStartCrouch();

	/** Stopped crouch (overlaps floor slide event) */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStopCrouch();

	/** Started sprinting animation */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStartSprint();

	/** Stopped sprinting animation */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStopSprint();

	/** Started crouch run animation */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStartCrouchRun();

	/** Stopped crouch run animation */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStopCrouchRun();

	/** Started floor slide */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStartFloorSliding();

	/** Stopped floor slide */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStopFloorSliding();

	/** Started pivot */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStartPivot();

	/** Stopped pivot */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation)
	void OnStopPivot();
};
