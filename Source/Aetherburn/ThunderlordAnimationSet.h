#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ThunderlordAnimationSet.generated.h"

class UBlendSpace;
class UAnimSequence;

/** Editable movement-to-animation map. Blend spaces map direction and speed. */
UCLASS(BlueprintType)
class AETHERBURN_API UThunderlordAnimationSet : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Directional loops")
	TObjectPtr<UBlendSpace> WalkSprint;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Directional loops")
	TObjectPtr<UBlendSpace> Crouch;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Actions")
	TMap<FName, TObjectPtr<UAnimSequence>> Actions;
};
