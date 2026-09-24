// Copyright Epic Games, Inc. All Rights Reserved.

#include "AetherburnCharacter.h"
#include "AetherburnMovementComponent.h"
#include "InputAction.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "ThunderlordBoltProjectile.h"
#include "ThunderlordAnimInstance.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DamageEvents.h"
#include "ThunderlordHUD.h"
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

	HeadshotCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Headshot Collider"));
	HeadshotCollider->SetupAttachment(GetMesh());
	HeadshotCollider->InitCapsuleSize(HeadshotColliderRadius, HeadshotColliderHalfHeight);
	HeadshotCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadshotCollider->SetGenerateOverlapEvents(false);

	ThunderlordBolt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Zeus Bolt"));
	ThunderlordBolt->SetupAttachment(GetMesh());
	ThunderlordBolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ThunderlordBolt->SetGenerateOverlapEvents(false);
	ThunderlordBolt->SetRelativeScale3D(FVector::OneVector);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ZeusBoltMesh(
		TEXT("/Game/Thunderlord/Zeus/Weapons/SM_ZeusBolt.SM_ZeusBolt"));
	if (ZeusBoltMesh.Succeeded())
	{
		ThunderlordBolt->SetStaticMesh(ZeusBoltMesh.Object);
	}
	ThunderlordBoltArcs = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Zeus Bolt Arcs"));
	ThunderlordBoltArcs->SetupAttachment(ThunderlordBolt);
	ThunderlordBoltArcs->SetAutoActivate(false);
	ThunderlordBoltArcs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ThunderlordBoltArcs->SetGenerateOverlapEvents(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ZeusBoltArcs(
		TEXT("/Game/Thunderlord/Zeus/VFX/NS_ZeusBolt_ArcWrap.NS_ZeusBolt_ArcWrap"));
	if (ZeusBoltArcs.Succeeded())
	{
		ThunderlordBoltArcSystem = ZeusBoltArcs.Object;
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BoltImpactSparks(
		TEXT("/Game/NiagaraExamples/FX_Sparks/NS_Spark_Burst.NS_Spark_Burst"));
	if (BoltImpactSparks.Succeeded()) ThunderlordBoltImpactSparks = BoltImpactSparks.Object;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BoltImpactElectricity(
		TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Electricity_Looping.NS_Player_Electricity_Looping"));
	if (BoltImpactElectricity.Succeeded()) ThunderlordBoltImpactElectricity = BoltImpactElectricity.Object;
	ThunderlordBoltProjectileClass = AThunderlordBoltProjectile::StaticClass();

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	// Default to a shoulder camera so the throw pose is visible while testing.
	CameraBoom->TargetArmLength = 360.0f;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 0.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
	// Keep the animation test view at a fixed distance instead of snapping in
	// and out when the spring-arm probe crosses nearby level geometry.
	CameraBoom->bDoCollisionTest = false;

	// Create the Camera Component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	FirstPersonCameraComponent->FieldOfView = 80.0f;
	FirstPersonCameraComponent->SetAutoActivate(false);
	FirstPersonCameraComponent->ComponentTags.AddUnique(FName(TEXT("First Person Camera")));
	FirstPersonCameraComponent->ComponentTags.AddUnique(FName(TEXT("FirstPersonCamera")));

	// The Hyper Extended Movement Blueprint expects a distinct camera with this
	// role. Keep it separate so its crouch timeline never dereferences a missing view.
	ThirdPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Third Person Camera"));
	ThirdPersonCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ThirdPersonCameraComponent->bUsePawnControlRotation = false;
	ThirdPersonCameraComponent->FieldOfView = 80.0f;
	ThirdPersonCameraComponent->SetAutoActivate(false);
	ThirdPersonCameraComponent->ComponentTags.AddUnique(FName(TEXT("Third Person Camera")));
	ThirdPersonCameraComponent->ComponentTags.AddUnique(FName(TEXT("ThirdPersonCamera")));

	// configure the character comps
	FirstPersonMesh->SetHiddenInGame(true);
	GetMesh()->SetOwnerNoSee(true);
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
	Health = MaximumHealth;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(52.0f);
	if (bIsDamageDummy)
	{
		GetCharacterMovement()->DisableMovement();
	}
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
	// Anchor a compact projectile capsule to the actual animated head bone. This
	// component is the only source of headshot classification, so Physics Asset
	// body hits can never accidentally receive the headshot multiplier.
	HeadshotBoneName = NAME_None;
	TArray<FName> CharacterBoneNames;
	GetMesh()->GetBoneNames(CharacterBoneNames);
	for (const FName BoneName : CharacterBoneNames)
	{
		const FString BoneLabel = BoneName.ToString();
		if (BoneLabel.EndsWith(TEXT("head"), ESearchCase::IgnoreCase))
		{
			HeadshotBoneName = BoneName;
			break;
		}
	}
	if (HeadshotCollider && !HeadshotBoneName.IsNone())
	{
		HeadshotCollider->AttachToComponent(GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, HeadshotBoneName);
		HeadshotCollider->SetRelativeLocation(HeadshotColliderOffset);
		HeadshotCollider->SetRelativeRotation(FRotator::ZeroRotator);
		HeadshotCollider->SetCapsuleSize(HeadshotColliderRadius, HeadshotColliderHalfHeight);
		HeadshotCollider->SetCollisionObjectType(ECC_WorldDynamic);
		HeadshotCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
		HeadshotCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		HeadshotCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		UE_LOG(LogAetherburn, Warning, TEXT("%s has no head bone; dedicated headshot collider is disabled"), *GetName());
	}
	// Keep the capsule for character movement, but route Zeus projectile queries
	// through the skeletal mesh's Physics Asset so hits follow the body shape.
	if (GetMesh()->GetPhysicsAsset())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	}
	else
	{
		UE_LOG(LogAetherburn, Warning, TEXT("%s has no Physics Asset; keeping the capsule as projectile fallback"), *GetName());
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	}
	FirstPersonMesh->SetHiddenInGame(true);
	FirstPersonMesh->SetVisibility(false, true);
	GetMesh()->SetHiddenInGame(false);
	GetMesh()->SetVisibility(true, true);
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->SetOnlyOwnerSee(false);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, ThunderlordMeshVerticalOffset));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	if (ThunderlordBolt && ThunderlordBolt->GetStaticMesh())
	{
		const FName RightHandBoneCandidates[] = {
			FName(TEXT("mixamorig:RightHand")),
			FName(TEXT("RightHand")),
			FName(TEXT("right_hand")),
			FName(TEXT("hand_r")),
			FName(TEXT("righthand"))
		};
		bool bAttachedToHand = false;
		FName AttachedBone = NAME_None;
		for (const FName BoneName : RightHandBoneCandidates)
		{
			if (GetMesh()->GetBoneIndex(BoneName) != INDEX_NONE)
			{
				AttachedBone = BoneName;
				break;
			}
		}
		if (AttachedBone.IsNone())
		{
			TArray<FName> BoneNames;
			GetMesh()->GetBoneNames(BoneNames);
			for (const FName BoneName : BoneNames)
			{
				FString NormalizedName = BoneName.ToString().ToLower();
				NormalizedName.ReplaceInline(TEXT(":"), TEXT(""));
				NormalizedName.ReplaceInline(TEXT("_"), TEXT(""));
				if (NormalizedName == TEXT("righthand") || NormalizedName == TEXT("handr"))
				{
					AttachedBone = BoneName;
					break;
				}
			}
		}
		if (!AttachedBone.IsNone())
		{
			ThunderlordBoltHandBone = AttachedBone;
			ThunderlordBolt->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				AttachedBone);
			ThunderlordBolt->SetRelativeScale3D(FVector::OneVector);
			bAttachedToHand = true;

			const FReferenceSkeleton& ReferenceSkeleton = GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
			const TArray<FMeshBoneInfo>& BoneInfo = ReferenceSkeleton.GetRefBoneInfo();
			const TArray<FTransform>& ReferencePose = ReferenceSkeleton.GetRefBonePose();
			const int32 HandBoneIndex = GetMesh()->GetBoneIndex(AttachedBone);
			FVector FingerBaseOffset = FVector::ZeroVector;
			FVector CentralFingerOffset = FVector::ZeroVector;
			int32 FingerCount = 0;
			int32 CentralFingerCount = 0;
			for (int32 BoneIndex = 0; BoneIndex < BoneInfo.Num(); ++BoneIndex)
			{
				if (BoneInfo[BoneIndex].ParentIndex != HandBoneIndex)
				{
					continue;
				}

				FString ChildBoneName = BoneInfo[BoneIndex].Name.ToString().ToLower();
				ChildBoneName.ReplaceInline(TEXT(":"), TEXT(""));
				ChildBoneName.ReplaceInline(TEXT("_"), TEXT(""));
				if (ChildBoneName.Contains(TEXT("thumb")))
				{
					continue;
				}

				const FVector ChildOffset = ReferencePose[BoneIndex].GetLocation();
				if (ChildOffset.IsNearlyZero())
				{
					continue;
				}

				FingerBaseOffset += ChildOffset;
				++FingerCount;
				if (ChildBoneName.Contains(TEXT("index1")) || ChildBoneName.Contains(TEXT("middle1")))
				{
					CentralFingerOffset += ChildOffset;
					++CentralFingerCount;
				}
			}

			if (CentralFingerCount > 0)
			{
				FingerBaseOffset = CentralFingerOffset / CentralFingerCount;
			}
			else if (FingerCount > 0)
			{
				FingerBaseOffset /= FingerCount;
			}

			// The hand bone begins at the wrist; shift toward the finger roots so
			// the center of the bolt sits in the palm, then out by half the hand
			// thickness. The reference pose places the finger roots on local -Y,
			// so local +Z is used as the palm-facing side.
			constexpr float ZeusBoltHalfHandThickness = 2.5f;
			// The bolt's long axis is local X, so roll it around that axis without
			// tipping its length away from the top-to-bottom grip direction.
			ThunderlordBolt->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
			ThunderlordBolt->SetRelativeLocation(
				FingerBaseOffset * 0.65f + FVector(0.0f, 0.0f, ZeusBoltHalfHandThickness));
			if (ThunderlordBoltArcs && ThunderlordBoltArcSystem)
			{
				const FBoxSphereBounds BoltBounds = ThunderlordBolt->GetStaticMesh()->GetBounds();
				const FVector ArcEndOffset(BoltBounds.BoxExtent.X, 0.0f, 0.0f);
				const FVector ArcStartLocal = BoltBounds.Origin - ArcEndOffset;
				const FVector ArcEndLocal = BoltBounds.Origin + ArcEndOffset;
				ThunderlordBoltArcs->SetAsset(ThunderlordBoltArcSystem);
				// Start the Niagara system at the forward tip and target the rear tip.
				// This reverses the bolt's arc direction while keeping both ends on the mesh.
				ThunderlordBoltArcs->SetRelativeLocation(ArcEndLocal);
				ThunderlordBoltArcs->SetRelativeRotation(FRotator::ZeroRotator);
				ThunderlordBoltArcs->SetRelativeScale3D(ThunderlordBoltArcScale);
				ThunderlordBoltArcs->SetVariableLinearColor(
					FName(TEXT("User.Smoke Color")), FLinearColor(0.18f, 0.72f, 1.0f, 0.8f));
				ThunderlordBoltArcs->SetVariablePosition(
					FName(TEXT("User.PositionTarget")),
					ThunderlordBolt->GetComponentTransform().TransformPosition(ArcStartLocal));
				ThunderlordBoltArcs->Activate(true);
			}
			UE_LOG(LogAetherburn, Log, TEXT("Zeus bolt attached to %s with hand offset %s and grip rotation %s"), *AttachedBone.ToString(), *ThunderlordBolt->GetRelativeLocation().ToCompactString(), *ThunderlordBolt->GetRelativeRotation().ToCompactString());
		}
		if (!bAttachedToHand)
		{
			UE_LOG(LogAetherburn, Warning, TEXT("Zeus bolt was not attached: no recognized right-hand bone on %s"), *GetNameSafe(GetMesh()->GetSkeletalMeshAsset()));
			ThunderlordBolt->SetHiddenInGame(true);
			if (ThunderlordBoltArcs)
			{
				ThunderlordBoltArcs->SetHiddenInGame(true, true);
			}
		}
	}
	FirstPersonCameraComponent->AttachToComponent(
		CameraBoom,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		USpringArmComponent::SocketName);
	ThirdPersonCameraComponent->AttachToComponent(
		CameraBoom,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		USpringArmComponent::SocketName);
	CameraBoom->TargetArmLength = 360.0f;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
	CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 0.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
	CameraBoom->bDoCollisionTest = false;
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	FirstPersonCameraComponent->Deactivate();
	ThirdPersonCameraComponent->Activate(true);
	bIsThirdPersonCamera = true;
	UE_LOG(LogAetherburn, Log, TEXT("Thunderlord test camera initialized: third-person arm=%.0f control-rotation=%d collision=%d"),
		CameraBoom->TargetArmLength, CameraBoom->bUsePawnControlRotation, CameraBoom->bDoCollisionTest);
	BaseCapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	BaseBodyMeshRelativeLocation = GetMesh()->GetRelativeLocation();
	BaseBodyMeshRelativeRotation = GetMesh()->GetRelativeRotation();
}

void AAetherburnCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ThunderlordBoltArcs && ThunderlordBoltArcs->IsActive() && ThunderlordBolt)
	{
		if (UStaticMesh* BoltMesh = ThunderlordBolt->GetStaticMesh())
		{
			const FBoxSphereBounds BoltBounds = BoltMesh->GetBounds();
			const FVector ArcStartLocal = BoltBounds.Origin - FVector(BoltBounds.BoxExtent.X, 0.0f, 0.0f);
			ThunderlordBoltArcs->SetVariablePosition(
				FName(TEXT("User.PositionTarget")),
				ThunderlordBolt->GetComponentTransform().TransformPosition(ArcStartLocal));
		}
	}

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
	const float CrouchBlendSpeed = bIsCrouched ? PostureBlendSpeed : PostureRecoverySpeed;
	const float SlideBlendSpeed = bIsSliding ? PostureBlendSpeed : PostureRecoverySpeed;
	CrouchAnimationAlpha = FMath::FInterpTo(CrouchAnimationAlpha, bIsCrouched ? 1.0f : 0.0f, DeltaSeconds, CrouchBlendSpeed);
	SlideAnimationAlpha = FMath::FInterpTo(SlideAnimationAlpha, bIsSliding ? 1.0f : 0.0f, DeltaSeconds, SlideBlendSpeed);
	if (!bIsThirdPersonCamera && CameraBoom)
	{
		CameraBoom->SetRelativeLocation(CalculateFirstPersonCameraBoomLocation());
	}
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
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AAetherburnCharacter::ThrowThunderlordBolt);
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &AAetherburnCharacter::ToggleCameraView);
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

bool AAetherburnCharacter::IsHeadshotHitComponent(const UPrimitiveComponent* Component) const
{
	return HeadshotCollider && Component == HeadshotCollider;
}

float AAetherburnCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float BaseDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (BaseDamage <= 0.0f || (bIsDead && !bIsDamageDummy))
	{
		return 0.0f;
	}

	const FPointDamageEvent* PointDamage = DamageEvent.IsOfType(FPointDamageEvent::ClassID)
		? static_cast<const FPointDamageEvent*>(&DamageEvent) : nullptr;
	const bool bHeadshot = PointDamage &&
		PointDamage->HitInfo.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase);
	const bool bThunderlordHeadshot = bHeadshot && Cast<AThunderlordBoltProjectile>(DamageCauser);
	const float AppliedDamage = bThunderlordHeadshot
		? BaseDamage * ThunderlordHeadshotDamageMultiplier
		: BaseDamage;
	if (!bIsDead)
	{
		Health = FMath::Clamp(Health - AppliedDamage, 0.0f, MaximumHealth);
	}
	if (bIsDamageDummy)
	{
		if (AThunderlordHUD* ThunderlordHUD = Cast<AThunderlordHUD>(
			GetWorld() && GetWorld()->GetFirstPlayerController()
				? GetWorld()->GetFirstPlayerController()->GetHUD() : nullptr))
		{
			const FVector HitLocation = PointDamage ? FVector(PointDamage->HitInfo.ImpactPoint) : GetActorLocation();
			ThunderlordHUD->ReportDummyDamage(AppliedDamage, HitLocation, bHeadshot);
		}
		if (Health <= 0.0f)
		{
			bIsDead = true;
		}
		return AppliedDamage;
	}
	if (Health <= 0.0f)
	{
		bIsDead = true;
		bIsSprinting = false;
		bSprintHeld = false;
		bIsThrowingBolt = false;
		GetWorldTimerManager().ClearTimer(BoltThrowReleaseTimer);
		if (ThunderlordBoltArcs)
		{
			ThunderlordBoltArcs->Deactivate();
		}
		StopJumping();
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (HeadshotCollider)
		{
			HeadshotCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		DisableInput(nullptr);
		GetMesh()->SetCollisionProfileName(FName(TEXT("Ragdoll")));
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();

		if (AController* PlayerController = GetController(); PlayerController && PlayerController->IsPlayerController())
		{
			RespawnController = PlayerController;
			UE_LOG(LogAetherburn, Log, TEXT("%s died; ragdolling and respawning in %.1f seconds"),
				*GetName(), RespawnDelay);
			GetWorldTimerManager().SetTimer(RespawnTimer, this,
				&AAetherburnCharacter::RespawnPlayer, RespawnDelay, false);
		}
	}
	return AppliedDamage;
}

void AAetherburnCharacter::RespawnPlayer()
{
	AController* PlayerController = RespawnController.Get();
	if (PlayerController && GetWorld())
	{
		PlayerController->UnPossess();
		if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
		{
			GameMode->RestartPlayer(PlayerController);
		}
	}
	Destroy();
}

FVector AAetherburnCharacter::CalculateFirstPersonCameraBoomLocation() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float CapsuleHeightReduction = Capsule
		? FMath::Max(0.0f, BaseCapsuleHalfHeight - Capsule->GetScaledCapsuleHalfHeight())
		: 0.0f;
	const float PostureDrop = FMath::Max(
		CrouchCameraDrop * CrouchAnimationAlpha,
		SlideCameraDrop * SlideAnimationAlpha);
	const float PostureForward = FMath::Max(
		CrouchCameraForward * CrouchAnimationAlpha,
		SlideCameraForward * SlideAnimationAlpha);
	const float CapsuleFront = Capsule ? Capsule->GetScaledCapsuleRadius() : 34.0f;

	// Put the view in front of the capsule and keep the character mesh visible.
	return FVector(CapsuleFront + FirstPersonCameraForwardClearance + PostureForward,
		0.0f, 82.0f + CapsuleHeightReduction - PostureDrop);
}

void AAetherburnCharacter::ThrowThunderlordBolt()
{
	if (bIsThrowingBolt || !bBoltInHand || !ThunderlordBolt || !ThunderlordBoltProjectileClass || bIsSliding)
	{
		UE_LOG(LogAetherburn, Warning, TEXT("Bolt attack rejected: throwing=%d inHand=%d prop=%s projectileClass=%s sliding=%d crouched=%d"),
			bIsThrowingBolt, bBoltInHand, *GetNameSafe(ThunderlordBolt),
			*GetNameSafe(ThunderlordBoltProjectileClass.Get()), bIsSliding, bIsCrouched);
		return;
	}
	bIsThrowingBolt = true;
	UpdateMovementState();
	UThunderlordAnimInstance* AnimInstance = Cast<UThunderlordAnimInstance>(GetMesh()->GetAnimInstance());
	const float ReleaseDelay = AnimInstance ? AnimInstance->PlayBoltThrowAnimation() : 0.0f;
	UE_LOG(LogAetherburn, Log, TEXT("Bolt attack accepted: animation=%s releaseDelay=%.2f"),
		AnimInstance ? TEXT("Thunderlord") : TEXT("missing"), ReleaseDelay);
	if (ReleaseDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(BoltThrowReleaseTimer, this,
			&AAetherburnCharacter::ReleaseThunderlordBolt, ReleaseDelay, false);
	}
	else
	{
		// Keep the player state valid if the throw animation has not been imported yet.
		ReleaseThunderlordBolt();
	}
}

void AAetherburnCharacter::ReleaseThunderlordBolt()
{
	if (!bIsThrowingBolt || !bBoltInHand || !GetWorld()) return;
	bIsThrowingBolt = false;
	bBoltInHand = false;
	UpdateMovementState();
	if (ThunderlordBoltArcs) ThunderlordBoltArcs->Deactivate();
	ThunderlordBolt->SetHiddenInGame(true, true);

	const UCameraComponent* ViewCamera = bIsThirdPersonCamera && ThirdPersonCameraComponent
		? ThirdPersonCameraComponent : FirstPersonCameraComponent;
	FVector ViewLocation = ViewCamera ? ViewCamera->GetComponentLocation() : GetActorLocation();
	FRotator ViewRotation = ViewCamera ? ViewCamera->GetComponentRotation() : GetActorRotation();
	if (GetController())
	{
		// Use the controller's actual aim rotation. A camera manager can still
		// report a stale POV while the spring arm is changing views.
		ViewRotation = GetController()->GetControlRotation();
	}
	FVector AimRayOrigin = ViewLocation;
	FVector AimRayDirection = ViewRotation.Vector();
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		int32 ViewportWidth = 0;
		int32 ViewportHeight = 0;
		PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
		FVector DeprojectedOrigin;
		FVector DeprojectedDirection;
		if (ViewportWidth > 0 && ViewportHeight > 0 && PlayerController->DeprojectScreenPositionToWorld(
			ViewportWidth * 0.5f, ViewportHeight * 0.5f, DeprojectedOrigin, DeprojectedDirection))
		{
			AimRayOrigin = DeprojectedOrigin;
			AimRayDirection = DeprojectedDirection.GetSafeNormal();
		}
	}
	FVector AimTarget = AimRayOrigin + AimRayDirection * 20000.0f;
	FCollisionQueryParams AimQueryParams(SCENE_QUERY_STAT(ThunderlordBoltAim), true, this);
	FHitResult AimHit;
	if (GetWorld()->LineTraceSingleByChannel(AimHit, AimRayOrigin, AimTarget, ECC_GameTraceChannel1, AimQueryParams))
	{
		AimTarget = AimHit.ImpactPoint;
	}
	FVector SpawnLocation = ThunderlordBolt->GetComponentLocation();
	if (GetMesh() && !ThunderlordBoltHandBone.IsNone())
	{
		const FTransform HandWorld = GetMesh()->GetSocketTransform(ThunderlordBoltHandBone, RTS_World);
		const FTransform BoltLocal = ThunderlordBolt->GetRelativeTransform();
		SpawnLocation = HandWorld.TransformPosition(BoltLocal.GetLocation());
	}
	FVector AimDirection = (AimTarget - SpawnLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = ViewRotation.Vector().GetSafeNormal();
	}
	// Move the projectile clear of the hand capsule so its first swept move
	// cannot start embedded in the player or ground.
	SpawnLocation += AimDirection * 28.0f;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	// Preserve the hand-based spawn point. Collision adjustment can otherwise
	// relocate a spawn that overlaps the player's capsule toward the ground.
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AThunderlordBoltProjectile* Projectile = GetWorld()->SpawnActor<AThunderlordBoltProjectile>(
		ThunderlordBoltProjectileClass, SpawnLocation, AimDirection.Rotation(), SpawnParameters))
	{
		Projectile->Launch(AimDirection, 3000.0f);
		UE_LOG(LogAetherburn, Log, TEXT("Bolt projectile launched from hand %s along %s (pitch %.1f) toward view target %s"),
			*SpawnLocation.ToCompactString(), *AimDirection.ToCompactString(), ViewRotation.Pitch, *AimTarget.ToCompactString());
	}
	else
	{
		UE_LOG(LogAetherburn, Error, TEXT("Bolt projectile spawn failed at %s"), *SpawnLocation.ToCompactString());
		bBoltInHand = true;
		ThunderlordBolt->SetHiddenInGame(false, true);
		if (ThunderlordBoltArcs) ThunderlordBoltArcs->Activate(true);
	}
}

void AAetherburnCharacter::OnThunderlordBoltImpact()
{
	bBoltInHand = true;
	if (ThunderlordBolt)
	{
		// The prop stays attached to its original hand bone while hidden in flight.
		// Revealing it avoids reattaching with a stale transform at the character's feet.
		ThunderlordBolt->SetHiddenInGame(false, true);
	}
	if (ThunderlordBoltArcs) ThunderlordBoltArcs->Activate(true);
	UE_LOG(LogAetherburn, Log, TEXT("Bolt reclaimed to hand component at %s; ready to throw"),
		ThunderlordBolt ? *ThunderlordBolt->GetComponentLocation().ToCompactString() : TEXT("missing"));
}

void AAetherburnCharacter::SpawnThunderlordBoltImpact(const FVector& Location, const FVector& Normal)
{
	if (!GetWorld()) return;
	const FRotator ImpactRotation = Normal.ToOrientationRotator();
	if (ThunderlordBoltImpactSparks)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ThunderlordBoltImpactSparks, Location, ImpactRotation);
	}
	if (ThunderlordBoltImpactElectricity)
	{
		UNiagaraComponent* Electricity = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, ThunderlordBoltImpactElectricity, Location, ImpactRotation, FVector(0.7f), false, true, ENCPoolMethod::None, true);
		if (Electricity)
		{
			FTimerHandle EffectTimer;
			GetWorldTimerManager().SetTimer(EffectTimer, [Electricity]()
			{
				if (IsValid(Electricity)) Electricity->DestroyComponent();
			}, 0.65f, false);
		}
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
	bIsSprinting = bSprintHeld && !bIsSliding && !bIsCrouched && !bIsThrowingBolt && !bStaminaExhausted && Stamina > 0.0f && GetPlanarSpeed() > 10.0f && Movement->IsMovingOnGround();

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
		Movement->MaxWalkSpeed = bIsThrowingBolt ? MovementWalkSpeed : (bIsSprinting ? MovementSprintSpeed : MovementWalkSpeed);
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

void AAetherburnCharacter::ToggleCameraView()
{
	if (!CameraBoom) return;
	bIsThirdPersonCamera = !bIsThirdPersonCamera;
	const bool bEnableThirdPerson = bIsThirdPersonCamera;
	if (bEnableThirdPerson)
	{
		CameraBoom->AttachToComponent(
			GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		CameraBoom->TargetArmLength = 360.0f;
		CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
		CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 0.0f);
		FirstPersonCameraComponent->Deactivate();
		ThirdPersonCameraComponent->Activate(true);
	}
	else
	{
		CameraBoom->AttachToComponent(
			GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		CameraBoom->TargetArmLength = 0.0f;
		CameraBoom->SocketOffset = FVector::ZeroVector;
		CameraBoom->SetRelativeLocation(CalculateFirstPersonCameraBoomLocation());
		ThirdPersonCameraComponent->Deactivate();
		FirstPersonCameraComponent->Activate(true);
	}
	CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;
	// Keep the full character mesh visible in first person. Position the camera forward
	// of the face instead of hiding head bones, so the head remains visible and casts shadows.
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->SetOnlyOwnerSee(false);
	UE_LOG(LogAetherburn, Log, TEXT("Camera view switched to %s (arm=%.0f)"),
		bEnableThirdPerson ? TEXT("third-person") : TEXT("first-person"), CameraBoom->TargetArmLength);
}
