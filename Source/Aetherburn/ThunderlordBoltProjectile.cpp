#include "ThunderlordBoltProjectile.h"

#include "AetherburnCharacter.h"
#include "Aetherburn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float ImpactEffectLifetime = 1.1f;
}

AThunderlordBoltProjectile::AThunderlordBoltProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(15.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->OnComponentHit.AddDynamic(this, &AThunderlordBoltProjectile::HandleHit);

	BoltMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bolt Mesh"));
	BoltMesh->SetupAttachment(Collision);
	BoltMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(
		TEXT("/Game/Thunderlord/Zeus/Weapons/SM_ZeusBolt.SM_ZeusBolt"));
	if (Mesh.Succeeded())
	{
		BoltMesh->SetStaticMesh(Mesh.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->SetUpdatedComponent(Collision);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 6000.0f;
	ProjectileMovement->ProjectileGravityScale = 0.12f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bSweepCollision = true;
	InitialLifeSpan = 8.0f;
}

void AThunderlordBoltProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Thrower = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(Thrower, true);
	}
}

void AThunderlordBoltProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!bHasHit)
	{
		if (AAetherburnCharacter* Thrower = Cast<AAetherburnCharacter>(GetOwner()))
		{
			UE_LOG(LogAetherburn, Warning, TEXT("Bolt projectile expired without a hit; restoring to thrower's hand"));
			Thrower->OnThunderlordBoltImpact();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AThunderlordBoltProjectile::Launch(const FVector& Direction, float Speed)
{
	const FVector SafeDirection = Direction.GetSafeNormal();
	SetActorRotation(SafeDirection.Rotation());
	ProjectileMovement->Velocity = SafeDirection * Speed;
}

void AThunderlordBoltProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit || !OtherActor || OtherActor == GetOwner())
	{
		return;
	}
	bHasHit = true;
	UE_LOG(LogAetherburn, Log, TEXT("Bolt projectile hit %s at %s"),
		*GetNameSafe(OtherActor), *Hit.ImpactPoint.ToCompactString());
	SetActorEnableCollision(false);
	ProjectileMovement->StopMovementImmediately();
	SetActorHiddenInGame(true);

	if (AAetherburnCharacter* Thrower = Cast<AAetherburnCharacter>(GetOwner()))
	{
		Thrower->SpawnThunderlordBoltImpact(Hit.ImpactPoint, Hit.ImpactNormal);
		Thrower->OnThunderlordBoltImpact();
	}

	Destroy();
}
