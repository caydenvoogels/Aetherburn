#include "ThunderlordAnimInstance.h"
#include "ThunderlordAnimationSet.h"
#include "AetherburnCharacter.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
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
	const bool bThrowMontageActive = ActiveThrowMontage && Montage_IsPlaying(ActiveThrowMontage);
	ThrowLayerWeight = FMath::FInterpTo(
		ThrowLayerWeight, bThrowMontageActive ? 1.0f : 0.0f, DeltaSeconds, 18.0f);
	if (!bThrowMontageActive && ThrowLayerWeight <= KINDA_SMALL_NUMBER)
	{
		ThrowLayerWeight = 0.0f;
		ActiveThrowMontage = nullptr;
	}

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

float UThunderlordAnimInstance::PlayBoltThrowAnimation()
{
	UAnimSequence* ThrowClip = AnimationSet ? AnimationSet->Actions.FindRef(TEXT("Throw")) : nullptr;
	if (!ThrowClip)
	{
		UE_LOG(LogTemp, Warning, TEXT("Thunderlord throw animation is missing from the animation set"));
		return 0.0f;
	}
	const float SafePlayRate = FMath::Max(0.1f, ThrowPlayRate);
	UAnimMontage* Montage = PlaySlotAnimationAsDynamicMontage(
		ThrowClip, TEXT("UpperBodyThrow"), 0.06f, 0.12f, SafePlayRate, 1, -1.0f, 0.0f);
	if (!Montage)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not start throw montage in slot UpperBodyThrow"));
		return 0.0f;
	}

	const bool bMontagePlaying = Montage_IsPlaying(Montage);
	ActiveThrowMontage = bMontagePlaying ? Montage : nullptr;
	ThrowLayerWeight = bMontagePlaying ? 1.0f : 0.0f;
	const float ReleaseDelay = ThrowClip->GetPlayLength() * ThrowReleaseFraction / SafePlayRate;
	UE_LOG(LogTemp, Log,
		TEXT("Thunderlord throw montage %s length=%.2f rate=%.2f release=%.2fs playing=%d layer=%.1f"),
		*GetNameSafe(ThrowClip), ThrowClip->GetPlayLength(), SafePlayRate, ReleaseDelay,
		bMontagePlaying, ThrowLayerWeight);
	return ReleaseDelay;
}
