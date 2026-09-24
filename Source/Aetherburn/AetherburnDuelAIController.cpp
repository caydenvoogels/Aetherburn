#include "AetherburnDuelAIController.h"

#include "AetherburnCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AAetherburnDuelAIController::AAetherburnDuelAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAetherburnDuelAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AAetherburnCharacter* Duelist = Cast<AAetherburnCharacter>(InPawn))
	{
		if (!bHasSpawnTransform)
		{
			EnemySpawnTransform = InPawn->GetActorTransform();
			RespawnPawnClass = InPawn->GetClass();
			bHasSpawnTransform = true;
		}
		bRespawnScheduled = false;
		TargetPawn.Reset();
		NextAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.0f : 0.0f;
		if (USkeletalMeshComponent* EnemyMesh = Duelist->GetMesh())
		{
			EnemyMesh->SetRenderCustomDepth(true);
			EnemyMesh->SetCustomDepthStencilValue(3);
		}
	}
}

void AAetherburnDuelAIController::OnUnPossess()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EnemyRespawnTimer);
	}
	Super::OnUnPossess();
}

void AAetherburnDuelAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AAetherburnCharacter* Duelist = Cast<AAetherburnCharacter>(GetPawn());
	if (!Duelist || !GetWorld())
	{
		return;
	}

	if (Duelist->GetHealth() <= 0.0f)
	{
		Duelist->StopSprint();
		if (!bRespawnScheduled && bHasSpawnTransform && RespawnPawnClass)
		{
			bRespawnScheduled = true;
			GetWorld()->GetTimerManager().SetTimer(EnemyRespawnTimer, this,
				&AAetherburnDuelAIController::RespawnEnemy, EnemyRespawnDelay, false);
		}
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAetherburnCharacter* PlayerCharacter = Cast<AAetherburnCharacter>(PlayerPawn);
	if (!PlayerPawn || PlayerPawn == Duelist || (PlayerCharacter && PlayerCharacter->GetHealth() <= 0.0f))
	{
		TargetPawn.Reset();
		Duelist->StopSprint();
		return;
	}
	TargetPawn = PlayerPawn;

	FVector AimLocation = PlayerPawn->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	AimLocation += PlayerPawn->GetVelocity() * 0.18f;
	const UCameraComponent* AimCamera = Duelist->GetThirdPersonCameraComponent();
	const FVector AimOrigin = AimCamera
		? AimCamera->GetComponentLocation()
		: Duelist->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	const FRotator AimRotation = (AimLocation - AimOrigin).Rotation();
	SetControlRotation(AimRotation);

	FVector ToPlayer = PlayerPawn->GetActorLocation() - Duelist->GetActorLocation();
	ToPlayer.Z = 0.0f;
	const float DistanceToPlayer = ToPlayer.Size();
	const FVector TowardPlayer = ToPlayer.GetSafeNormal();
	if (DistanceToPlayer > PreferredRange + 175.0f)
	{
		Duelist->StartSprint();
		Duelist->AddMovementInput(TowardPlayer, 1.0f);
	}
	else if (DistanceToPlayer < MinimumRange)
	{
		Duelist->StopSprint();
		Duelist->AddMovementInput(-TowardPlayer, 0.7f);
	}
	else
	{
		Duelist->StopSprint();
		const FVector OrbitDirection(-TowardPlayer.Y, TowardPlayer.X, 0.0f);
		const float OrbitSign = FMath::Sin(GetWorld()->GetTimeSeconds() * 0.45f) >= 0.0f ? 1.0f : -1.0f;
		Duelist->AddMovementInput(OrbitDirection, OrbitSign * 0.48f);
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (DistanceToPlayer <= AttackRange && CurrentTime >= NextAttackTime &&
		Duelist->CanThrowThunderlordBolt() && HasClearShot(AimOrigin, AimLocation, PlayerPawn))
	{
		Duelist->StopSprint();
		Duelist->ThrowThunderlordBolt();
		NextAttackTime = CurrentTime + AttackInterval;
	}
}

bool AAetherburnDuelAIController::HasClearShot(const FVector& Start, const FVector& End, AActor* Target) const
{
	if (!GetWorld() || !Target)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ThunderlordDuelLineOfSight), true, GetPawn());
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel1, QueryParams) &&
		Hit.GetActor() == Target;
}

void AAetherburnDuelAIController::RespawnEnemy()
{
	if (!GetWorld() || !RespawnPawnClass)
	{
		return;
	}

	APawn* RagdollPawn = GetPawn();
	UnPossess();
	if (RagdollPawn)
	{
		RagdollPawn->Destroy();
	}

	APawn* Replacement = GetWorld()->SpawnActorDeferred<APawn>(
		RespawnPawnClass, EnemySpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Replacement)
	{
		UE_LOG(LogTemp, Error, TEXT("Duel enemy respawn failed: could not spawn %s"), *GetNameSafe(RespawnPawnClass));
		return;
	}
	Replacement->AutoPossessAI = EAutoPossessAI::Disabled;
	UGameplayStatics::FinishSpawningActor(Replacement, EnemySpawnTransform);
	Possess(Replacement);
}
