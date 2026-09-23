#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ThunderlordAnimInstance.generated.h"

class UThunderlordAnimationSet;

/** Runtime state and data for the Thunderlord Animation Blueprint state machine. */
UCLASS(Transient, Blueprintable)
class AETHERBURN_API UThunderlordAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UThunderlordAnimInstance();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<UThunderlordAnimationSet> AnimationSet;
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
protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
private:
	bool bWasFalling = false;
	bool bWasSliding = false;
	float JumpStateRemaining = 0.0f;
	float SlideStateRemaining = 0.0f;
};
