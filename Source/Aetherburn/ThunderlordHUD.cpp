#include "ThunderlordHUD.h"
#include "AetherburnCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AThunderlordHUD::AThunderlordHUD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshairAsset(
		TEXT("/Game/Thunderlord/UI/T_Thunderlord_TriBoltReticle.T_Thunderlord_TriBoltReticle"));
	if (CrosshairAsset.Succeeded())
	{
		ThunderlordCrosshair = CrosshairAsset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UTexture2D> HealthFrameAsset(
		TEXT("/Game/Thunderlord/UI/T_Aetherburn_GreekHealthFrame.T_Aetherburn_GreekHealthFrame"));
	if (HealthFrameAsset.Succeeded())
	{
		GreekHealthFrame = HealthFrameAsset.Object;
	}
}

void AThunderlordHUD::ReportDummyDamage(float Damage, const FVector& WorldLocation, bool bHeadshot)
{
	if (Damage <= 0.0f || !GetWorld())
	{
		return;
	}

	FDamagePopup& Popup = DummyDamagePopups.AddDefaulted_GetRef();
	Popup.Damage = Damage;
	Popup.WorldLocation = WorldLocation;
	Popup.StartTime = GetWorld()->GetTimeSeconds();
	Popup.bHeadshot = bHeadshot;
	DummyDamageTotal += Damage;
	DummyLastDamage = Damage;
	bDummyLastHitHeadshot = bHeadshot;
}

void AThunderlordHUD::DrawHUD()
{
	Super::DrawHUD();
	APlayerController* PlayerController = GetOwningPlayerController();
	const AAetherburnCharacter* Player = PlayerController ? Cast<AAetherburnCharacter>(PlayerController->GetPawn()) : nullptr;
	if (!Player || !Canvas || !PlayerController) return;
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

	// A compact bronze-and-marble vitality plate with a Greek meander edge.
	const float Scale = FMath::Clamp(Canvas->ClipY / 900.0f, 0.85f, 1.25f);
	const float X = 34.0f * Scale;
	const float Y = Canvas->ClipY - 151.0f * Scale;
	const float Width = 286.0f * Scale;
	const float Height = GreekHealthFrame
		? Width * GreekHealthFrame->GetSurfaceHeight() / GreekHealthFrame->GetSurfaceWidth()
		: 36.0f * Scale;
	const FLinearColor Bronze(0.83f, 0.64f, 0.30f, 0.98f);
	const FLinearColor DarkBronze(0.22f, 0.14f, 0.07f, 0.96f);
	const FLinearColor Marble(0.88f, 0.82f, 0.66f, 1.0f);
	const float HealthFraction = FMath::Clamp(Player->GetHealthFraction(), 0.0f, 1.0f);

	DrawText(TEXT("OLYMPUS  /  VITALITY"), Marble, X + 2.0f * Scale, Y - 23.0f * Scale);
	const float InsetX = GreekHealthFrame ? 27.0f * Scale : 5.0f * Scale;
	const float InsetY = GreekHealthFrame ? 31.0f * Scale : 5.0f * Scale;
	const float FillWidth = FMath::Max(0.0f, Width - InsetX * 2.0f);
	const float FillHeight = FMath::Max(1.0f, Height - InsetY * 2.0f);
	DrawRect(DarkBronze, X + InsetX, Y + InsetY, FillWidth, FillHeight);
	DrawRect(FLinearColor(0.34f, 0.055f, 0.045f, 1.0f), X + InsetX, Y + InsetY, FillWidth * HealthFraction, FillHeight);
	if (GreekHealthFrame)
	{
		DrawTextureSimple(GreekHealthFrame, X, Y, Width / GreekHealthFrame->GetSurfaceWidth(), false);
	}
	else
	{
		DrawRect(Bronze, X, Y, Width, 2.0f * Scale);
		DrawRect(Bronze, X, Y + Height - 2.0f * Scale, Width, 2.0f * Scale);
		DrawRect(Bronze, X, Y, 2.0f * Scale, Height);
		DrawRect(Bronze, X + Width - 2.0f * Scale, Y, 2.0f * Scale, Height);
	}
	const FString HealthText = FString::Printf(TEXT("%03.0f  /  %03.0f"), Player->GetHealth(), Player->GetMaximumHealth());
	DrawText(HealthText, Marble, X + Width + 10.0f * Scale, Y + 5.0f * Scale);

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	DummyDamagePopups.RemoveAll([CurrentTime](const FDamagePopup& Popup)
	{
		return CurrentTime - Popup.StartTime > 1.5f;
	});
	for (const FDamagePopup& Popup : DummyDamagePopups)
	{
		const float Age = CurrentTime - Popup.StartTime;
		FVector PopupLocation = Popup.WorldLocation + FVector(0.0f, 0.0f, 24.0f + Age * 34.0f);
		FVector2D ScreenLocation;
		if (PlayerController->ProjectWorldLocationToScreen(PopupLocation, ScreenLocation, true))
		{
			const float Opacity = FMath::Clamp(1.0f - Age / 1.5f, 0.0f, 1.0f);
			const FLinearColor PopupColor = Popup.bHeadshot
				? FLinearColor(1.0f, 0.78f, 0.25f, Opacity)
				: FLinearColor(1.0f, 0.94f, 0.80f, Opacity);
			const FString PopupText = Popup.bHeadshot
				? FString::Printf(TEXT("HEADSHOT  -%.0f"), Popup.Damage)
				: FString::Printf(TEXT("-%.0f"), Popup.Damage);
			DrawText(PopupText, PopupColor, ScreenLocation.X, ScreenLocation.Y, nullptr, Scale * 1.15f);
		}
	}

	if (DummyDamageTotal > 0.0f)
	{
		const float PanelWidth = 270.0f * Scale;
		const float PanelX = Canvas->ClipX - PanelWidth - 30.0f * Scale;
		const float PanelY = 28.0f * Scale;
		DrawRect(FLinearColor(0.035f, 0.025f, 0.02f, 0.86f), PanelX, PanelY, PanelWidth, 78.0f * Scale);
		DrawRect(Bronze, PanelX, PanelY, PanelWidth, 2.0f * Scale);
		DrawText(TEXT("RED ZEUS  /  DAMAGE LOG"), Marble,
			PanelX + 12.0f * Scale, PanelY + 10.0f * Scale, nullptr, Scale);
		const FString LastHitText = FString::Printf(TEXT("LAST HIT  %s%.0f"),
			bDummyLastHitHeadshot ? TEXT("HEADSHOT  ") : TEXT(""), DummyLastDamage);
		DrawText(LastHitText, bDummyLastHitHeadshot ? Bronze : Marble,
			PanelX + 12.0f * Scale, PanelY + 32.0f * Scale, nullptr, Scale);
		DrawText(FString::Printf(TEXT("TOTAL DAMAGE  %.0f"), DummyDamageTotal), Marble,
			PanelX + 12.0f * Scale, PanelY + 53.0f * Scale, nullptr, Scale);
	}
}
