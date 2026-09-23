// Copyright Epic Games, Inc. All Rights Reserved.

#include "AetherburnCharacter.h"
#include "AetherburnMovementComponent.h"
#include "InputAction.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Aetherburn.h"
#include "UObject/ConstructorHelpers.h"

AAetherburnCharacter::AAetherburnCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAetherburnMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 420.0f;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 5.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 18.0f;

	// Create the Camera Component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	FirstPersonCameraComponent->FieldOfView = 80.0f;

	// configure the character comps
	FirstPersonMesh->SetHiddenInGame(true);
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	GetMesh()->SetRelativeLocation(FVector::ZeroVector);
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ThunderlordMesh(
		TEXT("/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig"));
	if (ThunderlordMesh.Succeeded())
	{
		ThunderlordMeshAsset = ThunderlordMesh.Object;
		GetMesh()->SetSkeletalMeshAsset(ThunderlordMeshAsset);
	}


	static ConstructorHelpers::FClassFinder<UAnimInstance> ThunderlordAnim(
		TEXT("/Game/Thunderlord/Zeus/AnimationMap/ABP_Thunderlord"));
	if (ThunderlordAnim.Succeeded())
	{
		ThunderlordAnimClass = ThunderlordAnim.Class;
		GetMesh()->SetAnimInstanceClass(ThunderlordAnimClass);
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(TEXT("/Game/Input/Actions/IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset(TEXT("/Game/Input/Actions/IA_Jump"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(TEXT("/Game/Input/Actions/IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseAsset(TEXT("/Game/Input/Actions/IA_MouseLook"));
	MoveAction = MoveAsset.Object;
	JumpAction = JumpAsset.Object;
	LookAction = LookAsset.Object;
	MouseLookAction = MouseAsset.Object;

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
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->SetCrouchedHalfHeight(52.0f);
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 620.0f, 0.0f);
}

void AAetherburnCharacter::BeginPlay()
{
	Super::BeginPlay();
	Stamina = MaximumStamina;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(52.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
	GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	// Apply the Thunderlord mesh and its authored Animation Blueprint at runtime
	// so stale template defaults cannot replace either asset.
	if (ThunderlordMeshAsset)
	{
		GetMesh()->SetSkeletalMeshAsset(ThunderlordMeshAsset);
	}
	if (ThunderlordAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(ThunderlordAnimClass);
	}
	FirstPersonMesh->SetHiddenInGame(true);
	FirstPersonMesh->SetVisibility(false, true);
	GetMesh()->SetHiddenInGame(false);
	GetMesh()->SetVisibility(true, true);
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->SetOnlyOwnerSee(false);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, ThunderlordMeshVerticalOffset));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	FirstPersonCameraComponent->AttachToComponent(
		CameraBoom,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		USpringArmComponent::SocketName);
	CameraBoom->TargetArmLength = 420.0f;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
	BaseCameraRelativeLocation = FirstPersonCameraComponent->GetRelativeLocation();
	BaseBodyMeshRelativeLocation = GetMesh()->GetRelativeLocation();
	BaseBodyMeshRelativeRotation = GetMesh()->GetRelativeRotation();
}

void AAetherburnCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsSliding)
	{
		SlideElapsed += DeltaSeconds;
		if (!GetCharacterMovement()->IsMovingOnGround() || GetPlanarSpeed() <= SlideExitSpeed) EndSlide();
	}
	if (bIsSprinting)
	{
		Stamina = FMath::Max(0.0f, Stamina - DeltaSeconds);
		RecoveryTime = StaminaRecoveryDelay;
		if (Stamina <= 0) bStaminaExhausted = true;
	}
	else
	{
		RecoveryTime = FMath::Max(0.0f, RecoveryTime - DeltaSeconds);
		if (RecoveryTime <= 0) Stamina = FMath::Min(MaximumStamina, Stamina + StaminaRecoveryPerSecond * DeltaSeconds);
		if (Stamina >= MaximumStamina * 0.3f) bStaminaExhausted = false;
	}
	UpdateMovementState();
	CrouchAnimationAlpha = FMath::FInterpTo(CrouchAnimationAlpha, bIsCrouched ? 1.0f : 0.0f, DeltaSeconds, PostureBlendSpeed);
	SlideAnimationAlpha = FMath::FInterpTo(SlideAnimationAlpha, bIsSliding ? 1.0f : 0.0f, DeltaSeconds, PostureBlendSpeed);
	FirstPersonCameraComponent->SetRelativeLocation(BaseCameraRelativeLocation - FVector(0, 0, 20 * CrouchAnimationAlpha + 15 * SlideAnimationAlpha));
	// ACharacter compensates the mesh when its capsule shrinks. Animation owns
	// the posture; manually lowering the mesh here would bury its feet.
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
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AAetherburnCharacter::ToggleCrouch);
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
		const FRotator ControlYaw(0.0f, GetController()->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(ControlYaw).GetUnitAxis(EAxis::Y), Right);
		AddMovementInput(FRotationMatrix(ControlYaw).GetUnitAxis(EAxis::X), Forward);
	}
}

void AAetherburnCharacter::DoJumpStart()
{
	bCrouchHeld = false;
	bCrouchToggled = false;
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

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bCooldownReady = CurrentTime - LastSlideEndTime >= SlideCooldown;
	const FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
	const bool bMovingForward = LocalVelocity.X > 10.0f;
	if (!bIsSliding && bCooldownReady && bIsSprinting && bMovingForward &&
		GetCharacterMovement()->IsMovingOnGround() && GetPlanarSpeed() >= MinimumSlideSpeed)
	{
		bIsSliding = true;
		bIsSprinting = false;
		SlideElapsed = 0.0f;
		SlideDistance = 0.0f;
		CurrentSlideSlopeDegrees = 0.0f;
		PreviousSlideLocation = GetActorLocation();
		SavedGroundFriction = GetCharacterMovement()->GroundFriction;
		SavedBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
		SavedMaxAcceleration = GetCharacterMovement()->MaxAcceleration;
		Crouch();

		FVector SlideDirection = GetVelocity().GetSafeNormal2D();
		if (SlideDirection.IsNearlyZero())
		{
			SlideDirection = GetActorForwardVector();
		}
		const float EntrySpeed = FMath::Clamp(
			GetPlanarSpeed() + SlideImpulse,
			MinimumSlideSpeed,
			MovementSprintSpeed + SlideImpulse);
		GetCharacterMovement()->Velocity.X = SlideDirection.X * EntrySpeed;
		GetCharacterMovement()->Velocity.Y = SlideDirection.Y * EntrySpeed;
	}
	else if (!bIsSliding)
	{
		Crouch();
	}
}

void AAetherburnCharacter::StopCrouchOrSlide()
{
	bCrouchHeld = false;
	// A slide is committed once it starts. Releasing crouch must not cancel it;
	// the speed, distance, duration, or loss of ground ends it instead.
	if (!bIsSliding && !bCrouchToggled)
	{
		UnCrouch();
	}
}

void AAetherburnCharacter::EndSlide()
{
	if (!bIsSliding)
	{
		return;
	}

	bIsSliding = false;
	LastSlideEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastSlideEndTime;
	SlideElapsed = 0.0f;
	SlideDistance = 0.0f;
	CurrentSlideSlopeDegrees = 0.0f;
	GetCharacterMovement()->GroundFriction = SavedGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = SavedBrakingDeceleration;
	GetCharacterMovement()->MaxAcceleration = SavedMaxAcceleration;

	if (!bCrouchHeld && !bCrouchToggled)
	{
		UnCrouch();
	}
}

void AAetherburnCharacter::FinishSlideFromAnimation()
{
	EndSlide();
}

float AAetherburnCharacter::GetSlideCooldownRemaining() const
{
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return FMath::Max(0.0f, SlideCooldown - (CurrentTime - LastSlideEndTime));
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
	bIsSprinting = bSprintHeld && !bIsSliding && !bIsCrouched && !bStaminaExhausted && Stamina > 0.0f && GetPlanarSpeed() > 10.0f && Movement->IsMovingOnGround();

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

void AAetherburnCharacter::ToggleCrouch()
{
	if (bIsSliding) return;
	if (bIsCrouched || bCrouchToggled)
	{
		bCrouchToggled = false;
		if (!bCrouchHeld) UnCrouch();
		return;
	}
	const bool WasHeld = bCrouchHeld;
	StartCrouchOrSlide();
	bCrouchHeld = WasHeld;
	bCrouchToggled = !bIsSliding;
}
