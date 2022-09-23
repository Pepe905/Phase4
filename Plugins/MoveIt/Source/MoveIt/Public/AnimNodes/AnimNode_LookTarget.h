// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "MITypes.h"
#include "AnimNode_LookTarget.generated.h"

UENUM(BlueprintType)
enum class EMILookTargetInput : uint8
{
	LTI_Actor				UMETA(DisplayName = "Actor"),
	LTI_Location			UMETA(DisplayName = "Location"),
};

struct FAngularVector
{
	FVector Axis;// vector of rotation axis, its length is AxisLength.
	double AxisLength;
	double Speed;// the magnitude of the vector (radians per sec).

	FAngularVector()
		: Axis(FVector::ZeroVector)
		, AxisLength(0.0)
		, Speed(0.0)
	{}

	FORCEINLINE bool IsValid() const
	{
		return AxisLength > DOUBLE_SMALL_NUMBER;
	}

	FORCEINLINE bool ConstructionInvalidCheck() const
	{
		return (!(Speed > DOUBLE_SMALL_NUMBER)) ||
			(!(AxisLength > DOUBLE_SMALL_NUMBER)) ||
			(!((Speed / AxisLength) > DOUBLE_SMALL_NUMBER));
	}
	FORCEINLINE FVector To3D(const float Seconds = 1.f) const
	{
		return (!IsValid()) ? FVector{ 0.0,0.0,0.0 } : (Axis * (Seconds * (Speed / AxisLength)));
	}

	FQuat ToQuat(double Seconds = 1.0) const
	{
		FQuat Ret;
		if (IsValid())
		{
			bool bNegate{ Seconds < -0.0 };
			double Angle{ (bNegate ? -Seconds : Seconds) * Speed };

			if (Angle > (2 * DOUBLE_PI))
			{
				Angle = FMath::Fmod(Angle, 2 * DOUBLE_PI);
			}

			if (Angle > DOUBLE_PI)
			{
				Angle = -(Angle - (2 * DOUBLE_PI));
				bNegate = !bNegate;
			}

			FMath::SinCos(&Ret.Z, &Ret.W, Angle / 2.0);
			double const Scale{ Ret.Z / AxisLength };

			if (bNegate)
			{
				Ret.X = -(Axis.X * Scale);
				Ret.Y = -(Axis.Y * Scale);
				Ret.Z = -(Axis.Z * Scale);
			}
			else
			{
				Ret.X = (Axis.X * Scale);
				Ret.Y = (Axis.Y * Scale);
				Ret.Z = (Axis.Z * Scale);
			}
		}
		else
		{
			Ret.X = 0.0;
			Ret.Y = 0.0;
			Ret.Z = 0.0;
			Ret.W = 1.f;
		}
		return Ret;
	}

	FORCEINLINE FVector RotateAtTime(const FVector& In, const float seconds = 1.f)
	{
		const FQuat Rot{ ToQuat(seconds) };
		return Rot.RotateVector(In);
	}

	FORCEINLINE FAngularVector(const FAngularVector&) = default;
	FORCEINLINE FAngularVector(FAngularVector&&) = default;

	FORCEINLINE FAngularVector& operator =(const FAngularVector&) = default;
	FORCEINLINE FAngularVector& operator =(FAngularVector&&) = default;

	FORCEINLINE FAngularVector(
		const FVector& axis,
		const float& axisLength,
		const float& speed
	) : Axis{ axis }, AxisLength{ axisLength }, Speed{ speed }{

	}
	FORCEINLINE FAngularVector(
		const FQuat& Difference,
		const float deltaTime = 1.f,
		const double MaxSpeed = 0.0)
		: Axis{ Difference.X,Difference.Y,Difference.Z }
		, AxisLength{ Axis.Size() }
		, Speed(FMath::Acos(Difference.W) * 2.0) {

		if (!(deltaTime >= DOUBLE_SMALL_NUMBER))
		{
			Axis = FVector{ 0.0,0.0,0.0 };
			Speed = 0.0;
			AxisLength = 0.0;
			return;
		}

		if (Speed > DOUBLE_PI)
		{

			Speed = -(Speed - (2.0 * DOUBLE_PI));

			Axis.X = -Axis.X;
			Axis.Y = -Axis.Y;
			Axis.Z = -Axis.Z;
		}

		AxisLength = Axis.Size();

		Speed /= deltaTime;

		if ((MaxSpeed > 0.0) && !(Speed <= MaxSpeed))
		{
			Speed = MaxSpeed;
		}

		if (ConstructionInvalidCheck())
		{
			Axis = FVector{ 0.0,0.0,0.0 };
			Speed = 0.0;
			AxisLength = 0.0;
		}
	}

	enum ENormals { BetweenNormals };
	FORCEINLINE FAngularVector(
		const ENormals Command,
		const FVector& StartNormal,
		const FVector& EndNormal,
		const float deltaTime = 1.f,
		const float MaxSpeed = 0.0)
		: FAngularVector{ FQuat::FindBetweenNormals(StartNormal, EndNormal), deltaTime, MaxSpeed }
	{}
	enum EVectors { BetweenVectors };

	FORCEINLINE FAngularVector(
		const EVectors Command,
		const FVector& StartNormal,
		const FVector& EndNormal,
		const float deltaTime = 1.f,
		const float MaxSpeed = 0.0)
		: FAngularVector{ FQuat::FindBetweenVectors(StartNormal, EndNormal), deltaTime, MaxSpeed }
	{}


	explicit FORCEINLINE FAngularVector(
		const FVector& Value3D
	) : Axis{ Value3D }, AxisLength{ Axis.Size() }, Speed{ AxisLength }
	{
		if (ConstructionInvalidCheck())
		{
			Axis = FVector{ 0.0,0.0,0.0 };
			Speed = 0.0;
			AxisLength = 0.0;
		}
	}

	static FORCEINLINE bool Interpolate(
		FAngularVector& Out,
		const FAngularVector& A,
		const FAngularVector& B,
		const float T)
	{

		if (A.IsValid())
		{
			if (B.IsValid())
			{

				const FVector NormalA{ A.Axis / A.AxisLength };
				const FVector NormalB{ B.Axis / B.AxisLength };

				// angular slerping..
				FAngularVector Between{
					BetweenNormals,
					NormalA,NormalB
				};

				const FQuat Rot{
						Between.ToQuat(T)
				};

				Rot.RotateVector(A.Axis);
				Out.AxisLength = A.AxisLength;
				Out.Speed = A.Speed + (B.Speed - A.Speed) * T;

				if (Out.ConstructionInvalidCheck())
				{
					Out.Axis = FVector{ 0.0,0.0,0.0 };
					Out.Speed = 0.0;
					Out.AxisLength = 0.0;
					return false;
				}

				return true;
			}
			else
			{
				Out = A;
				return false;
			}
		}
		else
		{
			Out = B;
			return false;
		}
	}
};

struct FBoneSpringDriver
{
	FVector Position;// the direction
	FAngularVector Velocity;
	FAngularVector Acceleration;
	bool bRunning{ false };

	FORCEINLINE void ReadInput(
		FVector direction,
		float deltaTime)
	{

		if (!bRunning)
		{
			Position = direction;

			if (Position.Normalize())
			{

				Velocity.Axis = FVector{ 0.0,0.0,0.0 };
				Velocity.AxisLength = 0.0;
				Velocity.Speed = 0.0;

				Acceleration.Axis = FVector{ 0.0,0.0,0.0 };
				Acceleration.AxisLength = 0.0;
				Acceleration.Speed = 0.0;
				bRunning = true;
			}
		}
		else if (direction.Normalize())
		{

			FAngularVector const OldVelocity{ Velocity };

			Velocity = FAngularVector{
				FAngularVector::BetweenNormals,
				Position, direction,
				deltaTime
			};

			Acceleration = FAngularVector{
				(Velocity.To3D() - OldVelocity.To3D()) / deltaTime
			};

			Position = direction;
		}
	}
};

struct FBoneSpring
{
	FBoneSpringDriver Driver;
	FVector Position;// the direction
	FAngularVector Velocity;
	bool bRunning{ false };

	FBoneSpring()
		: Driver(FBoneSpringDriver())
		, Position(FVector::ZeroVector)
		, Velocity(FAngularVector())
	{}

	void ReadInput(
		const FVector& direction,
		float deltaTime,
		const float damper = 1.f,//no damping.
		const float anchorMass = 1.f,
		const float angleStiffness = 4.f)
	{

		if (!bRunning)
		{
			Driver.ReadInput(direction, deltaTime);

			if (!Driver.bRunning)
			{
				return;
			}

			Position = Driver.Position;
			Velocity = Driver.Velocity;
			bRunning = true;
		}
		else
		{
			Driver.ReadInput(direction, deltaTime);

			const FVector AccelFromAnchor{ Driver.Acceleration.To3D() * anchorMass };

			const FVector AccelFromDelta{
				FAngularVector{
					FAngularVector::BetweenNormals,
					Position, Driver.Position
				}.To3D() * angleStiffness
			};

			Position = Velocity.RotateAtTime(Position, deltaTime);

			Velocity = FAngularVector{
				(Velocity.To3D() * damper) +
				((AccelFromAnchor + AccelFromDelta) * deltaTime)
			};
		}
	}
};

USTRUCT(BlueprintType)
struct FMILookTargetBone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = LookTarget)
	FBoneReference Bone;

	UPROPERTY(EditAnywhere, Category = LookTarget)
	float Weight;

	UPROPERTY()
	float Bias;

	FBoneSpring BoneSpring;

	FVector OldComponentDirection;
	FVector ComponentDirection;
	FVector SpringVelocity;
	FQuat TempAdvanceRot;

	FCompactPoseBoneIndex Index;

	FMILookTargetBone()
		: Bone("")
		, Weight(1.f)
		, Bias(1.f)
		, BoneSpring(FBoneSpring())
		, OldComponentDirection(FVector::ZeroVector)
		, ComponentDirection(FVector::ZeroVector)
		, SpringVelocity(FVector::ZeroVector)
		, TempAdvanceRot(FQuat::Identity)
		, Index(-1)
	{}

	FMILookTargetBone(const FBoneReference& InBone, float InWeight)
		: Bone(InBone)
		, Weight(InWeight)
		, Bias(1.f)
		, BoneSpring(FBoneSpring())
		, OldComponentDirection(FVector::ZeroVector)
		, ComponentDirection(FVector::ZeroVector)
		, SpringVelocity(FVector::ZeroVector)
		, TempAdvanceRot(FQuat::Identity)
		, Index(-1)
	{}
};

/**
 *	Procedurally looks at a given target or location
 *	Applies a spring for organic, realistic, natural motion
 *	Runs more or less on black magic
 *	Thanks to p@t from Unreal Slackers for major help with the math (he came up with almost all of it)
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_LookTarget : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	/**
	 * What the character should look at
	 * @param Actor Look at an actor
	 * @param Location Look at a position in world space
	 */
	UPROPERTY(EditAnywhere, Category = LookTarget)
	EMILookTargetInput TargetInputType;

	/** Location in world space to look at */
	UPROPERTY(EditAnywhere, Category = LookTarget, meta = (PinShownByDefault))
	FVector Location;

	/** Actor to look at */
	UPROPERTY(EditAnywhere, Category = LookTarget, meta = (PinShownByDefault))
	AActor* Target;

	/** How much strength applied when moving against the resistance of the spring (aka displacement, mass) */
	UPROPERTY(EditAnywhere, Category = Spring, meta = (PinHiddenByDefault))
	float SpringStrength;

	/** How stiff the spring is (aka tightness) */
	UPROPERTY(EditAnywhere, Category = Spring, meta = (PinHiddenByDefault))
	float SpringStiffness;

	/**
	 * Traversing between linear result and spring result
	 * 1 is no damping
	 * 0 is completely damped
	 */
	UPROPERTY(EditAnywhere, Category = Spring, meta = (PinHiddenByDefault))
	float SpringDamping;

	/**
	 * The frequency at which the solver plays
	 * Lower frequencies are prone to issues but are more performant
	 * Changing the frequency will greatly impact the outcome
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Spring)
	float SolverFrequency;

	/** Twisting motion along the aim axis (X) */
	UPROPERTY(EditAnywhere, Category = LookTarget, meta = (PinHiddenByDefault, UIMin = "0", ClampMin = "0", UIMax = "180", ClampMax = "180"))
	float TwistLimit;

	/** Side-to-side motion along the roll axis (Z) */
	UPROPERTY(EditAnywhere, Category = LookTarget, meta = (PinHiddenByDefault, UIMin = "0", ClampMin = "0", UIMax = "180", ClampMax = "180"))
	float RollLimit;

	/** Back and forward motion along the forward axis (Y) */
	UPROPERTY(EditAnywhere, Category = LookTarget, meta = (PinHiddenByDefault, UIMin = "0", ClampMin = "0", UIMax = "180", ClampMax = "180"))
	float ArchLimit;

	/** Z Height Limit for Arching */
	UPROPERTY(EditAnywhere, Category = LookTarget, meta = (PinHiddenByDefault))
	float ArchZOffsetLimit;

	/** The socket that represents where the character's eyes are */
	UPROPERTY(EditAnywhere, Category = LookTarget)
	FName EyeLevelSocket;

	/** Bones that look at the target. Result is averaged between all added bones and can be re-biased using the Weight */
	UPROPERTY(EditAnywhere, Category = LookTarget)
	TArray<FMILookTargetBone> Bones;

	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookTarget, meta = (PinHiddenByDefault))
	bool bWorkOutsidePIE;

protected:
	/** Internal use - Amount of time we need to simulate. */
	float RemainingTime;

	/** Internal use - Current timestep */
	float FixedTimeStep;

	/** Internal use - Current time dilation */
	float TimeDilation;

	/** Internal use - True if need to reinitialize */
	bool bReinitialize;

public:
	FAnimNode_LookTarget()
		: TargetInputType(EMILookTargetInput::LTI_Actor)
		, Location(FVector::ZeroVector)
		, Target(nullptr)
		, SpringStrength(0.1f)
		, SpringStiffness(28.f)
		, SpringDamping(0.99f)
		, SolverFrequency(480.0)
		, TwistLimit(85.f)
		, RollLimit(15.f)
		, ArchLimit(10.0)
		, ArchZOffsetLimit(0.0)
		, EyeLevelSocket(TEXT("eye_level"))
		, Bones({ 
				FMILookTargetBone(FBoneReference("spine_01"), 0.5f), 
				FMILookTargetBone(FBoneReference("spine_02"), 0.75f),
				FMILookTargetBone(FBoneReference("spine_03"), 1.f),
				FMILookTargetBone(FBoneReference("neck_01"), 1.25f),
				FMILookTargetBone(FBoneReference("head"), 1.25f) 
			})
		, bWorkOutsidePIE(false)
		, RemainingTime(0.0)
		, FixedTimeStep(0.0)
		, TimeDilation(1.f)
		, bReinitialize(true)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.5f;
		AlphaBoolBlend.BlendOutTime = 0.5f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;
	}

protected:
	// FAnimNode_LocalSkeletalControlBase interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual void ResetDynamics(ETeleportType InTeleportType) final;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) final;
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float BlendWeight) final;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_LocalSkeletalControlBase interface

	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) final;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) final;

	FORCEINLINE float ClampAngle180(double InputPositive, const double LimitPositive)
	{
		if (LimitPositive < (DOUBLE_PI - DOUBLE_KINDA_SMALL_NUMBER))
		{
			constexpr double BackFract = 0.25;// for the rotation possible to hold back, this fraction will stick.

			if (InputPositive > LimitPositive)
			{
				const double AngleToRear{ DOUBLE_PI - LimitPositive };
				const double StartMovingBack = LimitPositive + (AngleToRear * BackFract);
				const double AngleToBackFromStart{ DOUBLE_PI - StartMovingBack };

				if (InputPositive > StartMovingBack && (AngleToBackFromStart >= DOUBLE_KINDA_SMALL_NUMBER))
				{
					return LimitPositive * (1.0 - ((InputPositive - StartMovingBack) / AngleToBackFromStart));
				}
				else
				{
					return LimitPositive;
				}
			}
		}
		return InputPositive;
	}

	void PointBoneAt(
		FVector const& ComponentDirection,
		FTransform& ComponentBone,
		const FTransform& ComponentTransform)
	{
		// the axis we match to animation.
		FVector AnimatedAxis{ ComponentBone.TransformVectorNoScale(FVector{1.0,0.0,0.0}) };//X

		FQuat LookDirection;

		// when the angle of the look direction is too directly with, or directly against the animated axis we'll use the alternate axis animated.
		if ((1.f - FMath::Abs(AnimatedAxis | ComponentDirection)) <= DOUBLE_KINDA_SMALL_NUMBER)
		{
			AnimatedAxis = ComponentBone.TransformVectorNoScale(FVector{ 0.0,0.0,1.0 });//Z
			LookDirection = FRotationMatrix::MakeFromYZ(ComponentDirection, AnimatedAxis).ToQuat();
		}
		else
		{
			LookDirection = FRotationMatrix::MakeFromYX(ComponentDirection, AnimatedAxis).ToQuat();
		}

		// now get look direction in bone space of the ANIMATED bone.
		FQuat LocalLookDirection = ComponentBone.InverseTransformRotation(LookDirection);

		// and convert it to 3D rotation.
		{
			FVector AngularVector{ LocalLookDirection.X,LocalLookDirection.Y,LocalLookDirection.Z };

			// if we can't normalize, then there is no rotation so whatever.
			if (!AngularVector.Normalize())
			{
				return;
			}
			// get the angle..
			double Angle{ FMath::Acos(LocalLookDirection.W) };

			if (Angle > (DOUBLE_PI / 2.0))
			{

				// Angle - PI will make a negative angle
				// so we keep it positive by doing the times two negatively and...
				Angle = (Angle - DOUBLE_PI) * -2.0;

				// flipping the axis.
				AngularVector = -AngularVector;
			}
			else
			{
				// no need to flip anything, W is cos(angle/2) so we just mult by 2.
				Angle *= 2.0;
			}

			FVector Rotation3D{ AngularVector * Angle };

			//60 deg from animated direction means a cone of 120 degrees around X.
			const float LimitX = (TwistLimit * DOUBLE_PI) / 180.0;		// Twist axis
			const float LimitY = (RollLimit * DOUBLE_PI) / 180.0;		// Roll axis (left/right bank)
			const float LimitZ = (ArchLimit * DOUBLE_PI) / 180.0;

			const float LimitZOffset = (ArchZOffsetLimit * DOUBLE_PI) / 180.0;//let the back arch more forward than reverse.

			const FVector Rotation3DIn{ Rotation3D };

			Rotation3D.X = Rotation3D.X < 0.0 ? -ClampAngle180(-Rotation3D.X, LimitX) : ClampAngle180(Rotation3D.X, LimitX);
			Rotation3D.Y = Rotation3D.Y < 0.0 ? -ClampAngle180(-Rotation3D.Y, LimitY) : ClampAngle180(Rotation3D.Y, LimitY);
			Rotation3D.Z += LimitZOffset;
			Rotation3D.Y = Rotation3D.Z < 0.0 ? -ClampAngle180(-Rotation3D.Z, LimitZ) : ClampAngle180(Rotation3D.Z, LimitZ);
			Rotation3D.Z -= LimitZOffset;

			if (Rotation3DIn.Equals(Rotation3D))
			{
				return;// no rotation.
			}

			// we are adjusting stuff.
			double NewAngle = FMath::Sqrt(Rotation3D.Size());

			const double ActualMag{ NewAngle };

			if (NewAngle > 2 * DOUBLE_PI)
			{
				NewAngle -= 2 * DOUBLE_PI;
			}

			if (NewAngle > DOUBLE_PI)
			{
				NewAngle -= 2 * DOUBLE_PI;
			}

			if (!((NewAngle > DOUBLE_SMALL_NUMBER) || (NewAngle < -DOUBLE_SMALL_NUMBER)))
			{
				return;// no rotation.
			}

			const bool bFlip{ NewAngle < 0.0 };

			if (bFlip)
			{
				NewAngle = -NewAngle;
			}

			double Sine;

			FMath::SinCos(&Sine, &LocalLookDirection.W, NewAngle / 2.0);

			if (FMath::Abs(Sine) <= DOUBLE_SMALL_NUMBER)
			{
				return;// no rotation.
			}

			float Scale = Sine / ActualMag;
			if (bFlip)
			{
				Scale = -Scale;
			}

			// bind in sine * axis.
			LocalLookDirection.X = Rotation3D.X * Scale;
			LocalLookDirection.Y = Rotation3D.Y * Scale;
			LocalLookDirection.Z = Rotation3D.Z * Scale;
		}

		// now set it back in.

		const FQuat TargetRotation = ComponentBone.TransformRotation(LocalLookDirection);
		ComponentBone.SetRotation(TargetRotation);

		ComponentBone.NormalizeRotation();
	}
};