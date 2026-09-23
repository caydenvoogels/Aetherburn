#include "AetherburnMovementComponent.h"
#include "AetherburnCharacter.h"

float UAetherburnMovementComponent::GetMaxSpeed() const
{
	const AAetherburnCharacter* Player = Cast<AAetherburnCharacter>(CharacterOwner);
	return Player && Player->IsSliding() ? MaximumSlideSpeed : Super::GetMaxSpeed();
}

void UAetherburnMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	const AAetherburnCharacter* Player = Cast<AAetherburnCharacter>(CharacterOwner);
	if (!Player || !Player->IsSliding() || !IsMovingOnGround())
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}
	// Project gravity onto the actual floor. Do not normalize: steeper slopes
	// should accelerate faster. The same force brakes uphill travel.
	const FVector Normal = CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
	const FVector SlopeGravity = FVector::VectorPlaneProject(FVector(0, 0, GetGravityZ()), Normal);
	FVector Planar(Velocity.X, Velocity.Y, 0);
	Planar += FVector(SlopeGravity.X, SlopeGravity.Y, 0) * SlideGravityScale * DeltaTime;
	const float Speed = FMath::Clamp(Planar.Size() - SlideDrag * DeltaTime, 0.0f, MaximumSlideSpeed);
	FVector Direction = Planar.GetSafeNormal();
	const FVector Desired = Acceleration.GetSafeNormal2D();
	if (!Desired.IsNearlyZero() && FVector::DotProduct(Direction, Desired) > 0.0f)
	{
		Direction = FMath::Lerp(Direction, Desired, FMath::Clamp(SlideSteering * DeltaTime, 0.0f, 1.0f)).GetSafeNormal();
	}
	Velocity = Direction * Speed;
	// No ordinary walk acceleration, crouch speed clamp, or second braking pass.
}
