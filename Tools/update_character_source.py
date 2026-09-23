"""One-time migration of the original template movement to the native solver."""
from pathlib import Path
p = Path(__file__).resolve().parents[1] / 'Source/Aetherburn/AetherburnCharacter.cpp'
s = p.read_text()
s = s.replace('#include "AetherburnCharacter.h"', '#include "AetherburnCharacter.h"\n#include "AetherburnMovementComponent.h"\n#include "ThunderlordAnimInstance.h"\n#include "InputAction.h"')
s = s.replace('AAetherburnCharacter::AAetherburnCharacter()', 'AAetherburnCharacter::AAetherburnCharacter(const FObjectInitializer& ObjectInitializer)\n\t: Super(ObjectInitializer.SetDefaultSubobjectClass<UAetherburnMovementComponent>(ACharacter::CharacterMovementComponentName))')
s = s.replace('bUseControllerRotationYaw = false;', 'bUseControllerRotationYaw = true;')
s = s.replace('GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;', 'GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);')
a = s.index('\tstatic ConstructorHelpers::FClassFinder<UAnimInstance>')
b = s.index('\n\tGetCapsuleComponent()->SetCapsuleSize', a)
s = s[:a] + '''
	ThunderlordAnimClass = UThunderlordAnimInstance::StaticClass();
	GetMesh()->SetAnimInstanceClass(ThunderlordAnimClass);
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(TEXT("/Game/Input/Actions/IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset(TEXT("/Game/Input/Actions/IA_Jump"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(TEXT("/Game/Input/Actions/IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseAsset(TEXT("/Game/Input/Actions/IA_MouseLook"));
	MoveAction = MoveAsset.Object;
	JumpAction = JumpAsset.Object;
	LookAction = LookAsset.Object;
	MouseLookAction = MouseAsset.Object;
''' + s[b:]
s = s.replace('GetCharacterMovement()->bOrientRotationToMovement = true;', 'GetCharacterMovement()->bOrientRotationToMovement = false;\n\tGetCharacterMovement()->SetCrouchedHalfHeight(52.0f);')
s = s.replace('Super::BeginPlay();', '''Super::BeginPlay();
	Stamina = MaximumStamina;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(52.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
	GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);''')
s = s.replace('GetMesh()->SetAnimInstanceClass(ThunderlordAnimClass);', 'GetMesh()->SetAnimInstanceClass(UThunderlordAnimInstance::StaticClass());')
a = s.index('\tif (bIsSliding)', s.index('void AAetherburnCharacter::Tick'))
b = s.index('\nvoid AAetherburnCharacter::SetupPlayerInputComponent', a)
s = s[:a] + '''	if (bIsSliding)
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
''' + s[b:]
s = s.replace('PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AAetherburnCharacter::StartCrouchOrSlide);', 'PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AAetherburnCharacter::ToggleCrouch);')
s = s.replace('\n\tPlayerInputComponent->BindKey(EKeys::C, IE_Released, this, &AAetherburnCharacter::StopCrouchOrSlide);', '')
for line in ['\t\tGetCharacterMovement()->GroundFriction = 1.8f;\n', '\t\tGetCharacterMovement()->BrakingDecelerationWalking = 900.0f;\n', '\t\tGetCharacterMovement()->MaxAcceleration = 0.0f;\n']:
    s = s.replace(line, '')
s = s.replace('void AAetherburnCharacter::DoJumpStart()\n{', 'void AAetherburnCharacter::DoJumpStart()\n{\n\tbCrouchHeld = false;\n\tbCrouchToggled = false;')
s = s.replace('if (!bIsSliding)\n\t{\n\t\tUnCrouch();', 'if (!bIsSliding && !bCrouchToggled)\n\t{\n\t\tUnCrouch();')
s = s.replace('if (!bCrouchHeld)', 'if (!bCrouchHeld && !bCrouchToggled)')
s = s.replace('bIsSprinting = bSprintHeld && !bIsSliding && !bIsCrouched && bMovingForward && Movement->IsMovingOnGround();', 'bIsSprinting = bSprintHeld && !bIsSliding && !bIsCrouched && !bStaminaExhausted && Stamina > 0.0f && GetPlanarSpeed() > 10.0f && Movement->IsMovingOnGround();')
s += '''
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
'''
p.write_text(s)
