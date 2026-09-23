#include "ThunderlordHUD.h"
#include "AetherburnCharacter.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"

void AThunderlordHUD::DrawHUD()
{
	Super::DrawHUD();
	const AAetherburnCharacter* Player = GetOwningPlayerController() ? Cast<AAetherburnCharacter>(GetOwningPlayerController()->GetPawn()) : nullptr;
	if (!Player || !Canvas) return;
	const float Y = Canvas->ClipY - 72.0f;
	DrawText(TEXT("THUNDERLORD  /  STAMINA"), FLinearColor::White, 28, Y - 22);
	DrawRect(FLinearColor(0.02f, 0.025f, 0.04f, 0.8f), 28, Y, 184, 9);
	DrawRect(FLinearColor(0.85f, 0.63f, 0.22f), 30, Y + 2, 180 * Player->GetStaminaFraction(), 5);
	const FString State = Player->IsSliding() ? TEXT("SLIDING") : Player->bIsCrouched ? TEXT("CROUCH") : Player->IsSprinting() ? TEXT("SPRINT") : TEXT("WALK");
	DrawText(State, FLinearColor::White, 28, Y + 16);
	DrawText(TEXT("WASD  Move   |   Shift  Sprint   |   C  Toggle crouch   |   Ctrl  Hold crouch   |   Space  Jump"), FLinearColor(0.8f, 0.8f, 0.8f), 28, 24);
}
