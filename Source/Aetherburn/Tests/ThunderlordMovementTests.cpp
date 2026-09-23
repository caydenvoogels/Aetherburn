#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AetherburnCharacter.h"
#include "AetherburnMovementComponent.h"
#include "ThunderlordAnimInstance.h"
#include "ThunderlordAnimationSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Animation/BlendSpace.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThunderlordMovementTest, "Aetherburn.Thunderlord.MovementAndAnimation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThunderlordMovementTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName("ThunderlordMovementTest"));
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeNewWorld(UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true));
	AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>();
	Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
	Floor->SetActorScale3D(FVector(400, 400, 1));
	Floor->SetActorLocation(FVector(0, 0, -50));
	AAetherburnCharacter* Player = World->SpawnActor<AAetherburnCharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	World->BeginPlay();
	auto Tick = [&](float Seconds, bool Moving = false)
	{
		for (int32 I = 0; I < FMath::RoundToInt(Seconds * 60); ++I)
		{
			if (Moving) Player->AddMovementInput(FVector::ForwardVector);
			++GFrameCounter;
			World->Tick(LEVELTICK_All, 1.0f / 60.0f);
		}
	};
	Tick(0.5f);
	auto* Move = Cast<UAetherburnMovementComponent>(Player->GetCharacterMovement());
	TestNotNull(TEXT("Dedicated movement component"), Move);
	TestTrue(TEXT("Character stands on collision floor"), Move->IsMovingOnGround());
	TestTrue(TEXT("Body is visible to its owner"), Player->GetMesh()->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::None && !Player->GetMesh()->bOwnerNoSee && Player->GetMesh()->IsVisible());
	const auto* Anim = Cast<UThunderlordAnimInstance>(Player->GetMesh()->GetAnimInstance());
	TestNotNull(TEXT("Native animation graph is active"), Anim);
	if (Anim && Anim->AnimationSet)
	{
		TestNotNull(TEXT("Directional walk/sprint map"), Anim->AnimationSet->WalkSprint.Get());
		TestNotNull(TEXT("Directional crouch map"), Anim->AnimationSet->Crouch.Get());
		TestTrue(TEXT("Authored slide loop assigned"), Anim->AnimationSet->Actions.Contains("SlideLoop"));
	}
	else AddError(TEXT("Animation set is missing"));
	AddInfo(FString::Printf(TEXT("Mesh bounds height %.2f cm"), Player->GetMesh()->GetSkeletalMeshAsset()->GetBounds().BoxExtent.Z * 2));

	Player->StartCrouchOrSlide();
	Tick(0.15f);
	TestTrue(TEXT("Ctrl crouch changes actual capsule"), Player->bIsCrouched && Player->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() < 60);
	Player->StopCrouchOrSlide();
	Tick(0.15f);
	TestFalse(TEXT("Ctrl release uncrouches"), Player->bIsCrouched);
	Player->ToggleCrouch();
	Tick(0.2f);
	TestTrue(TEXT("C toggles crouch on"), Player->bIsCrouched);
	Player->ToggleCrouch();
	Tick(0.2f);
	TestFalse(TEXT("C toggles crouch off"), Player->bIsCrouched);

	Player->StartSprint();
	Tick(1.0f, true);
	TestTrue(TEXT("Sprint reaches slide entry speed"), Player->GetPlanarSpeed() > 600);
	Player->StartCrouchOrSlide();
	Player->StopCrouchOrSlide();
	Tick(1.5f, true);
	TestTrue(TEXT("Slide survives old 1.25-second cutoff and key release"), Player->IsSliding());
	TestTrue(TEXT("Crouch does not clamp slide speed"), Player->GetPlanarSpeed() > 500);
	Tick(3.5f, false);
	TestFalse(TEXT("Flat slide eventually ends from low speed"), Player->IsSliding());
	Player->StopSprint();
	Tick(5);

	// Change the ground into a long 15 degree ramp and test the actual floor normal.
	Floor->SetActorRotation(FRotator(-15, 0, 0));
	Player->SetActorLocation(FVector(0, 0, 150));
	Move->Velocity = FVector::ZeroVector;
	Move->SetMovementMode(MOVE_Falling);
	Tick(0.7f);
	Player->StartSprint();
	Tick(1, true);
	Player->StartCrouchOrSlide();
	const float EntrySpeed = Player->GetPlanarSpeed();
	Tick(0.8f, true);
	TestTrue(TEXT("Downhill slide gains speed"), Player->IsSliding() && Player->GetPlanarSpeed() > EntrySpeed);
	AddInfo(FString::Printf(TEXT("Downhill speed %.1f -> %.1f"), EntrySpeed, Player->GetPlanarSpeed()));
	Tick(5, true);
	TestTrue(TEXT("Downhill slide continues beyond old distance and time limits"), Player->IsSliding());
	TestTrue(TEXT("Downhill speed remains bounded"), Player->GetPlanarSpeed() <= Move->MaximumSlideSpeed + 1);
	Move->Velocity = FVector(100, 0, 0);
	Tick(0.05f);
	TestFalse(TEXT("Low speed ends slide"), Player->IsSliding());
	TestTrue(TEXT("Slide exit starts cooldown"), Player->GetSlideCooldownRemaining() > 0);
	Player->StopCrouchOrSlide();
	Player->StopSprint();
	Floor->SetActorRotation(FRotator::ZeroRotator);
	Player->SetActorLocation(FVector(0, 0, 150));
	Move->Velocity = FVector::ZeroVector;
	Move->SetMovementMode(MOVE_Falling);
	Tick(5);
	Player->StartSprint();
	Tick(4.4f, true);
	TestTrue(TEXT("Sprinting depletes stamina"), Player->GetStaminaFraction() < 0.05f);
	TestFalse(TEXT("Exhaustion stops sprint"), Player->IsSprinting());
	Player->StopSprint();
	Tick(4.5f);
	TestTrue(TEXT("Stamina recovers"), Player->GetStaminaFraction() > 0.99f);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
