#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AetherburnDuelAIController.generated.h"

UCLASS()
class AETHERBURN_API AAetherburnDuelAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAetherburnDuelAIController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Duel AI|Combat", meta=(ClampMin="100", Units="cm"))
	float PreferredRange = 1450.0f;
	UPROPERTY(EditDefaultsOnly, Category="Duel AI|Combat", meta=(ClampMin="100", Units="cm"))
	float MinimumRange = 850.0f;
	UPROPERTY(EditDefaultsOnly, Category="Duel AI|Combat", meta=(ClampMin="100", Units="cm"))
	float AttackRange = 3600.0f;
	UPROPERTY(EditDefaultsOnly, Category="Duel AI|Combat", meta=(ClampMin="0.1", Units="s"))
	float AttackInterval = 1.65f;
	UPROPERTY(EditDefaultsOnly, Category="Duel AI|Respawn", meta=(ClampMin="0.1", Units="s"))
	float EnemyRespawnDelay = 3.5f;

private:
	void RespawnEnemy();
	bool HasClearShot(const FVector& Start, const FVector& End, AActor* Target) const;

	TWeakObjectPtr<APawn> TargetPawn;
	UPROPERTY()
	TSubclassOf<APawn> RespawnPawnClass;
	FTransform EnemySpawnTransform = FTransform::Identity;
	FTimerHandle EnemyRespawnTimer;
	float NextAttackTime = 0.0f;
	bool bHasSpawnTransform = false;
	bool bRespawnScheduled = false;
};
