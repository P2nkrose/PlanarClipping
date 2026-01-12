#include "PlanarClippingComponent.h"

#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/AggregateGeom.h"


PlanarClippingComponent::PlanarClippingComponent()
{
	Plane = FPlane(0, 0, 0, 0);
}

void PlanarClippingComponent::ActivateHiddenPlane(const FPlane& InPlane)
{
	if (AActor* Owner = GetOwner())
	{
		SetActiveFlag(true);
		Plane = InPlane;
		ApplyPlaneToMaterials();
		UpdateSkeletalShapeCollision();
	}
}

void PlanarClippingComponent::DeactivateHiddenPlane()
{
	SetActiveFlag(false);
	Plane = FPlane(0, 0, 0, 0);
	ApplyPlaneToMaterials();
	UpdateSkeletalShapeCollision();
}

void PlanarClippingComponent::ActivateHiddenPlaneFromActor(AActor* PlaneActor)
{
	if (PlaneActor == nullptr)
	{
		return;
	}

	const FVector Point = PlaneActor->GetActorLocation();
	const FVector Normal = PlaneActor->GetActorUpVector().GetSafeNormal();

	FPlane P(Point, Normal);
	P.Normalize();

	ActivateHiddenPlane(P);
}

void PlanarClippingComponent::ApplyPlaneToMaterials()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents(Meshes);
	const FLinearColor ParamData = FLinearColor(Plane.X, Plane.Y, Plane.Z, Plane.W);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (Mesh == nullptr)
		{
			continue;
		}

		for (int i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInterface* Material = Mesh->GetMaterial(i))
			{
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
				if (MID == nullptr)
				{
					MID = Mesh->CreateAndSetMaterialInstanceDynamic(i);
				}

				if (MID)
				{
					static const FName PlaneParamName = TEXT("ClippingPlane");
					MID->SetVectorParameterValue(PlaneParamName, ParamData);
				}
			}
		}
	}
}

void PlanarClippingComponent::UpdateSkeletalShapeCollision()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* Skel = Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (Skel == nullptr)
	{
		return;
	}

	UPhysicsAsset* PA = Skel->GetPhysicsAsset();
	if (PA == nullptr)
	{
		return;
	}

	const FVector CompScale = Skel->GetComponentScale();
	const float UniformScale = Skel->GetComponentScale().GetAbsMax();
	const FTransform ActorWorldTrans = Owner->GetActorTransform();
	for (FBodyInstance* BodyInstance : Skel->Bodies)
	{
		if (BodyInstance && BodyInstance->IsValidBodyInstance())
		{
			UBodySetup* BodySetup = BodyInstance->GetBodySetup();
			const FTransform BoneWorld = BodyInstance->GetUnrealWorldTransform();
			int32 NumShapes = BodySetup->AggGeom.GetElementCount();
			for (int32 ShapeIndex = 0; ShapeIndex < NumShapes; ++ShapeIndex)
			{
				FKShapeElem* Shape = BodySetup->AggGeom.GetElement(ShapeIndex);
				bool IsAbove = false;
				if (IsActive())
				{
					if (Shape->GetShapeType() == EAggCollisionShape::Sphere)
					{
						const FKSphereElem* Sphere = static_cast<const FKSphereElem*>(Shape);
						const FVector Center = BoneWorld.TransformPosition(Sphere->Center);
						const float Radius = Sphere->Radius * UniformScale;
						IsAbove = IsSphereAbovePlane(Center, Radius);

					}
					else if (Shape->GetShapeType() == EAggCollisionShape::Box)
					{
						const FKBoxElem* Box = static_cast<const FKBoxElem*>(Shape);
						const FTransform WorldTM = Box->GetTransform() * BoneWorld;
						const FVector HalfExtent = FVector(Box->X, Box->Y, Box->Z) * 0.5f * CompScale;
						IsAbove = IsBoxAbovePlane(WorldTM, HalfExtent);
					}
					else if (Shape->GetShapeType() == EAggCollisionShape::Sphyl)
					{
						const FKSphylElem* Sphyl = static_cast<const FKSphylElem*>(Shape);
						const FTransform WorldTM = Sphyl->GetTransform() * BoneWorld;
						const FVector Center = WorldTM.GetLocation();
						const FVector Axis = WorldTM.GetUnitAxis(EAxis::X);
						const float Radius = Sphyl->Radius * UniformScale;
						const float HalfLength = (Sphyl->Length * 0.5f) * UniformScale;
						IsAbove = IsCapsuleAbovePlane(Center, Axis, HalfLength, Radius);
					}
				}
				else
				{
					IsAbove = true;
				}

				BodyInstance->SetShapeCollisionEnabled(ShapeIndex, IsAbove ? 
					ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision, true);
			}
		}
	}
}

bool PlanarClippingComponent::IsSphereAbovePlane(const FVector& Center, float Radius)
{
	return Plane.PlaneDot(Center) > Radius;
}

bool PlanarClippingComponent::IsBoxAbovePlane(const FTransform& WorldTM, const FVector& HalfExtent)
{
	const FVector C = WorldTM.GetLocation();
	const FVector A0 = WorldTM.GetUnitAxis(EAxis::X);
	const FVector A1 = WorldTM.GetUnitAxis(EAxis::Y);
	const FVector A2 = WorldTM.GetUnitAxis(EAxis::Z);

	const FVector N(Plane.X, Plane.Y, Plane.Z);

	const float r =
		FMath::Abs(FVector::DotProduct(N, A0)) * HalfExtent.X +
		FMath::Abs(FVector::DotProduct(N, A1)) * HalfExtent.Y +
		FMath::Abs(FVector::DotProduct(N, A2)) * HalfExtent.Z;

	return Plane.PlaneDot(C) > r;
}


bool PlanarClippingComponent::IsCapsuleAbovePlane(const FVector& Center, const FVector& Axis, float HalfLength, float Radius)
{
	const float s0 = Plane.PlaneDot(Center + Axis * HalfLength);
	const float s1 = Plane.PlaneDot(Center - Axis * HalfLength);

	return FMath::Min(s0, s1) > Radius;
}
