#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ThunderlordAnimInstance.generated.h"

class UThunderlordAnimationSet;
class UAnimMontage;

/** Runtime state and data for the Thunderlord Animation Blueprint state machine. */
UCLASS(Transient, Blueprintable)
class AETHERBURN_API UThunderlordAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UThunderlordAnimInstance();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<UThunderlordAnimationSet> AnimationSet;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Throw", meta=(ClampMin="0.1", ClampMax="0.9"))
	float ThrowReleaseFraction = 0.30f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Throw", meta=(ClampMin="0.1", ClampMax="3.0"))
	float ThrowPlayRate = 1.5f;
	/** Animated weight that enables the upper-body slot only during a throw. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Throw", meta=(BlueprintThreadSafe))
	float ThrowLayerWeight = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
	FName MovementState;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
	float Direction = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
	float Speed = 0;
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe), Category="Locomotion|Transitions") bool IsLocomotionState() const;
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe), Category="Locomotion|Transitions") bool IsCrouchState() const;
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe), Category="Locomotion|Transitions") bool IsSlideState() const;
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe), Category="Locomotion|Transitions") bool IsJumpState() const;
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe), Category="Locomotion|Transitions") bool IsFallState() const;
	/** Starts the Mixamo throw clip in the upper-body montage slot. */
	float PlayBoltThrowAnimation();
protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
private:
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveThrowMontage;
	bool bWasFalling = false;
	bool bWasSliding = false;
	float JumpStateRemaining = 0.0f;
	float SlideStateRemaining = 0.0f;
};
