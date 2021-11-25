#include "Athena.h"
#include "memory.h"
#include "redirect.h"
#include "sdk.h"
#include "core.h"
#include "peh.h"

void Athena::SpawnPawn()
{
	Pawn = Core::SpawnActorEasy(FindObject(_(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C")), FVector(0,0,5000), FRotator(0,0,0));
	PlayerController->Call(_("Possess"), Pawn);
}

void Athena::ShowSkin()
{
	static UObject* Hero = nullptr;

	if (auto TempHero = &PlayerController->Child(_("MyPlayerInfo"))->Child(_("AthenaMenuHeroDef")); !IsBadReadPtr(TempHero)) Hero = *TempHero;
	if (auto TempHero = &PlayerController->Child(_("MyPlayerInfo"))->Child(_("TempAthenaMenuHeroInstance")); !IsBadReadPtr(TempHero)) Hero = *TempHero;

	static auto CharacterParts = Hero->Child<TArray<UObject*>>(_("CharacterParts"));

	if (CharacterPartsArray.size() == 0)
		for (UObject* CurrentCharacterPart : CharacterParts) CharacterPartsArray.push_back(CurrentCharacterPart);

	for (UObject* CurrentCharacterPart : CharacterPartsArray)
		Pawn->Call(_("ServerChoosePart"), CurrentCharacterPart->Child<char>(_("CharacterPartType")), CurrentCharacterPart);

	PlayerController->Child(_("PlayerState"))->Call(_("OnRep_CharacterParts"));
	PlayerController->Child(_("PlayerState"))->Call(_("OnRep_CharacterData"));
}

void Athena::DestroyLods()
{
	CheatManager->Call(_("DestroyAll"), FindObject(_(L"/Script/FortniteGame.FortHLODSMActor")));
}

void Athena::DropLoadingScreen()
{
	GameState->Child<char>(_("GamePhase")) = 2;
	GameState->Call(_("OnRep_GamePhase"), char(0));
	PlayerController->Call(_("ServerReadyToStartMatch"));

	PlayerController->Child<bool>(_("bHasServerFinishedLoading")) = true;
	PlayerController->Child<bool>(_("bHasClientFinishedLoading")) = true;

	//Bar
	PlayerController->Child(_("PlayerState"))->Call(_("OnRep_SquadId"));
	PlayerController->Child(_("PlayerState"))->Child<char>(_("TeamIndex")) = char(2);
	PlayerController->Child(_("PlayerState"))->Call(_("OnRep_TeamIndex"));

	//MINIMAP
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("MinimapNextCircleBrush"))); !IsBadReadPtr(MAP)) *MAP = {};
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("FullMapNextCircleBrush"))); !IsBadReadPtr(MAP)) *MAP = {};
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("MinimapSafeZoneBrush"))); !IsBadReadPtr(MAP)) *MAP = {};
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("MinimapCircleBrush"))); !IsBadReadPtr(MAP)) *MAP = {};
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("FullMapCircleBrush"))); !IsBadReadPtr(MAP)) *MAP = {};
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("AircraftPathBrush"))); !IsBadReadPtr(MAP)) (*MAP).ObjectResource = IsBadReadPtr(&WorldSettings->Child<FSlateBrush>(_("AircraftPathBrush"))) ? nullptr : WorldSettings->Child<FSlateBrush>(_("AircraftPathBrush")).ObjectResource;
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("MinimapBackgroundBrush"))); !IsBadReadPtr(MAP)) (*MAP).ObjectResource = IsBadReadPtr(&WorldSettings->Child<FSlateBrush>(_("AthenaMapImage"))) ? nullptr : WorldSettings->Child<FSlateBrush>(_("AthenaMapImage")).ObjectResource;
	if (auto MAP = &(GameState->Child<UObject*>(_("MinimapMaterial"))); !IsBadReadPtr(MAP)) (*MAP) = FindObject(_(L"/Game/Athena/Apollo/Maps/UI/M_MiniMapApollo.M_MiniMapApollo")) ? FindObject(_(L"/Game/Athena/Apollo/Maps/UI/M_MiniMapApollo.M_MiniMapApollo")) : FindObject(_(L"/Game/Athena/HUD/MiniMap/M_MiniMapAthena.M_MiniMapAthena"));

	//Playlist
	auto PlayList = FindObject(_(L"/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2"));
	if (!PlayList) PlayList = FindObject(_(L"/Game/Athena/Playlists/Playground/Playlist_Playground.Playlist_Playground"));
	if (PlayList) {
		PlayList->Child<bool>(_("bIsLargeTeamGame")) = true;
		if (auto HUDElementsToHide = &PlayList->Child<FGameplayTagContainer>(_("HUDElementsToHide")); !IsBadReadPtr(HUDElementsToHide)) HUDElementsToHide->GameplayTags.Add(kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"HUD.Athena.PlayersLeft"))));

		if (auto BasePlaylist = &GameState->Child<UObject*>(_("BasePlaylist")); !IsBadReadPtr(BasePlaylist)) *BasePlaylist = PlayList;
		if (auto OverridePlaylist = &GameState->Child<UObject*>(_("OverridePlaylist")); !IsBadReadPtr(OverridePlaylist)) *OverridePlaylist = PlayList;
		if (auto CurrentPlaylistData = &GameState->Child<UObject*>(_("CurrentPlaylistData")); !IsBadReadPtr(CurrentPlaylistData)) *CurrentPlaylistData = PlayList;
		GameState->Call(_("OnRep_CurrentPlaylistData"));
		GameState->Call(_("OnRep_CurrentPlaylistInfo"));
	}
}

void Athena::InventoryUpdate()
{
	PlayerController->Call(_("HandleWorldInventoryLocalUpdate"));
	WorldInventory->Call(_("HandleInventoryLocalUpdate"));
	PlayerController->Call(_("OnRep_QuickBar"));
	Quickbars->Call(_("OnRep_SecondaryQuickBar"));
	Quickbars->Call(_("OnRep_PrimaryQuickBar"));
	PlayerController->Call(_("ClientForceUpdateQuickbar"), char(0));
	PlayerController->Call(_("ClientForceUpdateQuickbar"), char(1));
}

void Athena::AddToInventory(class UObject* item, int Count, char Index, int Slot)
{
	if (IsBadReadPtr(item)) return;

	static auto Size = *(int32*)(int64(FindObject(_(L"/Script/FortniteGame.FortItemEntry"))) + offsets::StructSize);

	auto ItemInstance = item->Call<UObject*>(_("CreateTemporaryItemInstanceBP"), Count, 1);
	WorldInventory->Child<TArray<UObject*>>(_("ItemInstances")).Add(ItemInstance);

	ItemInstance->Call(_("SetOwningControllerForTemporaryItem"), PlayerController);

	switch (Size)
	{
	case 0xA8: {
		struct ItemEntry { char pad[0xA8]; };
		WorldInventory->Child<TArray<ItemEntry>>(_("ReplicatedEntries")).Add(ItemInstance->Child<ItemEntry>(_("ItemEntry")));
		break;
	}
	case 0xB0: {
		struct ItemEntry { char pad[0xB0]; };
		WorldInventory->Child<TArray<ItemEntry>>(_("ReplicatedEntries")).Add(ItemInstance->Child<ItemEntry>(_("ItemEntry")));
		break;
	}
	case 0xC0: {
		struct ItemEntry { char pad[0xC0]; };
		WorldInventory->Child<TArray<ItemEntry>>(_("ReplicatedEntries")).Add(ItemInstance->Child<ItemEntry>(_("ItemEntry")));
		break;
	}
	case 0xC8: {
		struct ItemEntry { char pad[0xC8]; };
		WorldInventory->Child<TArray<ItemEntry>>(_("ReplicatedEntries")).Add(ItemInstance->Child<ItemEntry>(_("ItemEntry")));
		break;
	}
	case 0xD0: {
		struct ItemEntry { char pad[0xD0]; };
		WorldInventory->Child<TArray<ItemEntry>>(_("ReplicatedEntries")).Add(ItemInstance->Child<ItemEntry>(_("ItemEntry")));
		break;
	}
	case 0x120: {
		struct ItemEntry { char pad[0x120]; };
		WorldInventory->Child<TArray<ItemEntry>>(_("ReplicatedEntries")).Add(ItemInstance->Child<ItemEntry>(_("ItemEntry")));
		break;
	}
	case 0x150: {
		struct ItemEntry { char pad[0x150]; };
		WorldInventory->Child<TArray<ItemEntry>>(_("ReplicatedEntries")).Add(ItemInstance->Child<ItemEntry>(_("ItemEntry")));
		break;
	}
	}

	Quickbars->Call(_("ServerAddItemInternal"), ItemInstance->Call<FGuid>(_("GetItemGuid")), Index, Slot);
}

void Athena::InitializeInventory()
{
	WorldInventory = PlayerController->Child(_("WorldInventory"));

	if (auto QB = &PlayerController->Child(_("ClientQuickBars")); !IsBadReadPtr(QB)) Quickbars = *QB;
	else {
		Quickbars = Core::SpawnActorEasy(FindObject(_(L"/Script/FortniteGame.FortQuickBars")), FVector(0, 0, 0), FRotator(0,0,0));
		PlayerController->Child(_("QuickBars")) = Quickbars;
	}

	Quickbars->Call(_("SetOwner"), PlayerController);
	WorldInventory->Call(_("SetOwner"), PlayerController);

	auto StartingItems = GameMode->Child<TArray<FItemAndCount>>(_("StartingItems"));

	AddToInventory(PlayerController->Child(_("Pickaxe"))->Child(_("WeaponDefinition")), 1, char(0), 0);
	AddToInventory(StartingItems[0].Item, 1, char(1), 0);
	AddToInventory(StartingItems[1].Item, 1, char(1), 1);
	AddToInventory(StartingItems[2].Item, 1, char(1), 2);
	AddToInventory(StartingItems[3].Item, 1, char(1), 3);
	AddToInventory(StartingItems[5].Item, 999, char(3), 0);
	AddToInventory(FindObject(_(L"/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells")), 999, char(3), 0);
	AddToInventory(FindObject(_(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium")), 999, char(3), 0);
	AddToInventory(FindObject(_(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight")), 999, char(3), 0);
	AddToInventory(FindObject(_(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy")), 999, char(3), 0);
	AddToInventory(FindObject(_(L"/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets")), 999, char(3), 0);
	AddToInventory(FindObject(_(L"/Game/Athena/Items/Traps/TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena")), 1, char(3), 0);
	AddToInventory(FindObject(_(L"/Game/Athena/Items/Traps/TID_Floor_Spikes_Athena_R_T03.TID_Floor_Spikes_Athena_R_T03")), 1, char(3), 0);
	
	EditToolItem = StartingItems[4].Item;
	
	Athena::InventoryUpdate();
}

void Athena::OnServerExecuteInventoryItem(FGuid ItemGuid)
{
	auto Instances = WorldInventory->Child<TArray<UObject*>>(_("ItemInstances"));
	for (UObject* Instance : Instances) {
		if (kismetGuidLib->Call<bool>(_("EqualEqual_GuidGuid"), Instance->Call<FGuid>(_("GetItemGuid")), ItemGuid)) {
			auto CurrentItemDefinition = Instance->Call<UObject*>(_("GetItemDefinitionBP"));

			if (CurrentItemDefinition->IsA(FindObject(_(L"/Script/FortniteGame.FortTrapItemDefinition"))))
			{
				Pawn->Call<bool>(_("PickUpActor"), nullptr, CurrentItemDefinition);
				Pawn->Child(_("CurrentWeapon"))->Child<FGuid>(_("ItemEntryGuid")) = Instance->Call<FGuid>(_("GetItemGuid"));
			}
			else Pawn->Call<UObject*>(_("EquipWeaponDefinition"), CurrentItemDefinition, Instance->Call<FGuid>(_("GetItemGuid")));
		}
	}
}

void Athena::OnServerExecuteInventoryWeapon(UObject* FortWeapon)
{
	auto ItemGuid = FortWeapon->Child<FGuid>(_("ItemEntryGuid"));

	auto Instances = WorldInventory->Child<TArray<UObject*>>(_("ItemInstances"));
	for (UObject* Instance : Instances) {
		if (kismetGuidLib->Call<bool>(_("EqualEqual_GuidGuid"), Instance->Call<FGuid>(_("GetItemGuid")), ItemGuid))
			Pawn->Call<UObject*>(_("EquipWeaponDefinition"), Instance->Call<UObject*>(_("GetItemDefinitionBP")), Instance->Call<FGuid>(_("GetItemGuid")));
	}
}

void Athena::RemoveNetDebugUI()
{
	FindObject(_(L"/Script/UMG.Default__WidgetBlueprintLibrary"))->Call<TArray<UObject*>, 0x8>(_("GetAllWidgetsOfClass"), World, TArray<UObject*>(), FindObject(_(L"/Game/Athena/HUD/NetDebugUI.NetDebugUI_C")), false)[0]->Call(_("RemoveFromViewport"));
}

void Athena::TeleportToSpawnIsland()
{
	auto Array = GameStatics->Call<TArray<UObject*>, 0x10>(_("GetAllActorsOfClass"), World, FindObject(_(L"/Script/FortniteGame.FortPlayerStartWarmup")), TArray<UObject*>());
	
	Pawn->Call(_("K2_TeleportTo"), Array[kismetMathLib->Call(_("RandomIntegerInRange"), 0, Array.MaxIndex())]->Call<FVector>(_("K2_GetActorLocation")), FRotator(0,0,0));
}

void Athena::ConsoleKey()
{
	auto F2Key = FKey();

	F2Key.KeyName = kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"F6")));

	FindObject(_(L"/Script/Engine.Default__InputSettings"))->Child<TArray<FKey>>(_("ConsoleKeys"))[1] = F2Key;
}

void Athena::CheatScript(const char* script)
{
	string ScriptFullName = string(script);
	
	transform(ScriptFullName.begin(), ScriptFullName.end(), ScriptFullName.begin(), ::tolower);

	if (ScriptFullName.find(_("spawnrift")) != -1)
	{
		if (!Core::SpawnActorEasy(FindObject(_(L"/Game/Athena/Items/Consumables/RiftItem/BGA_RiftPortal_Item_Athena.BGA_RiftPortal_Item_Athena_C")), Pawn->Call<FVector>(_("K2_GetActorLocation")), FRotator(0, 0, 0)))
			GameMode->Call(_("Say"), FString(_(L"Rifts arent in-game on the version of fortnite you are currently using.")));
	}
	else if (ScriptFullName.find(_("spawnweapon")) != -1)
	{
		string WeaponName = ScriptFullName.substr(ScriptFullName.find(_(" ")) + 1);
		wstring Path = _(L"/Game/Athena/Items/Weapons/");
		Path.append(wstring(WeaponName.begin(), WeaponName.end())).append(_(L".")).append(wstring(WeaponName.begin(), WeaponName.end()));
		auto ItemDefinition = FindObject(Path.c_str());

		if (ItemDefinition) SpawnPickup(ItemDefinition, 1, Pawn->Call<FVector>(_("K2_GetActorLocation")));
	}
	else if (ScriptFullName.find(_("startevent")) != -1)
	{
		bool bEventStarted = false;

		auto CattusDoggus = FindObject(_(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.BP_CattusDoggus_Scripting_2"));
		auto NightNight = FindObject(_(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.BP_NightNight_Scripting_2"));
		auto RocketEvent = FindObject(_(L"Athena_Gameplay_Geode.PersistentLevel.LevelSequence_LaunchRocket.AnimationPlayer"));
		auto CycloneJerky = FindObjectFromGObj(_("/CycloneJerky/Levels/JerkyLoaderLevel.JerkyLoaderLevel.PersistentLevel.BP_Jerky_Loader_2"));

		if (CattusDoggus)
		{
			CattusDoggus->Call<bool>(_("LoadCattusLevel"));
			CattusDoggus->Call(_("startevent"));

			bEventStarted = !bEventStarted;
		}
		else if (RocketEvent)
		{
			RocketEvent->Call(_("Play"));

			bEventStarted = !bEventStarted;
		}
		else if (NightNight)
		{
			NightNight->Call<bool>(_("LoadNightNightLevel"));
			NightNight->Call(_("startevent"));

			bEventStarted = !bEventStarted;
		}
		else if (CycloneJerky)
		{
			CycloneJerky->Call<bool>(_("LoadJerkyLevel"));
			CycloneJerky->Call(_("startevent"));

			bEventStarted = !bEventStarted;
		}

		if (bEventStarted) GameMode->Call(_("Say"), FString(_(L"The event started successfully. Enjoy!")));
	}
}

void Athena::ServerHandlePickup(UObject* Pickup)
{
	UObject* ItemDefinition = Pickup->Child(_("ItemDefinition"));
	int Count = Pickup->Child<int>(_("Count"));

	auto InventoryContext = FindObject(_(L"/Script/BlueprintContext.Default__BlueprintContextLibrary"))->Call<UObject*>(_("GetContext"), GameViewportClient->Child(_("GameInstance"))->Child<TArray<UObject*>>(_("LocalPlayers"))[0], FindObject(_(L"/Script/FortniteUI.FortInventoryContext")));

	for (int i = 0; i < 6; ++i)
	{
		if (!InventoryContext->Call<UObject*>(_("GetQuickBarSlottedItem"), char(0), i)) {
			AddToInventory(ItemDefinition, Count, char(0), i);
			Athena::InventoryUpdate();
			Pickup->Call(_("K2_DestroyActor"));
			break;
		}
	}
}

void Athena::FixBuildingFoundations()
{
	if ((GetEngineVersion().ToString().substr(34, 4).find(_("6.")) == -1) &&
		(GetEngineVersion().ToString().substr(34, 4).find(_("7.")) == -1) &&
		(GetEngineVersion().ToString().substr(34, 4).find(_("8.")) == -1)) return;

	auto Array = GameStatics->Call<TArray<UObject*>, 0x10>(_("GetAllActorsOfClass"), GameViewportClient->Child(_("World")), FindObject(_(L"/Script/FortniteGame.BuildingFoundation")), TArray<UObject*>());
	
	for (UObject* CurrentBuilding : Array) {
			CurrentBuilding->Child<char>(_("DynamicFoundationType")) = char(0);
	}
}

void Athena::Fixbus()
{
	string FNVersion = GetEngineVersion().ToString().substr(34, 4);

	if ((FNVersion.find(_("4.2")) != -1) ||
		(FNVersion.find(_("8.")) != -1) ||
		(FNVersion.find(_("9.")) != -1) ||
		(FNVersion.find(_("10.")) != -1) ||
		(FNVersion.find(_("11.")) != -1) ||
		(FNVersion.find(_("12.")) != -1)) {
		GameState->Child<char>(_("GamePhase")) = 3;
		GameState->Call(_("OnRep_GamePhase"), char(2));

		Pawn->Call<bool>(_("K2_TeleportTo"), FVector(0, 0, 22500), FRotator(0, 0, 0));
		
		Pawn->Child<bool>(_("bIsSkydiving")) = true;
		Pawn->Call(_("OnRep_IsSkydiving"), false);
	}
}

void Athena::Loot()
{
	//Basic looting impl
}

void Athena::OnServerCreateBuildingActor(PVOID Params)
{
	struct OriginalParams
	{
		UObject* Class;
		char pad[0x8];
		FVector BuildingLocation;
		FRotator BuildingRotation;
	};

	struct NewParams
	{
		int ClassHandle;
		FVector BuildingLocation;
		FRotator BuildingRotation;
		char pad[0xC];
		UObject* Class;
	};

	if (FindObject(_(L"/Script/FortniteGame.FortPlayerController.ServerCreateBuildingActor"))->GetFunctionChildrenOffset().size() == 1)
	{
		auto IParams = *(NewParams*)(Params);
		auto BuildingActor = Core::SpawnActorEasy(IParams.Class, IParams.BuildingLocation, IParams.BuildingRotation);
		if (FindObject(_(L"/Script/FortniteGame.BuildingActor.InitializeKismetSpawnedBuildingActor"))->GetFunctionChildrenOffset().size() == 3) BuildingActor->Call(_("InitializeKismetSpawnedBuildingActor"), BuildingActor, PlayerController, true);
		else BuildingActor->Call(_("InitializeKismetSpawnedBuildingActor"), BuildingActor, PlayerController);
	}
	else
	{
		auto IParams = *(OriginalParams*)(Params);
		auto BuildingActor = Core::SpawnActorEasy(IParams.Class, IParams.BuildingLocation, IParams.BuildingRotation);
		if (FindObject(_(L"/Script/FortniteGame.BuildingActor.InitializeKismetSpawnedBuildingActor"))->GetFunctionChildrenOffset().size() == 3) BuildingActor->Call(_("InitializeKismetSpawnedBuildingActor"), BuildingActor, PlayerController, true);
		else BuildingActor->Call(_("InitializeKismetSpawnedBuildingActor"), BuildingActor, PlayerController);
	}
}

void Athena::OnBeginEditActor(UObject* BuildingPiece)
{
	auto WeaponEditActor = Pawn->Call<UObject*>(_("EquipWeaponDefinition"), EditToolItem, EditToolItem->Call<UObject*>(_("CreateTemporaryItemInstanceBP"), 1, 1)->Call<FGuid>(_("GetItemGuid")));

	WeaponEditActor->Child(_("EditActor")) = BuildingPiece;
	WeaponEditActor->Call(_("OnRep_EditActor"));
}

void Athena::OnFinishEditActor(UObject* BuildingActor, UObject* NewClass, int RotationIteration, bool bMirrored, PVOID Params)
{
	struct FixedParams
	{
		UObject* BuildingActor;
		UObject* NewClass;
		int8 RotationIteration;
		bool bMirrored;
	};

	auto FixedParamsInstance = *(FixedParams*)(Params);

	BuildingActor->Child<bool>(_("bPlayDestructionEffects")) = false;

	BuildingActor->Call(_("SetActorScale3D"), FVector(0, 0, 0));
	BuildingActor->Call(_("K2_DestroyActor"));

	auto EditedActor = Core::SpawnActorEasy(NewClass, EditComponentLocation, EditComponentRotation);
	auto Offset = FindObject(_(L"/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor"))->GetFunctionChildrenOffset()[3];
	if (Offset != 0x14 &&
		Offset > -1) {
		EditedActor->Call(_("SetMirrored"), FixedParamsInstance.bMirrored);
	}
	else EditedActor->Call(_("SetMirrored"), bMirrored);

	if (FindObject(_(L"/Script/FortniteGame.BuildingActor.InitializeKismetSpawnedBuildingActor"))->GetFunctionChildrenOffset().size() == 3) EditedActor->Call(_("InitializeKismetSpawnedBuildingActor"), EditedActor, PlayerController, true);
	else EditedActor->Call(_("InitializeKismetSpawnedBuildingActor"), EditedActor, PlayerController);
}

void Athena::SpawnPickup(UObject* ItemDefinition, int Count, FVector Location)
{
	auto Pickup = Core::SpawnActorEasy(FindObject(_(L"/Script/FortniteGame.FortPickupAthena")), Location, FRotator(0,0,0));

	Pickup->Child(_("ItemDefinition")) = ItemDefinition;
	Pickup->Child<int>(_("Count")) = Count;
	Pickup->Call(_("OnRep_PrimaryPickupItemEntry"));
	Pickup->Call(_("TossPickup"), Location, Pawn, 1, true);
}

void Athena::GrantAbility(UObject* Class)
{
	static UObject* GameplayEffect = FindObject(_(L"/Game/Athena/Items/Consumables/PurpleStuff/GE_Athena_PurpleStuff.GE_Athena_PurpleStuff_C"));
	if (!GameplayEffect) GameplayEffect = FindObject(_(L"/Game/Athena/Items/Consumables/PurpleStuff/GE_Athena_PurpleStuff_Health.GE_Athena_PurpleStuff_Health_C"));

	auto DefaultEffect = FindObject(_(L"/Script/FortniteGame.Default__FortAbilitySystemUI"))->Call<UObject*>(_("GetDefaultObjectOfGameplayEffectType"), GameplayEffect);

	if (Class) DefaultEffect->Child<TArray<FGameplayAbilitySpecDef>>(_("GrantedAbilities"))[0].Ability = Class;
	DefaultEffect->Child<char>(_("DurationPolicy")) = char(1);

	Pawn->Child(_("AbilitySystemComponent"))->Call<FActiveGameplayEffectHandle>(_("BP_ApplyGameplayEffectToSelf"), GameplayEffect, float(1.0), FGameplayEffectContextHandle());
}

void Athena::GrantDefaultAbilities()
{
	GrantAbility(FindObject(_(L"/Script/FortniteGame.FortGameplayAbility_Sprint")));
	GrantAbility(FindObject(_(L"/Script/FortniteGame.FortGameplayAbility_Jump")));
	GrantAbility(FindObject(_(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GA_DefaultPlayer_InteractUse.GA_DefaultPlayer_InteractUse_C")));
	GrantAbility(FindObject(_(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GA_DefaultPlayer_InteractSearch.GA_DefaultPlayer_InteractSearch_C")));
	GrantAbility(FindObject(_(L"/Game/Athena/DrivableVehicles/GA_AthenaEnterVehicle.GA_AthenaEnterVehicle_C")));
	GrantAbility(FindObject(_(L"/Game/Athena/DrivableVehicles/GA_AthenaExitVehicle.GA_AthenaExitVehicle_C")));
	GrantAbility(FindObject(_(L"/Game/Athena/DrivableVehicles/GA_AthenaInVehicle.GA_AthenaInVehicle_C")));
	GrantAbility(FindObject(_(L"/Game/Athena/Items/ForagedItems/Rift/GA_Rift_Athena_Skydive.GA_Rift_Athena_Skydive_C")));
	GrantAbility(FindObject(_(L"/Game/Athena/Environments/Blueprints/SurfaceEffects/GAB_SurfaceChange.GAB_SurfaceChange_C")));
}

void Athena::OnAircraftJump()
{
	auto Location = PlayerController->Call<FVector>(_("GetFocalLocation")); 
	auto Rotation = PlayerController->Call<FRotator>(_("GetControlRotation")); 
	Rotation.Pitch = 0;
	Rotation.Roll = 0;

	Pawn = Core::SpawnActorEasy(FindObject(_(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C")), Location, Rotation);

	PlayerController->Call(_("Possess"), Pawn);

	Pawn->Call(_("OnRep_CustomizationLoadout"));

	Athena::ShowSkin();

	bDroppedFromAircraft = true;
}
