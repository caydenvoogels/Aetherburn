// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AetherburnCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UAnimInstance;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS()
class AETHERBURN_API AAetherburnCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Third-person camera boom used by the 3D locomotion character. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> ThunderlordMeshAsset;

	UPROPERTY()
	TSubclassOf<UAnimInstance> ThunderlordAnimClass;

	/** Per-character mesh adjustment; set only on the Thunderlord Blueprint. */
	UPROPERTY(EditDefaultsOnly, Category="Thunderlord|Appearance")
	float ThunderlordMeshVerticalOffset = 0.0f;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	AAetherburnCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:
	UFUNCTION(BlueprintPure, Category="Movement")
	float GetStaminaFraction() const { return Stamina / MaximumStamina; }
	float GetSlideElapsed() const { return SlideElapsed; }
	/** Starts and stops the high-speed locomotion state. */
	UFUNCTION(BlueprintCallable, Category="Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category="Movement")
	void StopSprint();

	/** Crouches, or begins a slide when moving quickly on the ground. */
	UFUNCTION(BlueprintCallable, Category="Movement")
	void StartCrouchOrSlide();

	UFUNCTION(BlueprintCallable, Category="Movement")
	void StopCrouchOrSlide();
	void ToggleCrouch();

	UFUNCTION(BlueprintPure, Category="Movement")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category="Movement")
	bool IsSliding() const { return bIsSliding; }

	UFUNCTION(BlueprintPure, Category="Movement")
	float GetSlideCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category="Movement")
	float GetPlanarSpeed() const;
	/** Called by the animation instance when the one-shot slide clip finishes. */
	void FinishSlideFromAnimation();

	/** Normalized values intended for an Animation Blueprint or procedural camera rig. */
	UFUNCTION(BlueprintPure, Category="Movement|Animation")
	float GetCrouchAnimationAlpha() const { return CrouchAnimationAlpha; }

	UFUNCTION(BlueprintPure, Category="Movement|Animation")
	float GetSlideAnimationAlpha() const { return SlideAnimationAlpha; }

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Restores the ordinary grounded movement settings after a slide. */
	void EndSlide();

	void UpdateMovementState();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed", meta=(ClampMin="0"))
	float MovementWalkSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed", meta=(ClampMin="0"))
	float MovementSprintSpeed = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed", meta=(ClampMin="0"))
	float MovementCrouchSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0"))
	float MinimumSlideSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0"))
	float SlideImpulse = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0"))
	float SlideCooldown = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0"))
	float SlideDeceleration = 430.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0"))
	float SlideExitSpeed = 230.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Stamina", meta=(ClampMin="1"))
	float MaximumStamina = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Stamina", meta=(ClampMin="0"))
	float StaminaRecoveryPerSecond = 1.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Stamina", meta=(ClampMin="0"))
	float StaminaRecoveryDelay = 0.8f;
	float Stamina = 4.0f;
	float RecoveryTime = 0.0f;
	bool bStaminaExhausted = false;
	bool bCrouchToggled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0"))
	float DownhillSlideAcceleration = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0"))
	float UphillSlideBraking = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Slide", meta=(ClampMin="0", ClampMax="1"))
	float SlideSteeringStrength = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Animation", meta=(ClampMin="1"))
	float PostureBlendSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Animation")
	float CrouchCameraDrop = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Animation")
	float SlideCameraDrop = 44.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Animation")
	float SlideCameraForward = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Air", meta=(ClampMin="0", ClampMax="1"))
	float GroundAirControl = 0.55f;

	UPROPERTY(BlueprintReadOnly, Category="Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category="Movement")
	bool bIsSliding = false;

	bool bSprintHeld = false;
	bool bCrouchHeld = false;
	float SlideElapsed = 0.0f;
	float SavedGroundFriction = 8.0f;
	float SavedBrakingDeceleration = 2048.0f;
	float SavedMaxAcceleration = 2400.0f;
	float SlideDistance = 0.0f;
	float CurrentSlideSlopeDegrees = 0.0f;
	float LastSlideEndTime = -1000.0f;
	float CrouchAnimationAlpha = 0.0f;
	float SlideAnimationAlpha = 0.0f;
	FVector PreviousSlideLocation = FVector::ZeroVector;
	FVector BaseCameraRelativeLocation = FVector::ZeroVector;
	FVector BaseBodyMeshRelativeLocation = FVector::ZeroVector;
	FRotator BaseBodyMeshRelativeRotation = FRotator::ZeroRotator;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

