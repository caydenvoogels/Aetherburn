#include "ThunderlordGameMode.h"
#include "AetherburnCharacter.h"
#include "AetherburnPlayerController.h"
#include "ThunderlordHUD.h"
#include "UObject/ConstructorHelpers.h"

AThunderlordGameMode::AThunderlordGameMode()
{
	DefaultPawnClass = AAetherburnCharacter::StaticClass();
	static ConstructorHelpers::FClassFinder<APawn> Player(TEXT("/Game/Thunderlord/Blueprints/BP_Thunderlord"));
	if (Player.Succeeded()) DefaultPawnClass = Player.Class;
	PlayerControllerClass = AAetherburnPlayerController::StaticClass();
	static ConstructorHelpers::FClassFinder<APlayerController> ShowcaseController(
		TEXT("/Game/Thunderlord/Blueprints/BP_AetherburnPlayerController"));
	if (ShowcaseController.Succeeded()) PlayerControllerClass = ShowcaseController.Class;
	HUDClass = AThunderlordHUD::StaticClass();
}
