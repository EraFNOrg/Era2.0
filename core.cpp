#include "core.h"
#include "memory.h"
#include "redirect.h"
#include "sdk.h"
#include "peh.h"
#include "Athena.h"
#include "Libraries/era_api.h"
#pragma comment(lib, "Libraries/era_api.lib")

UObject* Core::SpawnActorEasy(UObject* Class, FVector Location, FRotator Rotation)
{
	if (!Class) return nullptr;

	SpawnActorParams params{};

	return SpawnActor(World, Class, &Location, &Rotation, params);
}

void Core::Setup()
{
	//Start the backend
	api_init_thread(4444);
	
	Redirect::CurlSet = decltype(Redirect::CurlSet)(FindPattern(_("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 33 ED 49 8B F0 48 8B D9")));
	Redirect::CurlEasy = decltype(Redirect::CurlEasy)(FindPattern(_("89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 48 83 EC 28 48 85 C9 75 08 8D 41 2B 48 83 C4 28 C3 4C")));

	//SSL BYPASS
	PGH::Hook((uint64)Redirect::CurlEasy, (uint64)Redirect::CurlEasyHook);

	//SETUP CORE
	StaticFindObject = decltype(StaticFindObject)(FindPattern(_("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 80 3D ? ? ? ? ? 41 0F B6 D9 49 8B F8 48 8B F2")));
	
	StaticLoadObject = decltype(StaticLoadObject)(FindPattern(_("E8 ? ? ? ? 48 8B 4C 24 ? 48 8B F0 48 85 C9 74 05 E8 ? ? ? ? 48 8B 7C 24 ? 48 8B C6 48 8B 74 24 ?"), true, 1));

	GetEngineVersion = decltype(GetEngineVersion)(FindPattern(_("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B C8 41 B8 04 ? ? ? 48 8B D3")));

	SpawnActor = decltype(SpawnActor)(FindPattern(_("40 53 56 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 0F 28 1D ? ? ? ? 0F 57 D2 48 8B B4 24 ? ? ? ? 0F 28 CB")));
	
	GObjectArray = decltype(GObjectArray)(FindPattern(_("49 63 C8 48 8D 14 40 48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1"), true, 10));

	GetDataTableRow = decltype(GetDataTableRow)(FindPattern(_("E8 ? ? ? ? 44 0F B6 F8 E9 ? ? ? ? 4C 8D 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 8D 15 ? ? ? ?"), true, 1));
	
	CopyScriptStruct = decltype(CopyScriptStruct)(FindPattern(_("E8 ? ? ? ? B9 ? ? ? ? ? 89 ? ? E8 ? ? ? ? ? ? ? 48 85 C0 74 ? C7 ? ? ? ? ? ? C7 ? ? ? ? ? ? 4C"), true, 1) > 0 ? FindPattern(_("E8 ? ? ? ? B9 ? ? ? ? ? 89 ? ? E8 ? ? ? ? ? ? ? 48 85 C0 74 ? C7 ? ? ? ? ? ? C7 ? ? ? ? ? ? 4C"), true, 1) : FindPattern(_("E8 ? ? ? ? B9 ? ? ? ? ? 89 ? ? E8 ? ? ? ? ? ? ? 48 85 C0 74 ?"), true, 1));

	//Better get the address from these calls rather than using 5 different patterns
	FNameToString = decltype(FNameToString)(FindPattern(_("E8 ? ? ? ? F3 41 0F 10 06 48 8D 15 ? ? ? ? F3 0F 59 05 ? ? ? ? 48 8B CF"), true, 1));

	FreeInternal = decltype(FreeInternal)(FindPattern(_("E8 ? ? ? ? 48 8B 0F 48 89 0B 4C 89 37 8B 47 08 89 43 08 8B 47 0C 89 43 0C 4C 89 77 08"), true, 1));
	
	//This is the fix to arrays behaving "bad", such as causing crashes when attempting to go back to lobby
	//or freezing the game when adding to inventory in S9+ 
	Realloc = decltype(Realloc)(FindPattern(_("E8 ? ? ? ? 48 89 03 48 8B 5C 24 ? 48 83 C4 20"), true, 1));

	//Initialize hardcoded offsets 
	switch ((int)(stod(GetEngineVersion().ToString().substr(0, 4)) * 100))
	{
	case 422:
	case 423:
	case 424: {
		offsets::Children = 0x48;
		offsets::Next = 0x28;
		offsets::ParamsSize = 0x9E;
		offsets::ReturnValueOffset = 0xA0;
		offsets::SuperClass = 0x40;
		offsets::StructSize = 0x50;
		break;
	}
	case 425: {
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

	printf(_("EraV2 - discord.gg/erafn"));

	FreeConsole();

	GameStatics = FindObject(_(L"/Script/Engine.Default__GameplayStatics"));
	kismetMathLib = FindObject(_(L"/Script/Engine.Default__KismetMathLibrary"));
	kismetGuidLib = FindObject(_(L"/Script/Engine.Default__KismetGuidLibrary"));
	kismetStringLib = FindObject(_(L"/Script/Engine.Default__KismetStringLibrary"));
	kismetSystemLib = FindObject(_(L"/Script/Engine.Default__KismetSystemLibrary"));
	DataTableFunctionLibrary = FindObject(_(L"/Script/Engine.Default__DataTableFunctionLibrary"));

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
	CheatManager = GameStatics->Call<UObject*>(_("SpawnObject"), FindObject(_(L"/Script/FortniteGame.FortCheatManager")), PlayerController);
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
	Athena::SpawnBuildPreviews();
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

	//Fix for dataTables
	Hook(LPVOID(FindPattern(_("E8 ? ? ? ? B9 ? ? ? ? ? 89 ? ? E8 ? ? ? ? ? ? ? 48 85 C0 74 ? C7 ? ? ? ? ? ? C7 ? ? ? ? ? ? 4C"), true, 1) > 0 ? FindPattern(_("E8 ? ? ? ? B9 ? ? ? ? ? 89 ? ? E8 ? ? ? ? ? ? ? 48 85 C0 74 ? C7 ? ? ? ? ? ? C7 ? ? ? ? ? ? 4C"), true, 1) : FindPattern(_("E8 ? ? ? ? B9 ? ? ? ? ? 89 ? ? E8 ? ? ? ? ? ? ? 48 85 C0 74 ?"), true, 1)), (LPVOID*)(&CopyScriptStruct), CopyScriptStructHook);

	Athena::SpawnFloorLoot();
}


