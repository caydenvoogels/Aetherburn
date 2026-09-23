#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ThunderlordContentLibrary.generated.h"
class UBlendSpace;
class UAnimSequence;
class UAnimBlueprint;

/** Editor-only implementation behind repeatable content setup scripts. */
UCLASS()
class AETHERBURN_API UThunderlordContentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Thunderlord|Editor")
	static bool ConfigureBlendSpace(UBlendSpace* BlendSpace, const TArray<UAnimSequence*>& Clips, const TArray<FVector>& Coordinates, float MaximumSpeed);
	UFUNCTION(BlueprintCallable, Category="Thunderlord|Editor")
	static bool ApplyCrouchPosture(UAnimSequence* DirectionalOutput, UAnimSequence* ForwardWalk, UAnimSequence* ForwardCrouch);
	UFUNCTION(BlueprintCallable, Category="Thunderlord|Editor")
	static bool MakeAnimationInPlace(UAnimSequence* Animation, bool bLockVertical = false);
	UFUNCTION(BlueprintPure, Category="Thunderlord|Editor")
	static bool IsAnimationInPlace(UAnimSequence* Animation, bool bLockVertical = false, float Tolerance = 0.1f);
	UFUNCTION(BlueprintCallable, Category="Thunderlord|Editor")
	static bool ConfigureAnimationBlueprint(UAnimBlueprint* Blueprint, UBlendSpace* Standing, UBlendSpace* Crouch, UAnimSequence* Slide, UAnimSequence* Jump, UAnimSequence* Fall);
};
