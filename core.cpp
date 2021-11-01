#include "core.h"
#include "memory.h"
#include "redirect.h"
#include "sdk.h"
#include "peh.h"
#include "Athena.h"

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

	//INITIALIZE GOBJECTS AND FUNCTIONS
	StaticFindObject = decltype(StaticFindObject)(FindPattern(_("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 80 3D ? ? ? ? ? 41 0F B6 D9 49 8B F8 48 8B F2")));

	uint64 NameToString = FindPattern(_("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9"));
	if (!NameToString) NameToString = FindPattern(_("40 53 48 83 EC 40 83 79 04 00 48 8B DA 75 19 E8 ? ? ? ?"));
	if (!NameToString) NameToString = FindPattern(_("48 89 5C 24 ? 57 48 83 EC 30 83 79 04 00 48 8B DA 48 8B F9"));
	FNameToString = decltype(FNameToString)(NameToString);

	uint64 Free = FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 48 8B 0D ?"));
	if (!Free) Free = FindPattern(_("48 85 C9 74 1D 4C 8B 05 ? ? ? ? 4D 85 C0"));
	if (!Free) Free = FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9"));

	FreeInternal = decltype(FreeInternal)(Free);

	uint64 SpawnActorAddress = FindPattern(_("40 53 56 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 0F 28 1D ? ? ? ? 0F 57 D2 48 8B B4 24 ? ? ? ? 0F 28 CB"));
	SpawnActor = decltype(SpawnActor)(SpawnActorAddress);
}

void Core::InitializeHook()
{
	UEngine = FindObject(_(L"/Engine/Transient.FortEngine_0"));
	GameStatics = FindObject(_(L"/Script/Engine.Default__GameplayStatics"));
	kismetMathLib = FindObject(_(L"/Script/Engine.Default__KismetMathLibrary"));
	kismetGuidLib = FindObject(_(L"/Script/Engine.Default__KismetGuidLibrary"));

	PE = decltype(PE)(UEngine->Vtable[0x40]);
	DetourHook(&(void*&)PE, ProcessEvent);
}

void Core::InitializeGlobals()
{
	PlayerController = UEngine
		->Child(_("GameViewport"))
		->Child(_("GameInstance"))
		->Child<TArray<UObject*>>(_("LocalPlayers"))[0]
		->Child(_("PlayerController"));

	GameMode = UEngine
		->Child(_("GameViewport"))
		->Child(_("World"))
		->Child(_("AuthorityGameMode"));

	GameState = UEngine
		->Child(_("GameViewport"))
		->Child(_("World"))
		->Child(_("GameState"));

	World = UEngine
		->Child(_("GameViewport"))
		->Child(_("World"));

	WorldSettings = World
		->Child(_("PersistentLevel"))
		->Child(_("WorldSettings"));

	//Console & CheatManager
	auto CheatMans = FindObject(_(L"/Script/Engine.CheatManager"));
	CheatManager = GameStatics->Call<UObject*>(_("SpawnObject"), CheatMans, PlayerController);
	PlayerController->Child(_("CheatManager")) = CheatManager;
	UEngine->Child(_("GameViewport"))->Child(_("ViewportConsole")) = GameStatics->Call<UObject*>(_("SpawnObject"), FindObject(_(L"/Script/Engine.Console")), UEngine->Child(_("GameViewport")));
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
}

void Core::OnServerLoadingScreenDropped()
{
	Athena::InitializeInventory();
	Athena::GrantDefaultAbilities();
}


