#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ThunderlordHUD.generated.h"

class UTexture2D;

UCLASS()
class AETHERBURN_API AThunderlordHUD : public AHUD
{
	GENERATED_BODY()
public:
	AThunderlordHUD();
	virtual void DrawHUD() override;
	void ReportDummyDamage(float Damage, const FVector& WorldLocation, bool bHeadshot);

private:
	UPROPERTY(EditDefaultsOnly, Category="HUD|Crosshair")
	TObjectPtr<UTexture2D> ThunderlordCrosshair;
	UPROPERTY(EditDefaultsOnly, Category="HUD|Health")
	TObjectPtr<UTexture2D> GreekHealthFrame;
	struct FDamagePopup
	{
		float Damage = 0.0f;
		FVector WorldLocation = FVector::ZeroVector;
		float StartTime = 0.0f;
		bool bHeadshot = false;
	};
	TArray<FDamagePopup> DummyDamagePopups;
	float DummyDamageTotal = 0.0f;
	float DummyLastDamage = 0.0f;
	bool bDummyLastHitHeadshot = false;
};
