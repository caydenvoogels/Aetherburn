#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThunderlordBoltProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class AETHERBURN_API AThunderlordBoltProjectile : public AActor
{
	GENERATED_BODY()

public:
	AThunderlordBoltProjectile();
	void Launch(const FVector& Direction, float Speed);
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BoltMesh;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	float TravelledDistance = 0.0f;
	FVector PreviousLocation = FVector::ZeroVector;
	bool bHasHit = false;
};
