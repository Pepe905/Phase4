// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNodes_Shared.h"
#include "Animation/AnimInstanceProxy.h"

void FAnimNodeStatics::DrawDebugFootBox(FPrimitiveDrawInterface* PDI, const FVector& Center, const FVector& Box, const FQuat& Rotation, const FColor& Color, float Thickness /*= 0.5f*/, ESceneDepthPriorityGroup DepthPriority /*= SDPG_World*/)
{
	const FTransform Transform(Rotation);
	FVector Start = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	FVector End = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);
}

void FAnimNodeStatics::DrawDebugFootBox(FAnimInstanceProxy* Proxy, const FVector& Center, const FVector& Box, const FQuat& Rotation, const FColor& Color, float Thickness /*= 0.5f*/)
{
	const FTransform Transform(Rotation);
	FVector Start = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	FVector End = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);
}

void FAnimNodeStatics::DrawDebugLocator(FPrimitiveDrawInterface* PDI, const FVector& Center, const FQuat& Rotation, float LocatorSize, const FColor& Color, float Thickness /* = 0.5f */, ESceneDepthPriorityGroup DepthPriority /* = SDPG_World */)
{
	const FTransform Transform(Rotation);
	FVector Start = Transform.TransformPosition(FVector(LocatorSize, 0.f, 0.f));
	FVector End = Transform.TransformPosition(FVector(-LocatorSize, 0.f, 0.f));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(0.f, LocatorSize, 0.f));
	End = Transform.TransformPosition(FVector(0.f, -LocatorSize, 0.f));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);

	Start = Transform.TransformPosition(FVector(0.f, 0.f, LocatorSize));
	End = Transform.TransformPosition(FVector(0.f, 0.f, -LocatorSize));
	PDI->DrawLine(Center + Start, Center + End, Color, DepthPriority, Thickness);
}

void FAnimNodeStatics::DrawDebugLocator(FAnimInstanceProxy* Proxy, const FVector& Center, const FQuat& Rotation, float LocatorSize, const FColor& Color, float Thickness /* = 0.5f */)
{
	const FTransform Transform(Rotation);
	FVector Start = Transform.TransformPosition(FVector(LocatorSize, 0.f, 0.f));
	FVector End = Transform.TransformPosition(FVector(-LocatorSize, 0.f, 0.f));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(0.f, LocatorSize, 0.f));
	End = Transform.TransformPosition(FVector(0.f, -LocatorSize, 0.f));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);

	Start = Transform.TransformPosition(FVector(0.f, 0.f, LocatorSize));
	End = Transform.TransformPosition(FVector(0.f, 0.f, -LocatorSize));
	Proxy->AnimDrawDebugLine(Center + Start, Center + End, Color, false, -1.f, Thickness);
}

void FAnimNodeStatics::DrawDebugLine(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FColor& Color, float Thickness /* = 0.5f */, ESceneDepthPriorityGroup DepthPriority /* = SDPG_World */)
{
	PDI->DrawLine(Start, End, Color, DepthPriority, Thickness);
}

void FAnimNodeStatics::DrawDebugLine(FAnimInstanceProxy* Proxy, const FVector& Start, const FVector& End, const FColor& Color, float Thickness /*= 0.5f*/)
{
	Proxy->AnimDrawDebugLine(Start, End, Color, false, -1.f, Thickness);
}
