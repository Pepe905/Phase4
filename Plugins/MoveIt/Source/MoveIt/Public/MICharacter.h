// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MITypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/EngineTypes.h"
#include "Curves/CurveFloat.h"
#include "MICharacter.generated.h"

class UMICharacterMovementComponent;
class UMIViewComponent;
class UAudioComponent;
class UAnimSequenceBase;
class UMIPhysicalMaterial;
class UNiagaraSystem;
class UParticleSystem;

UENUM(BlueprintType)
enum class EMIMovementState : uint8
{
	MS_None					UMETA(DisplayName = "None"),
	MS_Sprinting			UMETA(DisplayName = "Sprinting"),
	MS_Crouching			UMETA(DisplayName = "Crouching"),
	MS_CrouchRunning		UMETA(DisplayName = "Crouch Running"),
	MS_FloorSliding			UMETA(DisplayName = "Floor Sliding"),
	MS_Ragdoll				UMETA(DisplayName = "Ragdoll"),
	MS_MAX = 255			UMETA(Hidden),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMIHitWallDelegate, FVector, ImpactVelocity, FVector, ImpactNormal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMIScuffWallDelegate, FVector, ImpactVelocity, FVector, ImpactNormal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMIHitCharacterDelegate, AMICharacter*, OtherCharacter, FVector, ImpactVelocity, FVector, ImpactNormal);

UCLASS()
class MOVEIT_API AMICharacter : public ACharacter
{
	GENERATED_BODY()

private:
	/** Movement component used for movement logic in various movement modes (walking, sprinting, falling, etc), containing relevant settings and functions to control movement. */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	UMICharacterMovementComponent* MICharacterMovement;

	/** View component used for camera blending and transition logic in various movement modes (crouching, sprinting, etc). */
	UMIViewComponent* MIViewComponent;

public:
	/** Replicated to late joiners (or as they become relevant? docs unclear) to somewhat sync up the facing rotation */
	UPROPERTY(ReplicatedUsing=OnRep_InitialOffset)
	float InitialRootYawOffset;

public:
	/** Default settings that are passed to GetOrientToFloorSettings() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OrientToFloor)
	FOrientToFloorSettings OrientDefaultSettings;

	/** 
	 * Convenience settings to pass to GetOrientToFloorSettings() to make the character orient to the floor
	 * Helpful for floor sliding
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OrientToFloor)
	FOrientToFloorSettings OrientMatchGround;

public:
	FTimerHandle StopRootMotionRotationHandle;

	UPROPERTY()
	float StopRootInitialRotationYaw;

	/** How long it takes for character to face control rotation yaw (if enabled) after finishing a root motion montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pawn)
	float StopRootMotionOrientTime;

	/** How fast the character turns back to face the camera when switching MovementSystem back to OrientToView */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement)
	float MovementSystemChangeOrientRate;

	/** 
	 * Direction to orient in when strafing left
	 * Great for over the shoulder movement
	 * NOT replicated - you must handle this yourself if you want to change it during runtime and other players to see it
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement)
	EMIStrafeOrientation StrafeOrientation;

	/**
	 * General type of movement used
	 * OrientToView faces the camera
	 * OrientToMovement is similar to Third Person Template
	 * CycleOrientToMovement is same as OrientToMovement but has a minimum turning circle like Zelda BOTW or Mario Odyssey so they can't simply spin on the spot, combines with pivot to allow direction change on the spot
	 */
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetMovementSystem", Category = CharacterMovement)
	EMIMovementSystem MovementSystem;

public:
	/** If false, will not rotate during root motion montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pawn)
	bool bDisableControlRotationDuringRootMotion;

	/**
	 * If true, the amount of time to stand up will be taken from the animation played
	 * Note that this can cause desync when used online due to server and client possibly playing different animations
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	bool bRagdollStandUpTimeFromAnimation;

	/** Whether Hit Wall can occur */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall")
	bool bHitWallEnabled;

	/** If true, can apply hit wall effect when montage is playing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall")
	bool bCanHitWallWhileMontagePlaying;

	/** If true, can apply hit wall effect when root motion is playing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall")
	bool bCanHitWallWhileRootMotionPlaying;
	
	/** Whether Hit Character can occur */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	bool bHitCharacterEnabled;

	/** If true, can apply hit character effect when montage is playing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	bool bCanHitCharacterWhileMontagePlaying;

	/** If true, can apply hit character effect when root motion is playing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	bool bCanHitCharacterWhileRootMotionPlaying;

	/** Used for simulated proxies to know when to pivot */
	UPROPERTY(ReplicatedUsing="OnRep_SimulatedPivot")
	bool bSimulatedPivot;

protected:
	bool bMovementSystemInterp;

public:
	/** True if in ragdoll and ragdoll is currently on the ground. Valid updating character only (usually local, or if unpossessed, server). */
	UPROPERTY(BlueprintReadOnly, Category = Ragdoll)
	bool bRagdollOnGround;

	/**
	 * How long it takes to stand up if bRagdollStandUpTimeFromAnimation is false
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll, meta = (EditCondition = "!bRagdollStandUpTimeFromAnimation"))
	float RagdollStandUpTime;

	/** Animation to play when standing up from lying down on face */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	UAnimSequenceBase* RagdollStandUpFace;

	/** Animation to play when standing up from lying down on back */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	UAnimSequenceBase* RagdollStandUpBack;

	/** Name of pelvis bone used by ragdoll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	FName RagdollPelvisBoneName;

	/** How long the ragdoll will aggressively sync position for after entering ragdoll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll, meta = (UIMin = "0", ClampMin = "0"))
	float RagdollAggressiveSyncTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll, meta = (UIMin = "1", ClampMin = "1"))
	float RagdollAggressiveSyncRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll, meta = (UIMin = "1", ClampMin = "1"))
	float RagdollNormalSyncRate;

	/** Ragdoll must exceed this distance to be pushed towards replicated location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Ragdoll)
	float RagdollSmallDistanceThreshold;

	/** Ragdoll must exceed this distance to be teleported to replicated location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Ragdoll)
	float RagdollLargeDistanceThreshold;

	/** Physical material to use when none is assigned to the material being interacted with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact")
	UMIPhysicalMaterial* DefaultPhysicalMaterial;

	/** 
	 * If the physical material has EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a 
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual USoundBase* GetHitSound(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual USoundBase* GetHitSound(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/** 
	 * If the physical material has LookupType "Blueprint" then this function is used to override on a 
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Hit Sound"))
	USoundBase* K2_GetHitSound(const UMIPhysicalMaterial* PhysMat);

	/** 
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a 
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual USoundBase* GetScuffSound(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual USoundBase* GetScuffSound(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/** 
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a 
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Scuff Sound"))
	USoundBase* K2_GetScuffSound(const UMIPhysicalMaterial* PhysMat);

	/** 
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a 
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual USoundBase* GetBoneImpactSound(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual USoundBase* GetBoneImpactSound(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/** 
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a 
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Bone Impact Sound"))
	USoundBase* K2_GetBoneImpactSound(const UMIPhysicalMaterial* PhysMat);

	/**
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual UNiagaraSystem* GetHitNiagara(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual UNiagaraSystem* GetHitNiagara(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/**
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Hit Niagara"))
	UNiagaraSystem* K2_GetHitNiagara(const UMIPhysicalMaterial* PhysMat);

	/**
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual UNiagaraSystem* GetScuffNiagara(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual UNiagaraSystem* GetScuffNiagara(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/**
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Scuff Niagara"))
	UNiagaraSystem* K2_GetScuffNiagara(const UMIPhysicalMaterial* PhysMat);

	/**
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual UNiagaraSystem* GetBoneImpactNiagara(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual UNiagaraSystem* GetBoneImpactNiagara(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/**
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Bone Impact Niagara"))
	UNiagaraSystem* K2_GetBoneImpactNiagara(const UMIPhysicalMaterial* PhysMat);

	/**
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual UParticleSystem* GetHitParticle(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual UParticleSystem* GetHitParticle(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/**
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Hit Particle"))
	UParticleSystem* K2_GetHitParticle(const UMIPhysicalMaterial* PhysMat);

	/**
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual UParticleSystem* GetScuffParticle(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual UParticleSystem* GetScuffParticle(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/**
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Scuff Particle"))
	UParticleSystem* K2_GetScuffParticle(const UMIPhysicalMaterial* PhysMat);

	/**
	 * Requires physical material EMIPhysMatLookupType::PMLT_CPP then this function is used to override on a
	 * per-character basis which asset is used
	 * MARK THE OVERRIDE AS FINAL FOR PERFORMANCE GAINS:
	 * virtual UParticleSystem* GetBoneImpactParticle(const UMIPhysicalMaterial* const PhysMat) override final;
	 */
	virtual UParticleSystem* GetBoneImpactParticle(const UMIPhysicalMaterial* const PhysMat) { return nullptr; }

	/**
	 * Requires physical material LookupType "Blueprint" then this function is used to override on a
	 * per-character basis which asset is used
	 * @warning: NOT RECOMMENDED - CALLED FREQUENTLY AND NOT PERFORMANT - LAST RESORT FOR BP ONLY USERS REQUIRING PER-CHARACTER RETURN
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Physical Impact", meta = (DisplayName = "Get Bone Impact Particle"))
	UParticleSystem* K2_GetBoneImpactParticle(const UMIPhysicalMaterial* PhysMat);

	/** Minimum time to pass before we hit a wall again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall")
	float HitWallMinInterval;
	
	/** Set by character movement to specify that this Character is currently sprinting. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_IsSprinting", Category=Sprint)
	uint32 bIsSprinting:1;

	/** Delegate for hitting a wall */
	UPROPERTY(BlueprintAssignable)
	FMIHitWallDelegate OnHitWall;

	/** Delegate for scuffing a wall */
	UPROPERTY(BlueprintAssignable)
	FMIScuffWallDelegate OnScuffWall;

	/** Delegate for hitting a character */
	UPROPERTY(BlueprintAssignable)
	FMIHitCharacterDelegate OnHitCharacter;

	/**
	* Max LOD that wall impact is allowed to run
	* For example if you have ImpactLODThreshold at 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall", meta = (DisplayName = "Impact LOD Threshold"))
	int32 ImpactLODThreshold;

	/**
	 * Impact velocity must exceed this when running into a wall for a "wall hit" to occur
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall")
	float HitWallImpactVelocityThreshold;

	/** How much Impact Velocity [Time] results in how much Physics Impulse [Value] to play physical animation on the character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall", meta = (DisplayName = "Hit [Impact Velocity -> Physics Impulse]"))
	FRuntimeFloatCurve HitImpactVelocityToPhysicsImpulse;

	/** Blending for impact from hitting a wall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall")
	FPhysicsBlend HitImpactPhysics;

	/** 
	 * Blending settings for bone impact from being shot
	 * @note BoneName is not used 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Take Shot")
	FPhysicsBlend ShotImpactPhysics;

	/** Primarily used to stop simulation on pelvis and hands/feet (they don't look good) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Take Shot")
	TMap<FName, FName> ShotImpactBoneRedirects;

	/** Blending for impact from hitting another character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	FPhysicsBlend HitCharacterImpactPhysics;

	/** Blending for impact from being hit by another character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	FPhysicsBlend HitByCharacterImpactPhysics;

	/** How fast we have to be moving to apply hit another character effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	float HitCharacterVelocityThreshold;

	/** How fast other character has to be moving to apply hit another character effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	float HitByCharacterVelocityThreshold;
	
	/** Sound to play when hitting a character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	USoundBase* HitCharacterGrunt;

	/** Sound to play when hit by a character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	USoundBase* HitByCharacterGrunt;

	/** Voice sound to play when hit by a character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	USoundBase* HitByCharacterVoice;

	/** Minimum time to pass before we can hit a character again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	float HitCharacterMinInterval;

	/** Minimum time to pass before we can be hit by a character again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	float HitByCharacterMinInterval;

	/** How often a voice will play instead of a grunt when hit by another character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Character")
	float HitByCharacterMinVoiceInterval;

	/** 
	 * Impact velocity must exceed this when walking into a wall to play a scuffing sound (set in physical material)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Scuff Wall")
	float ScuffWallSoundVelocityThreshold;

	/** Minimum time to pass before we can play scuff wall sound again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Scuff Wall")
	float ScuffWallSoundMinInterval;

	/** Minimum time to pass before we can play scuff wall particle again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Scuff Wall")
	float ScuffWallParticleMinInterval;

	/** Up normal of the wall surface relative to wall up before it wont allow us to hit it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall|Advanced")
	float HitMaxUpNormal;

	/** 
	 * Movement vector (acceleration) difference to the wall before it wont allow us to hit it 
	 * Lower is easier to trigger
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Hit Wall|Advanced")
	float HitMaxMovementNormal;

	/** Up normal of the wall surface relative to wall up before it wont allow us to scuff it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Scuff Wall|Advanced")
	float ScuffMaxUpNormal;

	/** 
	 * Movement vector (acceleration) difference to the wall before it wont allow us to scuff it 
	 * Lower is easier to trigger
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Impact|Scuff Wall|Advanced")
	float ScuffMaxMovementNormal;

protected:
	FTimerHandle HitWallTimerHandle;

	FTimerHandle ScuffWallSoundTimerHandle;
	FTimerHandle ScuffWallParticleTimerHandle;
	FTimerHandle HitWallSoundTimerHandle;
	FTimerHandle HitWallParticleTimerHandle;

	FTimerHandle HitCharacterTimerHandle;
	FTimerHandle HitByCharacterTimerHandle;
	FTimerHandle HitByCharacterVoiceTimerHandle;

	UPROPERTY()
	UAudioComponent* HitWallAudioComponent;

public:
	/** Mesh -> Impact Struct -> BoneName -> PhysicsBlend */
	UPROPERTY()
	TMap<USkeletalMeshComponent*, FMIShotImpact> ShotBonePhysicsImpacts;

protected:
	FRotator MovementSystemControlRotation;

public:
	/** Game time when sprinting started */
	UPROPERTY(BlueprintReadOnly, Category = Character)
	float SprintStartTime;

	/** Set by character movement to specify that this Character is currently floor sliding. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_IsFloorSliding", Category="Floor Slide")
	uint32 bIsFloorSliding:1;

	/** Set by character movement to specify that this Character is currently crouch running. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_IsCrouchRunning", Category="Crouch Run")
	uint32 bIsCrouchRunning:1;

	/** Used to disable crouch running if it doesn't exist in your game */
	UPROPERTY(EditDefaultsOnly, Category = "Crouch Run")
	bool bCanEverCrouchRun;

	/** What happens if you press crouch while in the air */
	UPROPERTY(EditDefaultsOnly, Category = Character)
	EMICrouchInAirBehaviour AirCrouchBehaviour;

protected:
	/** True if in air and waiting for landing before crouching */
	UPROPERTY(BlueprintReadWrite, Category = Character)
	bool bPendingCrouch;

private:
	bool bWasSimulatingPivot = false;

	float FloorSlideSpeedStartTime;

	float FloorSlideStopTime;

public:
	/** Current input vector. Replicated to simulated players. Used for pivoting and animation sync. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Character)
	FVector Input;

	/** Previous input vector. Available to simulated players. Used for pivoting and animation sync. */
	UPROPERTY(BlueprintReadOnly, Category = Character)
	FVector PreviousInput;

	/** Previous input vector that wasn't zero. Available to simulated players. Used for pivoting and animation sync. */
	UPROPERTY(BlueprintReadOnly, Category = Character)
	FVector PreviousNonZeroInput;

	/** 0 is orient to view, 1 is any movement facing movement system. */
	float MovementSystemViewAlpha;

	/**
	 * True if currently in ragdoll
	 * Available to all valid net roles
	 */
	UPROPERTY(ReplicatedUsing="OnRep_Ragdoll")
	bool bRagdoll;

	bool bRagdollStandingUp;

	/** THe current ragdoll location on local client */
	UPROPERTY()
	FVector RagdollLocation;

	/** 
	 * The current actor location while in ragdoll
	 * Available to everyone (simulated, auton, server)
	 */
	UPROPERTY(Replicated)
	FVector RagdollActorLocation;

	/** Used to compare locations so we don't send updates when a small change occurs */
	FVector LastNetSendRagdollActorLocation;

	/** For first 10 seconds, ragdoll corrects more aggressively to handle initial impulse */
	FTimerHandle RagdollCorrectionTimerHandle;

	FTimerHandle RagdollStandUpTimerHandle;

protected:
	FVector PendingInput;

private:
	// Replicated property cache

	FVector SavedRagdollActorLocation;
	ECollisionResponse SavedResponseToPawn;
	ECollisionChannel SavedObjectType;
	ECollisionEnabled::Type SavedCollisionEnabled;

public:
	/** NetMode at time of entering Ragdoll - Unpossessing a server changes GetLocalRole() to client! */
	ENetMode RagdollNetMode;

public:
	AMICharacter(const FObjectInitializer& OI);

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	/** @return ragdoll velocity when in ragdoll */
	virtual FVector GetVelocity() const override;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreNetReceive() override;
	virtual void PostNetReceive() override;

	UFUNCTION()
	virtual void OnRep_InitialOffset();

	/** @return MICharacterMovement subobject **/
	FORCEINLINE UMICharacterMovementComponent* GetMICharacterMovement() const { return MICharacterMovement; }

	/** @return MIViewComponent subobject **/
	FORCEINLINE UMIViewComponent* GetMIViewComponent() const { return MIViewComponent; }

	UFUNCTION(BlueprintPure, Category = Character)
	FORCEINLINE EMIMovementState GetMIMovementState() const
	{
		if (bRagdoll || bRagdollStandingUp) { return EMIMovementState::MS_Ragdoll; }
		if (bIsCrouchRunning) {	return EMIMovementState::MS_CrouchRunning; }
		if (bIsFloorSliding) { return EMIMovementState::MS_FloorSliding; }
		if (bIsCrouched) { return EMIMovementState::MS_Crouching; }
		if (bIsSprinting) { return EMIMovementState::MS_Sprinting; }

		return EMIMovementState::MS_None;
	}

	/**
	 * Override to return information about current weapon
	 * Used by animation instance to offset the weapon position and grip with left hand properly
	 */
	UFUNCTION(BlueprintNativeEvent, Category = Weapon)
	FMIWeapon GetWeaponAnimInfo() const;

	/**
	 * If returning a physical material, will take absolute priority
	 * Perfect for having footsteps with special FX / etc. for when playing abilities
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Physical Impact")
	UMIPhysicalMaterial* GetPhysicalMaterialOverride() const;

protected:
	UPROPERTY(Transient, DuplicateTransient)
	TArray<UCameraComponent*> CachedCameras;

public:
	/** 
	 * Must return whether each applicable character state is active
	 * @see CharacterStates
	 */
	UFUNCTION(BlueprintNativeEvent, Category = ViewComponent)
	bool IsViewComponentStateActive(uint8 State) const;

	/** Must return all cameras used by the character */
	UFUNCTION(BlueprintNativeEvent, Category = ViewComponent)
	TArray<UCameraComponent*> GetViewComponentStateCameras() const;

	/** Notify ViewComponent that character state has changed to a new state that it handles */
	void OnCharacterStateChanged();

	/** Camera blending logic for ViewComponent */
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;

public:
	/**
	 * Change the current movement system
	 * NOT replicated (you should handle this yourself)
	 */
	UFUNCTION(BlueprintCallable, Category = Movement)
	void SetMovementSystem(EMIMovementSystem NewMovement);

public:
	/** 
	 * Call instead of AddMovementInput when applying forward input 
	 * Pass the exact same information you would pass to AddMovementInput

	* Add movement input along the given world direction vector (usually normalized) scaled by 'ScaleValue'. If ScaleValue < 0, movement will be in the opposite direction.
	 * Base Pawn classes won't automatically apply movement, it's up to the user to do so in a Tick event. Subclasses such as Character and DefaultPawn automatically handle this input and move.
	 *
	 * @param WorldDirection	Direction in world space to apply input
	 * @param ScaleValue		Scale to apply to input. This can be used for analog input, ie a value of 0.5 applies half the normal value, while -1.0 would reverse the direction.
	 * @param bForce			If true always add the input, ignoring the result of IsMoveInputIgnored().
	 * @see GetPendingMovementInputVector(), GetLastMovementInputVector(), ConsumeMovementInputVector()
	 */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void AddForwardMovementInput(FVector WorldDirection, float ScaleValue, bool bForce = false);

	/**
	 * Call instead of AddMovementInput when applying right input
	 * Pass the exact same information you would pass to AddMovementInput

	* Add movement input along the given world direction vector (usually normalized) scaled by 'ScaleValue'. If ScaleValue < 0, movement will be in the opposite direction.
	 * Base Pawn classes won't automatically apply movement, it's up to the user to do so in a Tick event. Subclasses such as Character and DefaultPawn automatically handle this input and move.
	 *
	 * @param WorldDirection	Direction in world space to apply input
	 * @param ScaleValue		Scale to apply to input. This can be used for analog input, ie a value of 0.5 applies half the normal value, while -1.0 would reverse the direction.
	 * @param bForce			If true always add the input, ignoring the result of IsMoveInputIgnored().
	 * @see GetPendingMovementInputVector(), GetLastMovementInputVector(), ConsumeMovementInputVector()
	 */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void AddRightMovementInput(FVector WorldDirection, float ScaleValue, bool bForce = false);

	/** Called from UMICharacterMovementComponent::ConsumeInputVector()
	 * This maintains the same entry points used by CMC to ensure that the raw input values are processed at
	 * the exact same time as Acceleration so nothing is ever out of sync 
	 */
	void HandleConsumeInputVector();

	/** Sends compressed local inputs to server, which is then replicated to proxies */
	UFUNCTION(Server, Unreliable, WithValidation)
	void SendInputToServer(float X, float Y);

	/**
	 * Called whenever anim instance changes
	 */
	virtual void OnAnimInstanceChanged(UAnimInstance* const PreviousAnimInstance);

	/**
	 * Called whenever anim instance changes
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Animation, meta = (DisplayName = "On Anim Instance Changed"))
	void K2_OnAnimInstanceChanged();

	/** Used to interpolate back to facing direction */
	UFUNCTION()
	void OnStopMontage(UAnimMontage* Montage, bool bInterrupted);

	virtual void Tick(float DeltaTime);

	void TickShotImpacts(float DeltaTime);

	void TickRagdoll();

	void RagdollTraceGround(FHitResult& Hit, const FVector& InRagdollLocation);

	/** Suitable entry-point for prediction related abilities */
	virtual void CheckJumpInput(float DeltaTime) override;

	virtual bool CanJumpInternal_Implementation() const override;

	virtual void Jump() override;

	/** Used to make the character interpolate facing rotation when changing MovementSystem to OrientToView */
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime /* = 0.f */) override;

	UFUNCTION()
	void OnRep_SimulatedPivot();

public:
	/** Call to trigger physical animation from being hit or shot */
	UFUNCTION(BlueprintCallable, Category = "Physical Impact|Take Shot")
	void TakeShotPhysicsImpact(FName BoneName, USkeletalMeshComponent* HitMesh, const FVector& HitNormal, const float HitMagnitude);

public:
	/** Override to notify when weapon is being aimed (used by animation blueprint) */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Weapon)
	bool IsAimingWeapon() const;

public:
	/**
	 * True if currently in ragdoll
	 * Available to everyone (simulated, auton, server)
	 */
	UFUNCTION(BlueprintPure, Category = Ragdoll)
	FORCEINLINE bool IsRagdoll() const
	{
		return bRagdoll;
	}

	/**
	 * True if ragdoll is currently standing up
	 */
	UFUNCTION(BlueprintPure, Category = Ragdoll)
	FORCEINLINE bool IsRagdollStandingUp() const
	{
		return bRagdollStandingUp;
	}

	/** Override to return true if we can enter ragdoll. Always true by default. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Ragdoll)
	bool CanStartRagdoll() const;

	/** Override to return true if we can leave ragdoll and stand back up. Always true by default. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Ragdoll)
	bool CanStopRagdoll() const;

	UFUNCTION()
	void OnRep_Ragdoll();

	/** Call this to enter ragdoll */
	UFUNCTION(BlueprintCallable, Category = Ragdoll)
	void StartRagdoll();

	/** Call this to stop ragdoll and stand back up */
	UFUNCTION(BlueprintCallable, Category = Ragdoll)
	void StopRagdoll();

	void OnStartRagdoll();
	void OnStopRagdoll();

	UFUNCTION()
	void OnRagdollStandUp();

	/** Called when starting to ragdoll */
	UFUNCTION(BlueprintImplementableEvent, Category = Ragdoll, meta = (DisplayName = "On Start Ragdoll"))
	void K2_OnStartRagdoll();

	/** Called when stopping ragdoll and standing up */
	UFUNCTION(BlueprintImplementableEvent, Category = Ragdoll, meta = (DisplayName = "On Stop Ragdoll"))
	void K2_OnStopRagdoll();

	/** Called when ragdoll finishes standing up */
	UFUNCTION(BlueprintImplementableEvent, Category = Ragdoll, meta = (DisplayName = "On Ragdoll Stand Up"))
	void K2_OnRagdollStandUp();

	/** Send actor location to server during ragdoll */
	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerReceiveRagdoll(const FVector& NewRagdollActorLocation);

	/** Set actor location during ragdoll */
	void UpdateRagdoll();

public:
	virtual void OnPivot()
	{
		K2_OnPivot();
	}

	/** Called any time the character pivots (changes direction, causing animation and brief movement changes */
	UFUNCTION(BlueprintImplementableEvent, Category = Movement, meta = (DisplayName = "On Pivot"))
	void K2_OnPivot();

	virtual bool CanStartSprinting() const;

	/** 
	 * Override to change when character can sprint
	 
	 * Default conditions:
	 * - Not being destroyed
	 * - Is providing input
	 * - Input is suitably in direction of character facing direction @see UMICharacterMovement::GetMaxSprintDirectionInputNormal()
	 * - When strafing orientation is set to right, Input isn't moving backwards
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category=Sprint)
	bool CanSprint() const;

	/** True if sprinting and moving fast enough to actually enter sprinting state (always true by default) */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category=Sprint)
	bool IsSprinting() const;

	/**
	 * Request the character to start sprinting.
	 * @see OnStartSprinting
	 * @see IsSprinting
	 * @see MICharacterMovement->WantsToSprint
	 */
	UFUNCTION(BlueprintCallable, Category=Sprint)
	virtual void Sprint();

	/**
	 * Request the character to stop sprinting.
	 * @see OnStopSprinting
	 * @see IsSprinting
	 * @see MICharacterMovement->WantsToSprint
	 */
	UFUNCTION(BlueprintCallable, Category=Sprint)
	virtual void StopSprinting();

	void OnStartSprint();
	void OnStopSprint();

	virtual void OnStartSprinting() {}
	virtual void OnStopSprinting() {}

	/** Called when character starts to sprint */
	UFUNCTION(BlueprintImplementableEvent, Category=Sprint, meta = (DisplayName = "On Start Sprint"))
	void K2_OnStartSprint();

	/** Called when character stops sprinting */
	UFUNCTION(BlueprintImplementableEvent, Category = Sprint, meta = (DisplayName = "On Stop Sprint"))
	void K2_OnStopSprint();

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	/** Handle Sprinting replicated from server */
	UFUNCTION()
	virtual void OnRep_IsSprinting();

	// Crouch running

	/** 
	 * Override to change when character can crouch run
	 
	 * Default conditions:
	 * - bCanEverCrouchRun is true
	 * - Not being destroyed
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="Crouch Run")
	bool CanCrouchRun() const;

	void OnStartCrouchRun();

	void OnStopCrouchRun();

	/** @return true if this character is currently able to crouch (and is not currently crouched) */
	virtual bool CanCrouch() const override;

	virtual void Crouch(bool bClientSimulation = false) override;
	virtual void UnCrouch(bool bClientSimulation = false) override;

	// Used here to trigger Crouching if required after landing (OnLanded is still considered falling)
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);

	/** Called when character starts to Crouch Run */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crouch Run", meta = (DisplayName = "On Start Crouch Run"))
	void K2_OnStartCrouchRun();

	/** Called when character stops Crouch Runing */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crouch Run", meta = (DisplayName = "On Stop Crouch Run"))
	void K2_OnStopCrouchRun();

	/** Handle Crouch running replicated from server */
	UFUNCTION()
	virtual void OnRep_IsCrouchRunning();

	// Floor sliding
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Floor Slide")
	bool CanStartFloorSlide() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Floor Slide")
	bool CanContinueFloorSlide() const;

	UFUNCTION(BlueprintPure, Category = "Floor Slide")
	bool IsFloorSliding() const;

	virtual void OnStartFloorSlide();

	virtual void OnStopFloorSlide();

	/**
	 * If true and conditions for floor sliding are met, will floor slide regardless of input
	 * @see CanContinueFloorSlide() - you will need to make sure it can floor slide when it should
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Floor Slide")
	bool ShouldAutoFloorSlide() const;

	/** Called when character starts to floor slide */
	UFUNCTION(BlueprintImplementableEvent, Category= "Floor Slide", meta = (DisplayName = "On Start Floor Slide"))
	void K2_OnStartFloorSlide();

	/** Called when character stops floor sliding */
	UFUNCTION(BlueprintImplementableEvent, Category = "Floor Slide", meta = (DisplayName = "On Stop Floor Slide"))
	void K2_OnStopFloorSlide();

	/** Handle Floor Sliding replicated from server */
	UFUNCTION()
	virtual void OnRep_IsFloorSliding();

public:
	/**
	 * @return True if movement should be cycled
	 * Cycling provides a minimum turning circle preventing spinning on the spot
	 * By default is True if MovementSystem is CycleOrientToMovement
	 */
	UFUNCTION(BlueprintNativeEvent, Category = CharacterMovement)
	bool ShouldCycleMovement() const;

public:
	/** @return True if character should orient to the floor */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category= OrientToFloor)
	bool ShouldOrientToFloor() const;

	UFUNCTION(BlueprintPure, Category = CharacterMovement)
	bool IsOnWalkableFloor() const;

	UFUNCTION(BlueprintPure, Category = CharacterMovement)
	bool IsCurrentFloorMovable() const;

	/** Settings for currently orienting to the floor */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = OrientToFloor)
	FOrientToFloorSettings GetOrientToFloorSettings() const;

public:
	bool IsValidLOD(const int32& LODThreshold) const;

	UFUNCTION()
	void OnCapsuleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void OnCollideWith(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void HandleMeshImpact(FPhysicsBlend& ImpactPhysics, const FVector& ImpactNormal, const FVector& ImpactVelocity);

	void HandleImpactCharacter(AMICharacter* OtherCharacter, const FVector& ImpactNormal, const FVector& ImpactVelocity, bool bInstigator);

	UFUNCTION(BlueprintNativeEvent, Category = "Physical Impact|Character")
	bool CanPlayHitByCharacterVoice() const;

	/** Always returns true by default, used for custom logic to override */
	UFUNCTION(BlueprintNativeEvent, Category = "Physical Impact|Hit Wall")
	bool CanHitWall() const;
	virtual bool CanHitWall_Implementation() const { return true; }

	/** Always returns true by default, used for custom logic to override */
	UFUNCTION(BlueprintNativeEvent, Category = "Physical Impact|Hit Wall")
	bool CanScuffWall() const;
	virtual bool CanScuffWall_Implementation() const { return true; }

	/** Always returns true by default, used for custom logic to override */
	UFUNCTION(BlueprintNativeEvent, Category = "Physical Impact|Character")
	bool CanHitCharacter();
	virtual bool CanHitCharacter_Implementation() const { return true; }
};