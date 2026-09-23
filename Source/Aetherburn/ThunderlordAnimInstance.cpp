#include "ThunderlordAnimInstance.h"
#include "ThunderlordAnimationSet.h"
#include "AetherburnCharacter.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

UThunderlordAnimInstance::UThunderlordAnimInstance()
{
	RootMotionMode = ERootMotionMode::IgnoreRootMotion;
	static ConstructorHelpers::FObjectFinder<UThunderlordAnimationSet> Set(TEXT("/Game/Thunderlord/Zeus/AnimationMap/DA_ThunderlordAnimations"));
	AnimationSet = Set.Object;
}

void UThunderlordAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	AAetherburnCharacter* Player = Cast<AAetherburnCharacter>(TryGetPawnOwner());
	if (!Player || !Player->GetCharacterMovement())
	{
		MovementState = TEXT("Locomotion");
		Speed = 0.0f;
		Direction = 0.0f;
		return;
	}

	const FVector Velocity = Player->GetVelocity();
	const FVector LocalVelocity = Player->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	Speed = LocalVelocity.Size2D();
	Direction = Speed > 5.0f ? FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X)) : 0.0f;

	const bool bSliding = Player->IsSliding();
	if (bSliding && !bWasSliding)
	{
		const TObjectPtr<UAnimSequence> SlideClip = AnimationSet
			? AnimationSet->Actions.FindRef(TEXT("Slide")) : nullptr;
		SlideStateRemaining = SlideClip ? SlideClip->GetPlayLength() : 0.6f;
	}
	if (bSliding)
	{
		SlideStateRemaining = FMath::Max(0.0f, SlideStateRemaining - DeltaSeconds);
		if (SlideStateRemaining <= 0.0f)
		{
			Player->FinishSlideFromAnimation();
		}
	}
	else
	{
		SlideStateRemaining = 0.0f;
	}
	bWasSliding = Player->IsSliding();

	const bool bFalling = Player->GetCharacterMovement()->IsFalling();
	JumpStateRemaining = FMath::Max(0.0f, JumpStateRemaining - DeltaSeconds);
	if (bFalling)
	{
		if (!bWasFalling && Velocity.Z > 100.0f)
		{
			// Keep the takeoff pose for its authored duration; the next state is Fall.
			JumpStateRemaining = AnimationSet && AnimationSet->Actions.Contains(TEXT("Jump"))
				? AnimationSet->Actions.FindRef(TEXT("Jump"))->GetPlayLength() : 0.25f;
		}
		MovementState = Velocity.Z > 0.0f && JumpStateRemaining > 0.0f ? TEXT("Jump") : TEXT("Fall");
	}
	else
	{
		JumpStateRemaining = 0.0f;
		if (Player->IsSliding())
		{
			MovementState = TEXT("Slide");
		}
		else if (Player->bIsCrouched)
		{
			MovementState = TEXT("Crouch");
		}
		else
		{
			MovementState = TEXT("Locomotion");
		}
	}
	bWasFalling = bFalling;
}

bool UThunderlordAnimInstance::IsLocomotionState() const { return MovementState == TEXT("Locomotion"); }
bool UThunderlordAnimInstance::IsCrouchState() const { return MovementState == TEXT("Crouch"); }
bool UThunderlordAnimInstance::IsSlideState() const { return MovementState == TEXT("Slide"); }
bool UThunderlordAnimInstance::IsJumpState() const { return MovementState == TEXT("Jump"); }
bool UThunderlordAnimInstance::IsFallState() const { return MovementState == TEXT("Fall"); }
