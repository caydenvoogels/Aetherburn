// Copyright Epic Games, Inc. All Rights Reserved.


#include "AetherburnPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "AetherburnCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Aetherburn.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "UObject/ConstructorHelpers.h"

AAetherburnPlayerController::AAetherburnPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AAetherburnCameraManager::StaticClass();
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultContext(TEXT("/Game/Input/IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseContext(TEXT("/Game/Input/IMC_MouseLook"));
	if (DefaultContext.Succeeded()) DefaultMappingContexts.AddUnique(DefaultContext.Object);
	if (MouseContext.Succeeded()) MobileExcludedMappingContexts.AddUnique(MouseContext.Object);
}

void AAetherburnPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController() && GetWorld())
	{
		ActorSpawnedHandle = GetWorld()->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &AAetherburnPlayerController::HandleActorSpawned));
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AAetherburnPlayerController::RefreshAffiliationOutlines);
	}

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogAetherburn, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AAetherburnPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld() && ActorSpawnedHandle.IsValid())
	{
		GetWorld()->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		ActorSpawnedHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AAetherburnPlayerController::RefreshAffiliationOutlines()
{
	if (!GetWorld())
	{
		return;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		ApplyAffiliationOutline(*It);
	}
}

void AAetherburnPlayerController::HandleActorSpawned(AActor* Actor)
{
	ApplyAffiliationOutline(Actor);
}

void AAetherburnPlayerController::ApplyAffiliationOutline(AActor* Actor) const
{
	if (!Actor)
	{
		return;
	}

	static UClass* ActorIdentifierClass = StaticLoadClass(
		UActorComponent::StaticClass(), nullptr,
		TEXT("/Game/Hyper/Core/ActorIdentifier/Blueprints/AC_Actor_Identifier.AC_Actor_Identifier_C"));
	if (!ActorIdentifierClass)
	{
		return;
	}

	const FGameplayTag BlueTeam = FGameplayTag::RequestGameplayTag(FName(TEXT("Team Affiliation.Blue")), false);
	const FGameplayTag RedTeam = FGameplayTag::RequestGameplayTag(FName(TEXT("Team Affiliation.Red")), false);
	if (!BlueTeam.IsValid() || !RedTeam.IsValid())
	{
		return;
	}

	int32 StencilValue = 0;
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
	for (const UActorComponent* Component : Components)
	{
		if (!Component || !Component->IsA(ActorIdentifierClass))
		{
			continue;
		}

		const FStructProperty* AffiliationsProperty = FindFProperty<FStructProperty>(
			Component->GetClass(), FName(TEXT("Own Affiliations")));
		if (!AffiliationsProperty || AffiliationsProperty->Struct != FGameplayTagContainer::StaticStruct())
		{
			continue;
		}

		const FGameplayTagContainer* Affiliations = AffiliationsProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(Component);
		if (Affiliations->HasTag(BlueTeam))
		{
			StencilValue = 1; // Hyper's primary outline is blue.
			break;
		}
		if (Affiliations->HasTag(RedTeam))
		{
			StencilValue = 3; // Hyper's secondary outline is red.
			break;
		}
	}

	if (StencilValue == 0)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		Primitive->SetRenderCustomDepth(true);
		Primitive->SetCustomDepthStencilValue(StencilValue);
	}
}

void AAetherburnPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

bool AAetherburnPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
