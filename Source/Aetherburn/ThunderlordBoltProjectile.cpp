#include "ThunderlordBoltProjectile.h"

#include "AetherburnCharacter.h"
#include "Aetherburn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float ImpactEffectLifetime = 1.1f;
}

AThunderlordBoltProjectile::AThunderlordBoltProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(15.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_GameTraceChannel1);
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
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bSweepCollision = true;
	InitialLifeSpan = 8.0f;
}

void AThunderlordBoltProjectile::BeginPlay()
{
	Super::BeginPlay();
	PreviousLocation = GetActorLocation();
	if (AActor* Thrower = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(Thrower, true);
	}
}

void AThunderlordBoltProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector CurrentLocation = GetActorLocation();
	TravelledDistance += FVector::Distance(PreviousLocation, CurrentLocation);
	PreviousLocation = CurrentLocation;
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
	PreviousLocation = GetActorLocation();
}

void AThunderlordBoltProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit || !OtherActor || OtherActor == GetOwner())
	{
		return;
	}
	bHasHit = true;
	TravelledDistance += FVector::Distance(PreviousLocation, Hit.Location);
	const float DamageRange = 5000.0f;
	const float MinimumDamage = 25.0f;
	const float MaximumDamage = 60.0f;
	const float DistanceFraction = FMath::Clamp(TravelledDistance / DamageRange, 0.0f, 1.0f);
	constexpr float LogCurveStrength = 6.0f;
	// This normalized inverse-log curve rises gently near launch and accelerates
	// toward the full damage value at the maximum range.
	const float DamageScale = 1.0f - FMath::Loge(
		1.0f + LogCurveStrength * (1.0f - DistanceFraction)) / FMath::Loge(1.0f + LogCurveStrength);
	const AAetherburnCharacter* CharacterTarget = Cast<AAetherburnCharacter>(OtherActor);
	const bool bHeadshot = CharacterTarget && CharacterTarget->IsHeadshotHitComponent(OtherComponent);
	float ImpactDamage = FMath::Lerp(MinimumDamage, MaximumDamage, DamageScale);
	if (bHeadshot && DamageScale >= 1.0f && CharacterTarget)
	{
		// TakeDamage applies the headshot multiplier; send enough base damage to
		// preserve the lethal-at-maximum-range rule after that multiplier.
		ImpactDamage = FMath::Max(ImpactDamage,
			CharacterTarget->GetHealth() / AAetherburnCharacter::ThunderlordHeadshotDamageMultiplier);
	}
	FHitResult DamageHit = Hit;
	if (bHeadshot)
	{
		// Preserve a head bone label for the existing dummy damage display.
		DamageHit.BoneName = CharacterTarget->GetHeadshotBoneName();
	}
	const float DealtDamage = UGameplayStatics::ApplyPointDamage(OtherActor, ImpactDamage,
		ProjectileMovement->Velocity.GetSafeNormal(), DamageHit,
		GetInstigator() ? GetInstigator()->GetController() : nullptr, this, UDamageType::StaticClass());
	UE_LOG(LogAetherburn, Log, TEXT("Bolt projectile %s hit %s for %.1f damage after %.0f units at %s"),
		bHeadshot ? TEXT("HEADSHOT") : TEXT("body"), *GetNameSafe(OtherActor), DealtDamage,
		TravelledDistance, *Hit.ImpactPoint.ToCompactString());
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
