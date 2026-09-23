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

private:
	UPROPERTY(EditDefaultsOnly, Category="HUD|Crosshair")
	TObjectPtr<UTexture2D> ThunderlordCrosshair;
};
