// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "CarMovemenSystemHud.generated.h"


UCLASS(config = Game)
class ACarMovemenSystemHud : public AHUD
{
	GENERATED_BODY()

public:
	ACarMovemenSystemHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
