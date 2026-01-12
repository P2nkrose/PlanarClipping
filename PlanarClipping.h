#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlanarClippingComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class EVGAME_API PlanarClippingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	PlanarClippingComponent();
	
	// When it overlap the plane, collision body of the actor turns off and the mesh becomes transparent
	UFUNCTION(BlueprintCallable)
	void ActivateHiddenPlane(const FPlane& InPlane);

	// When the overlap is turned off, collision is turned on and Mesh is seen again
	UFUNCTION(BlueprintCallable)
	void DeactivateHiddenPlane();

	UFUNCTION(BlueprintCallable)
	void ActivateHiddenPlaneFromActor(AActor* PlaneActor);
	
private:
	void ApplyPlaneToMaterials();
	void UpdateSkeletalShapeCollision();

	bool IsSphereAbovePlane(const FVector& Center, float Radius);
	bool IsBoxAbovePlane(const FTransform& WorldTM, const FVector& HalfExtent);
	bool IsCapsuleAbovePlane(const FVector& Center, const FVector& Axis, float HalfLength, float Radius);

private:
	FPlane Plane = FPlane(0, 0, 0, 0);
};
