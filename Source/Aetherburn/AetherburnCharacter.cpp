// Copyright Epic Games, Inc. All Rights Reserved.

#include "AetherburnCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Aetherburn.h"

AAetherburnCharacter::AAetherburnCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = GroundAirControl;
	GetCharacterMovement()->MaxWalkSpeed = MovementWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = MovementCrouchSpeed;
	GetCharacterMovement()->JumpZVelocity = 560.0f;
	GetCharacterMovement()->GravityScale = 1.65f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1800.0f;
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->MaxAcceleration = 2400.0f;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

void AAetherburnCharacter::BeginPlay()
{
	Super::BeginPlay();
	BaseCameraRelativeLocation = FirstPersonCameraComponent->GetRelativeLocation();
}

void AAetherburnCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsSliding)
	{
		SlideElapsed += DeltaSeconds;
		SlideDistance += FVector::Dist2D(GetActorLocation(), PreviousSlideLocation);
		PreviousSlideLocation = GetActorLocation();

		UCharacterMovementComponent* Movement = GetCharacterMovement();
		const FVector FloorNormal = Movement->CurrentFloor.IsWalkableFloor()
			? Movement->CurrentFloor.HitResult.ImpactNormal.GetSafeNormal()
			: FVector::UpVector;
		const FVector DownSlope = FVector::VectorPlaneProject(FVector(0.0f, 0.0f, -1.0f), FloorNormal).GetSafeNormal();
		const FVector TravelDirection = GetVelocity().GetSafeNormal2D();
		const float SlopeAlignment = FVector::DotProduct(TravelDirection, DownSlope);
		CurrentSlideSlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FloorNormal.Z, -1.0f, 1.0f))) * FMath::Sign(SlopeAlignment);

		if (!DownSlope.IsNearlyZero())
		{
			if (SlopeAlignment > 0.0f)
			{
				Movement->Velocity += DownSlope * DownhillSlideAcceleration * SlopeAlignment * DeltaSeconds;
			}
			else if (SlopeAlignment < 0.0f)
			{
				Movement->Velocity += TravelDirection * UphillSlideBraking * SlopeAlignment * DeltaSeconds;
			}
		}

		// Permit limited steering without allowing an instant 180-degree turn.
		const FVector DesiredDirection = GetLastMovementInputVector().GetSafeNormal2D();
		if (!DesiredDirection.IsNearlyZero() && !TravelDirection.IsNearlyZero())
		{
			const float Speed = GetPlanarSpeed();
			const FVector SteeredDirection = FMath::Lerp(TravelDirection, DesiredDirection, SlideSteeringStrength * DeltaSeconds * 6.0f).GetSafeNormal();
			Movement->Velocity.X = SteeredDirection.X * Speed;
			Movement->Velocity.Y = SteeredDirection.Y * Speed;
		}

		const float Speed = GetPlanarSpeed();
		if (!GetCharacterMovement()->IsMovingOnGround() ||
			SlideElapsed >= MaximumSlideDuration ||
			SlideDistance >= MaximumSlideDistance ||
			Speed < MovementCrouchSpeed + 40.0f)
		{
			EndSlide();
		}
	}

	UpdateMovementState();

	const float CrouchTarget = (bIsCrouched && !bIsSliding) ? 1.0f : 0.0f;
	const float SlideTarget = bIsSliding ? 1.0f : 0.0f;
	CrouchAnimationAlpha = FMath::FInterpTo(CrouchAnimationAlpha, CrouchTarget, DeltaSeconds, PostureBlendSpeed);
	SlideAnimationAlpha = FMath::FInterpTo(SlideAnimationAlpha, SlideTarget, DeltaSeconds, PostureBlendSpeed);
	const FVector TargetCameraOffset(
		SlideCameraForward * SlideAnimationAlpha,
		0.0f,
		-(CrouchCameraDrop * CrouchAnimationAlpha + SlideCameraDrop * SlideAnimationAlpha));
	FirstPersonCameraComponent->SetRelativeLocation(BaseCameraRelativeLocation + TargetCameraOffset);
}

void AAetherburnCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAetherburnCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAetherburnCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAetherburnCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAetherburnCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AAetherburnCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogAetherburn, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	// Direct key bindings keep the movement system functional without requiring
	// project-specific Input Action assets. They can later be replaced or
	// supplemented by Enhanced Input actions for remapping/gamepad support.
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AAetherburnCharacter::StartSprint);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AAetherburnCharacter::StopSprint);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &AAetherburnCharacter::StartCrouchOrSlide);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Released, this, &AAetherburnCharacter::StopCrouchOrSlide);
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AAetherburnCharacter::StartCrouchOrSlide);
	PlayerInputComponent->BindKey(EKeys::C, IE_Released, this, &AAetherburnCharacter::StopCrouchOrSlide);
}


void AAetherburnCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AAetherburnCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AAetherburnCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AAetherburnCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AAetherburnCharacter::DoJumpStart()
{
	if (bIsSliding)
	{
		EndSlide();
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	// pass Jump to the character
	Jump();
}

void AAetherburnCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AAetherburnCharacter::StartSprint()
{
	bSprintHeld = true;
	UpdateMovementState();
}

void AAetherburnCharacter::StopSprint()
{
	bSprintHeld = false;
	bIsSprinting = false;
	UpdateMovementState();
}

void AAetherburnCharacter::StartCrouchOrSlide()
{
	bCrouchHeld = true;

	if (!bIsSliding && GetCharacterMovement()->IsMovingOnGround() && GetPlanarSpeed() >= MinimumSlideSpeed)
	{
		bIsSliding = true;
		bIsSprinting = false;
		SlideElapsed = 0.0f;
		SlideDistance = 0.0f;
		CurrentSlideSlopeDegrees = 0.0f;
		PreviousSlideLocation = GetActorLocation();
		SavedGroundFriction = GetCharacterMovement()->GroundFriction;
		SavedBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
		GetCharacterMovement()->GroundFriction = 0.75f;
		GetCharacterMovement()->BrakingDecelerationWalking = 280.0f;
		Crouch();

		FVector SlideDirection = GetVelocity().GetSafeNormal2D();
		if (SlideDirection.IsNearlyZero())
		{
			SlideDirection = GetActorForwardVector();
		}
		GetCharacterMovement()->AddImpulse(SlideDirection * SlideImpulse, true);
	}
	else if (!bIsSliding)
	{
		Crouch();
	}
}

void AAetherburnCharacter::StopCrouchOrSlide()
{
	bCrouchHeld = false;
	if (bIsSliding)
	{
		EndSlide();
	}
	UnCrouch();
}

void AAetherburnCharacter::EndSlide()
{
	if (!bIsSliding)
	{
		return;
	}

	bIsSliding = false;
	SlideElapsed = 0.0f;
	SlideDistance = 0.0f;
	CurrentSlideSlopeDegrees = 0.0f;
	GetCharacterMovement()->GroundFriction = SavedGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = SavedBrakingDeceleration;

	if (!bCrouchHeld)
	{
		UnCrouch();
	}
}

void AAetherburnCharacter::UpdateMovementState()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	const FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
	const bool bMovingForward = LocalVelocity.X > 10.0f;
	bIsSprinting = bSprintHeld && !bIsSliding && !bIsCrouched && bMovingForward && Movement->IsMovingOnGround();

	if (bIsSliding)
	{
		Movement->MaxWalkSpeed = FMath::Max(MovementSprintSpeed, GetPlanarSpeed());
	}
	else if (bIsCrouched)
	{
		Movement->MaxWalkSpeed = MovementCrouchSpeed;
	}
	else
	{
		Movement->MaxWalkSpeed = bIsSprinting ? MovementSprintSpeed : MovementWalkSpeed;
	}

	Movement->MaxWalkSpeedCrouched = MovementCrouchSpeed;
	Movement->AirControl = GroundAirControl;
}

float AAetherburnCharacter::GetPlanarSpeed() const
{
	return GetVelocity().Size2D();
}
