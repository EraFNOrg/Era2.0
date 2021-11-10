#include "core.h"
#include "memory.h"
#include "redirect.h"
#include "sdk.h"
#include "peh.h"
#include "Athena.h"

UObject* Core::SpawnActorEasy(UObject* Class, FVector Location)
{
	FTransform Transform;

	Transform.Translation = Location;
	Transform.Scale3D = FVector(1, 1, 1);
	Transform.Rotation = FQuat{0,0,0,0 };

	auto TempActor = GameStatics->Call<UObject*>(_("BeginDeferredActorSpawnFromClass"), World, Class, Transform, char(0), nullptr);
	return GameStatics->Call<UObject*>(_("FinishSpawningActor"), TempActor, Transform);
}

void Core::Setup()
{
	AllocConsole();

	ShowWindow(GetConsoleWindow(), SW_SHOW);
	FILE* fp;
	freopen_s(&fp, "CONOIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	printf(_("EraFN Copyright (C) 2021 danii\n\nThis program is free software: you can redistribute it and/or modify\n"));
	printf(_("it under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of the License, or\n"));
	printf(_("(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\n"));
	printf(_("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\n\n"));

	printf(_("Era 2.0 || Made by danii#4000\nBackend by Kyiro#7884\nLauncher by ozne#3303 and Not a Robot#6932\nSpecial Thanks to Sizzy, Kemo, Mix, Fischsalat!\n\nEnjoy!\n\n\n"));

	Redirect::CurlSet = decltype(Redirect::CurlSet)(FindPattern(_("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 33 ED 49 8B F0 48 8B D9")));
	Redirect::CurlEasy = decltype(Redirect::CurlEasy)(FindPattern(_("89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 48 83 EC 28 48 85 C9 75 08 8D 41 2B 48 83 C4 28 C3 4C")));

	//SSL BYPASS
	PGH::Hook((uint64)Redirect::CurlEasy, (uint64)Redirect::CurlEasyHook);

	//SETUP CORE
	StaticFindObject = decltype(StaticFindObject)(FindPattern(_("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 80 3D ? ? ? ? ? 41 0F B6 D9 49 8B F8 48 8B F2")));

	GetEngineVersion = decltype(GetEngineVersion)(FindPattern(_("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B C8 41 B8 04 ? ? ? 48 8B D3")));

	//This is the fix to arrays behaving "bad", such as causing crashes when attempting to go back to lobby
	//or freezing the game when adding to inventory in S9+ 
	Realloc = decltype(Realloc)(FindPattern(_(/*Gonna add sigs tomorrow - danii*/"")));

	//Initialize hardcoded offsets and Functions
	switch ((int)(stod(GetEngineVersion().ToString().substr(0, 4)) * 100))
	{
	case 416: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 48 8B 0D ?")));
		offsets::Children = 0x38;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0x8E;
		offsets::ReturnValueOffset = 0x90;
		offsets::SuperClass = 0x30;
		offsets::StructSize = 0x40;
		break;
	}
	case 419: {
		FNameToString = decltype(FNameToString)(FindPattern(_("40 53 48 83 EC 40 83 79 04 00 48 8B DA 75 19 E8 ? ? ? ?")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 1D 4C 8B 05 ? ? ? ? 4D 85 C0")));
		offsets::Children = 0x38;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0x8E;
		offsets::ReturnValueOffset = 0x90;
		offsets::SuperClass = 0x30;
		offsets::StructSize = 0x40;
		break;
	}
	case 420: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 1D 4C 8B 05 ? ? ? ? 4D 85 C0")));
		offsets::Children = 0x38;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0x8E;
		offsets::ReturnValueOffset = 0x90;
		offsets::SuperClass = 0x30;
		offsets::StructSize = 0x40;
		break;
	}
	case 421: {
		FNameToString = decltype(FNameToString)(FindPattern(_("48 89 5C 24 ? 57 48 83 EC 30 83 79 04 00 48 8B DA 48 8B F9")));
		FreeInternal = decltype(FreeInternal)(FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9")));
		offsets::Children = 0x38;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0x8E;
		offsets::ReturnValueOffset = 0x90;
		offsets::SuperClass = 0x30;
		offsets::StructSize = 0x40;
		break;
	}
	case 422: {
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
	}
}

void Core::InitializeHook()
{
	GameStatics = FindObject(_(L"/Script/Engine.Default__GameplayStatics"));
	kismetMathLib = FindObject(_(L"/Script/Engine.Default__KismetMathLibrary"));
	kismetGuidLib = FindObject(_(L"/Script/Engine.Default__KismetGuidLibrary"));
	kismetStringLib = FindObject(_(L"/Script/Engine.Default__KismetStringLibrary"));

	PE = decltype(PE)(FindObject(_(L"/Script/CoreUObject.Default__Object"))->Vtable[0x40]);
	DetourHook(&(void*&)PE, ProcessEvent);
	
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
	auto CheatMans = FindObject(_(L"/Script/Engine.CheatManager"));
	CheatManager = GameStatics->Call<UObject*>(_("SpawnObject"), CheatMans, PlayerController);
	PlayerController->Child(_("CheatManager")) = CheatManager;
	GameViewportClient->Child(_("ViewportConsole")) = GameStatics->Call<UObject*>(_("SpawnObject"), FindObject(_(L"/Script/Engine.Console")), GameViewportClient);

	//Changes default console bind
	Athena::ConsoleKey();
}

//HOOKS
void Core::OnReadyToStartMatch()
{
	bLoadedInMatch = true;
	Core::InitializeGlobals();
	Athena::SpawnPawn();
	Athena::ShowSkin();
	Athena::DestroyLods();
	Athena::DropLoadingScreen();
	Athena::InitializeInventory();
}

void Core::OnServerLoadingScreenDropped()
{
	Athena::GrantDefaultAbilities();
	Athena::RemoveNetDebugUI();
	Athena::TeleportToSpawnIsland();
}


