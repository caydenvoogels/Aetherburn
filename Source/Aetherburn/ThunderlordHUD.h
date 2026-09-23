#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ThunderlordHUD.generated.h"

UCLASS()
class AETHERBURN_API AThunderlordHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
