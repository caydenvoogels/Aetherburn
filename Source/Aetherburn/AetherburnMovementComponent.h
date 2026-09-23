#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AetherburnMovementComponent.generated.h"

/** Ground sliding uses the normal swept character collision and floor solver. */
UCLASS()
class AETHERBURN_API UAetherburnMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	virtual float GetMaxSpeed() const override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slide", meta=(ClampMin="0"))
	float SlideDrag = 210.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slide", meta=(ClampMin="0"))
	float SlideGravityScale = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slide", meta=(ClampMin="300"))
	float MaximumSlideSpeed = 1800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slide", meta=(ClampMin="0", ClampMax="1"))
	float SlideSteering = 0.35f;
};
