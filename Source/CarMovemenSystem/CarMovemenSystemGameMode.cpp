// Copyright Epic Games, Inc. All Rights Reserved.

#include "CarMovemenSystemGameMode.h"
#include "CarMovemenSystemPawn.h"
#include "CarMovemenSystemHud.h"

ACarMovemenSystemGameMode::ACarMovemenSystemGameMode()
{
	DefaultPawnClass = ACarMovemenSystemPawn::StaticClass();
	HUDClass = ACarMovemenSystemHud::StaticClass();
}
