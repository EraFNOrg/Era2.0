#include "core.h"
#include "memory.h"
#include "redirect.h"
#include "sdk.h"
#include "peh.h"
#include "Athena.h"

UObject* Core::SpawnActorEasy(UObject* Class, FVector Location, FRotator Rotation)
{
	if (!Class) return nullptr;

	SpawnActorParams params{};

	return SpawnActor(World, Class, &Location, &Rotation, params);
}

void Core::Setup()
{
	Redirect::CurlSet = decltype(Redirect::CurlSet)(FindPattern(_("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 33 ED 49 8B F0 48 8B D9")));
	Redirect::CurlEasy = decltype(Redirect::CurlEasy)(FindPattern(_("89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 48 83 EC 28 48 85 C9 75 08 8D 41 2B 48 83 C4 28 C3 4C")));

	//SSL BYPASS
	PGH::Hook((uint64)Redirect::CurlEasy, (uint64)Redirect::CurlEasyHook);

	//SETUP CORE
	StaticFindObject = decltype(StaticFindObject)(FindPattern(_("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 80 3D ? ? ? ? ? 41 0F B6 D9 49 8B F8 48 8B F2")));

	GetEngineVersion = decltype(GetEngineVersion)(FindPattern(_("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B C8 41 B8 04 ? ? ? 48 8B D3")));

	SpawnActor = decltype(SpawnActor)(FindPattern(_("40 53 56 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 0F 28 1D ? ? ? ? 0F 57 D2 48 8B B4 24 ? ? ? ? 0F 28 CB")));
	
	GObjectArray = decltype(GObjectArray)(FindPattern(_("49 63 C8 48 8D 14 40 48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1"), true, 10));

	//This is the fix to arrays behaving "bad", such as causing crashes when attempting to go back to lobby
	//or freezing the game when adding to inventory in S9+ 
	Realloc = decltype(Realloc)(FindPattern(_("E8 ? ? ? ? 48 89 03 48 8B 5C 24 ? 48 83 C4 20"), true, 1));

	//Initialize hardcoded offsets and Functions
	switch ((int)(stod(GetEngineVersion().ToString().substr(0, 4)) * 100))
	{
	case 416: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 48 8B 0D ?")));
		break;
	}
	case 419: {
		FNameToString = decltype(FNameToString)(FindPattern(_("40 53 48 83 EC 40 83 79 04 00 48 8B DA 75 19 E8 ? ? ? ?")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 1D 4C 8B 05 ? ? ? ? 4D 85 C0")));
		break;
	}
	case 420: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 1D 4C 8B 05 ? ? ? ? 4D 85 C0")));
		break;
	}
	case 421: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 57 48 83 EC 30 83 79 04 00 48 8B DA 48 8B F9")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9")));
		break;
	}
	case 422:
	case 423: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 57 48 83 EC 30 83 79 04 00 48 8B DA 48 8B F9")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9")));
		offsets::Children = 0x48;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0x9E;
		offsets::ReturnValueOffset = 0xA0;
		offsets::SuperClass = 0x40;
		offsets::StructSize = 0x50;
		break;
	}
	case 424: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 30 8B 01 48 8B F1 44 8B 49 04 8B F8")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9")));
		offsets::Children = 0x48;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0x9E;
		offsets::ReturnValueOffset = 0xA0;
		offsets::SuperClass = 0x40;
		offsets::StructSize = 0x50;
		break;
	}
	case 425: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 30 8B 01 48 8B F1 44 8B 49 04 8B F8")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9")));
		offsets::Children = 0x48;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0xCE;
		offsets::ReturnValueOffset = 0xD0;
		offsets::SuperClass = 0x40;
		offsets::StructSize = 0x58;
		offsets::Offset = 0x4C;
		offsets::Class = 0x78;
		break;
	}
	}
}

void Core::InitializeHook()
{
	AllocConsole();

	ShowWindow(GetConsoleWindow(), SW_SHOW);
	FILE* fp;
	freopen_s(&fp, "CONOIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	printf(_("EraFN Copyright (C) 2021 danii#2961\n\nThis program is free software: you can redistribute it and/or modify\n"));
	printf(_("it under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of the License, or\n"));
	printf(_("(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\n"));
	printf(_("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\n\n"));

	printf(_("Era 2.0 || Made by danii#2961 & sizzy#1337\nBackend by Kyiro#7884\nLauncher by ozne#3303 and Not a Robot#6932\nSpecial Thanks to Kemo, Mix, Fischsalat!\n\nEnjoy!\n\n\n"));

	FreeConsole();

	GameStatics = FindObject(_(L"/Script/Engine.Default__GameplayStatics"));
	kismetMathLib = FindObject(_(L"/Script/Engine.Default__KismetMathLibrary"));
	kismetGuidLib = FindObject(_(L"/Script/Engine.Default__KismetGuidLibrary"));
	kismetStringLib = FindObject(_(L"/Script/Engine.Default__KismetStringLibrary"));

	//CH2 checks
	if ((GetEngineVersion().ToString().substr(34, 4).find(_("12.")) != -1))
	{
		PE = decltype(PE)(FindObject(_(L"/Script/CoreUObject.Default__Object"))->Vtable[0x42]);
		Hook(FindObject(_(L"/Script/CoreUObject.Default__Object"))->Vtable[0x42], (LPVOID*)(&PE), ProcessEvent);
	}
	else if ((GetEngineVersion().ToString().substr(34, 4).find(_("11.")) != -1) ||
		(GetEngineVersion().ToString().substr(34, 4).find(_("7.4")) != -1)) {
		PE = decltype(PE)(FindObject(_(L"/Script/CoreUObject.Default__Object"))->Vtable[0x41]);
		Hook(FindObject(_(L"/Script/CoreUObject.Default__Object"))->Vtable[0x41], (LPVOID*)(&PE), ProcessEvent);
	}
	else {
		PE = decltype(PE)(FindObject(_(L"/Script/CoreUObject.Default__Object"))->Vtable[0x40]);
		Hook(FindObject(_(L"/Script/CoreUObject.Default__Object"))->Vtable[0x40], (LPVOID*)(&PE), ProcessEvent);
	}

	GameViewportClient = GameStatics->Call<UObject*>(_("GetPlayerController"), FindObject(_(L"/Game/Maps/Frontend.Frontend")), 0)->Child(_("Player"))->Child(_("ViewportClient"));
}

void Core::InitializeGlobals()
{
	PlayerController = GameViewportClient	
		->Child(_("GameInstance"))
		->Child<TArray<UObject*>>(_("LocalPlayers"))[0]
		->Child(_("PlayerController"));

	GameMode = GameViewportClient
		->Child(_("World"))
		->Child(_("AuthorityGameMode"));

	GameState = GameViewportClient
		->Child(_("World"))
		->Child(_("GameState"));

	World = GameViewportClient
		->Child(_("World"));

	WorldSettings = World
		->Child(_("PersistentLevel"))
		->Child(_("WorldSettings"));

	//Console & CheatManager
	CheatManager = GameStatics->Call<UObject*>(_("SpawnObject"), FindObject(_(L"/Script/Engine.CheatManager")), PlayerController);
	PlayerController->Child(_("CheatManager")) = CheatManager;
	GameViewportClient->Child(_("ViewportConsole")) = GameStatics->Call<UObject*>(_("SpawnObject"), FindObject(_(L"/Script/Engine.Console")), GameViewportClient);

	//Changes default console bind
	Athena::ConsoleKey();
}

void Core::PlayButton()
{
	Core::InitializeGlobals();

	//Season check
	if ((GetEngineVersion().ToString().substr(34, 4).find(_("11.")) != -1) ||
		(GetEngineVersion().ToString().substr(34, 4).find(_("12.")) != -1)) {
		PlayerController->Call(_("SwitchLevel"), FString(_(L"Apollo_Terrain")));
	}
	else
	{
		PlayerController->Call(_("SwitchLevel"), FString(_(L"Athena_Terrain")));
	}

	bPressedPlay = !bPressedPlay;
}


//HOOKS
void Core::OnReadyToStartMatch()
{
	bLoadedInMatch = true;
	bInFrontend = !bInFrontend;
	Core::InitializeGlobals();
	Athena::SpawnPawn();
	Athena::ShowSkin();
	Athena::DestroyLods();
	Athena::DropLoadingScreen();
	Athena::FixBuildingFoundations();
	Athena::InitializeInventory();
	Athena::GrantDefaultAbilities();
}

void Core::OnServerLoadingScreenDropped()
{
	if (bInFrontend)
	{
		bPressedPlay = !bPressedPlay;
		bLoadedInMatch = !bLoadedInMatch;
		bDroppedFromAircraft = !bDroppedFromAircraft;
		return;
	}

	Athena::RemoveNetDebugUI();
	Athena::TeleportToSpawnIsland();
	Athena::Fixbus();
}


