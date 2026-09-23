#include "ThunderlordHUD.h"
#include "AetherburnCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AThunderlordHUD::AThunderlordHUD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshairAsset(
		TEXT("/Game/Thunderlord/UI/T_Thunderlord_TriBoltReticle.T_Thunderlord_TriBoltReticle"));
	if (CrosshairAsset.Succeeded())
	{
		ThunderlordCrosshair = CrosshairAsset.Object;
	}
}

void AThunderlordHUD::DrawHUD()
{
	Super::DrawHUD();
	const AAetherburnCharacter* Player = GetOwningPlayerController() ? Cast<AAetherburnCharacter>(GetOwningPlayerController()->GetPawn()) : nullptr;
	if (!Player || !Canvas) return;
	if (ThunderlordCrosshair)
	{
		const float MaxTextureDimension = FMath::Max(
			static_cast<float>(ThunderlordCrosshair->GetSurfaceWidth()),
			static_cast<float>(ThunderlordCrosshair->GetSurfaceHeight()));
		const float ReticleSize = FMath::Clamp(Canvas->ClipY * 0.045f, 40.0f, 56.0f);
		const float ReticleScale = ReticleSize / MaxTextureDimension;
		const float ReticleAimX = ThunderlordCrosshair->GetSurfaceWidth() * 0.5f;
		// The three bolts converge just below the bitmap center; anchor that point
		// on screen center so the reticle aims where the camera does.
		const float ReticleAimY = ThunderlordCrosshair->GetSurfaceHeight() * 0.57f;
		DrawTextureSimple(
			ThunderlordCrosshair,
			Canvas->ClipX * 0.5f - ReticleAimX * ReticleScale,
			Canvas->ClipY * 0.5f - ReticleAimY * ReticleScale,
			ReticleScale,
			false);
	}
	const float Y = Canvas->ClipY - 72.0f;
	DrawText(TEXT("THUNDERLORD  /  STAMINA"), FLinearColor::White, 28, Y - 22);
	DrawRect(FLinearColor(0.02f, 0.025f, 0.04f, 0.8f), 28, Y, 184, 9);
	DrawRect(FLinearColor(0.85f, 0.63f, 0.22f), 30, Y + 2, 180 * Player->GetStaminaFraction(), 5);
	const FString State = Player->IsSliding() ? TEXT("SLIDING") : Player->bIsCrouched ? TEXT("CROUCH") : Player->IsSprinting() ? TEXT("SPRINT") : TEXT("WALK");
	DrawText(State, FLinearColor::White, 28, Y + 16);
	DrawText(TEXT("WASD  Move   |   Shift  Sprint   |   C  Toggle crouch   |   Ctrl  Hold crouch   |   Space  Jump"), FLinearColor(0.8f, 0.8f, 0.8f), 28, 24);
}
