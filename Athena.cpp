#include "Athena.h"

#include <map>

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
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("AircraftPathBrush"))); !IsBadReadPtr(MAP)) (*MAP).ObjectResource = FindObject(_(L"/Game/Athena/HUD/MiniMap/M_BusPath.M_BusPath"));
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

	auto ItemInstance = item->Call<UObject*>(_("CreateTemporaryItemInstanceBP"), Count, 1);
	
	WorldInventory->Child<TArray<UObject*>>(_("ItemInstances")).Add(ItemInstance);
	WorldInventory->Child<TArray<char>>(_("ReplicatedEntries")).AddWithSize(*(int32*)(int64(FindObject(_(L"/Script/FortniteGame.FortItemEntry"))) + offsets::StructSize), &ItemInstance->Child<void*>(_("ItemEntry")));
	
	ItemInstance->Call(_("SetOwningControllerForTemporaryItem"), PlayerController);
		
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

	kismetSystemLib->Call(_("SetBoolPropertyByName"), PlayerController, kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"bInfiniteAmmo"))), true);
	kismetSystemLib->Call(_("SetBoolPropertyByName"), PlayerController, kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"bBuildFree"))), true);
	
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
			else if (CurrentItemDefinition->IsA(FindObject(_(L"AthenaGadgetItemDefinition"), false, true)) ||
				CurrentItemDefinition->IsA(FindObject(_(L"FortGadgetItemDefinition"), false, true)))
			{
				if (CurrentItemDefinition->GetName() == _("AGID_CarminePack"))
				{
					Pawn->Call(_("ServerChoosePart"), FindObject(_(L"Dev_TestAsset_Head_M_XL"), false, true)->Child<char>(_("CharacterPartType")), FindObject(_(L"Dev_TestAsset_Head_M_XL"), false, true));
					Pawn->Call(_("ServerChoosePart"), FindObject(_(L"Dev_TestAsset_Body_M_XL"), false, true)->Child<char>(_("CharacterPartType")), FindObject(_(L"Dev_TestAsset_Body_M_XL"), false, true));
				}
				
				PlayerController->Child(_("PlayerState"))->Call(_("OnRep_CharacterParts"));
				PlayerController->Child(_("PlayerState"))->Call(_("OnRep_CharacterData"));

				//Grant abilities
				auto AbilitySet = kismetSystemLib->Call<UObject*>(_("Conv_SoftObjectReferenceToObject"), CurrentItemDefinition->Child<SoftObjectPtr>(_("AbilitySet")));
				for (auto AbilityClass : AbilitySet->Child<TArray<UObject*>>(_("GameplayAbilities")))
					Athena::GrantAbility(AbilityClass);
				
				for (auto Effect : AbilitySet->Child<TArray<GEHard>>(_("GrantedGameplayEffects")))
					Athena::GrantEffect(Effect.Effect, Effect.Level);
				
				//set anim instance
				Pawn->Child(_("Mesh"))->Call(_("SetAnimInstanceClass"), CurrentItemDefinition->Child(_("AnimBPOverride")));

				Pawn->Call<UObject*>(_("EquipWeaponDefinition"), CurrentItemDefinition->Call<UObject*>(_("GetDecoItemDefinition")), Instance->Call<FGuid>(_("GetItemGuid")));
			}
			else
			{
				Pawn->Call<UObject*>(_("EquipWeaponDefinition"), CurrentItemDefinition, Instance->Call<FGuid>(_("GetItemGuid")));

				if (GetEngineVersion().ToString().find(_("4.19")) != -1)
				{
					if (CurrentItemDefinition == FindObject(_(L"BuildingItemData_Wall"), false, true))
					{
						PlayerController->Child(_("CurrentBuildableClass")) = FindObject(_(L"PBWA_W1_Solid_C"), false, true);
						PlayerController->Child(_("BuildPreviewMarker"))->Call(_("SetActorHiddenInGame"), true);
						Wall->Call(_("SetActorHiddenInGame"), false);
					}
					else if (CurrentItemDefinition == FindObject(_(L"BuildingItemData_Floor"), false, true))
					{
						PlayerController->Child(_("CurrentBuildableClass")) = FindObject(_(L"PBWA_W1_Floor_C"), false, true);
						PlayerController->Child(_("BuildPreviewMarker"))->Call(_("SetActorHiddenInGame"), true);
						Floor->Call(_("SetActorHiddenInGame"), false);
					}
					else if (CurrentItemDefinition == FindObject(_(L"BuildingItemData_Stair_W"), false, true))
					{
						PlayerController->Child(_("CurrentBuildableClass")) = FindObject(_(L"PBWA_W1_StairW_C"), false, true);
						PlayerController->Child(_("BuildPreviewMarker"))->Call(_("SetActorHiddenInGame"), true);
						Stairs->Call(_("SetActorHiddenInGame"), false);
					}
					else if (CurrentItemDefinition == FindObject(_(L"BuildingItemData_RoofS"), false, true))
					{
						PlayerController->Child(_("CurrentBuildableClass")) = FindObject(_(L"PBWA_W1_RoofC_C"), false, true);
						PlayerController->Child(_("BuildPreviewMarker"))->Call(_("SetActorHiddenInGame"), true);
						Roof->Call(_("SetActorHiddenInGame"), false);
					}

					if (CurrentItemDefinition->IsA(FindObject(_(L"FortBuildingItemDefinition"), false, true)))
					{
						auto CurrentResource =  PlayerController->Child<int8>(_("CurrentResourceType")); //copy 
					
						//not to fuck up the preview
						CheatManager->Call(_("BuildWith"), FString(_(L"Wood")));
						CheatManager->Call(_("BuildWith"), FString(_(L"Stone")));
						CheatManager->Call(_("BuildWith"), FString(_(L"Metal")));

						switch(CurrentResource)
						{
						case 0:
							{
								CheatManager->Call(_("BuildWith"), FString(_(L"Wood")));
								break;
							}
						case 1:
							{
								CheatManager->Call(_("BuildWith"), FString(_(L"Stone")));
								break;
							}
						case 2:
							{
								CheatManager->Call(_("BuildWith"), FString(_(L"Metal")));
								break;
							}
						}
					}
				}
			}
		}
	}
}

void Athena::OnServerExecuteInventoryWeapon(UObject* FortWeapon)
{
	auto ItemGuid = FortWeapon->Child<FGuid>(_("ItemEntryGuid"));

	auto Instances = WorldInventory->Child<TArray<UObject*>>(_("ItemInstances"));
	for (UObject* Instance : Instances)
	{
		if (kismetGuidLib->Call<bool>(_("EqualEqual_GuidGuid"), Instance->Call<FGuid>(_("GetItemGuid")), ItemGuid)) {
			Pawn->Call<UObject*>(_("EquipWeaponDefinition"), Instance->Call<UObject*>(_("GetItemDefinitionBP")), Instance->Call<FGuid>(_("GetItemGuid")));
		}
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
		auto ItemDefinition = FindObject(wstring(WeaponName.begin(), WeaponName.end()).c_str(), false, true);

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

	static auto InventoryContext = FindObject(_(L"/Script/BlueprintContext.Default__BlueprintContextLibrary"))->Call<UObject*>(_("GetContext"), GameViewportClient->Child(_("GameInstance"))->Child<TArray<UObject*>>(_("LocalPlayers"))[0], FindObject(_(L"/Script/FortniteUI.FortInventoryContext")));

	for (int i = 0; i < 6; ++i)
	{
		if (!InventoryContext->Call<UObject*>(_("GetQuickBarSlottedItem"), char(0), i)) {
			AddToInventory(ItemDefinition, Count, char(0), i);
			Athena::InventoryUpdate();
			Pickup->Call(_("K2_DestroyActor"));
			return;
		}
	}
	
	auto CurrentFocusedSlot = (&Quickbars->Child<Struct>(_("PrimaryQuickBar")))->Child<int>(FindObject(_(L"/Script/FortniteGame.QuickBar")), _("CurrentFocusedSlot"));
	if (CurrentFocusedSlot == 0) return;
	auto ItemInstance = InventoryContext->Call<UObject*>(_("GetQuickBarSlottedItem"), char(0), CurrentFocusedSlot);
	Athena::DropInventoryItem(ItemInstance->Call<FGuid>(_("GetItemGuid")), 1);
	AddToInventory(ItemDefinition, Count, char(0), CurrentFocusedSlot);
	Athena::InventoryUpdate();
	Pickup->Call(_("K2_DestroyActor"));
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

void Athena::Loot(UObject* ReceivingActor)
{
	//Basic looting impl

	if (strstr(ReceivingActor->Class->GetName().c_str(), _("Tiered_Chest")) &&
		ReceivingActor->Class->GetName().find(_("Creative")) == -1)
	{
		if (ReceivingActor->Child<float>(_("TimeUntilLootRegenerates")) == 0)
		{
			auto Result = Looting::PickLootDrops(kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"Loot_AthenaTreasure"))));

			for (auto Instance : Result)
			{
				auto RightVector = kismetMathLib->Call<FVector>(_("GetRightVector"), ReceivingActor->Call<FRotator>(_("K2_GetActorRotation")));
				RightVector.X = RightVector.X*64;
				RightVector.Y = RightVector.Y*64;
				auto FinalLocation = kismetMathLib->Call<FVector>(_("Add_VectorVector"), ReceivingActor->Call<FVector>(_("K2_GetActorLocation")), RightVector);
			
				SpawnPickup(Instance.ItemDefinition, Instance.Count, FinalLocation);
			}

			ReceivingActor->Child<float>(_("TimeUntilLootRegenerates")) = 1;
		}
	}

	kismetSystemLib->Call(_("SetBoolPropertyByName"), ReceivingActor, kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"bAlreadySearched"))), true);
	ReceivingActor->Call(_("OnRep_bAlreadySearched"));
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

void Athena::SpawnPickup(UObject* ItemDefinition, int Count, FVector Location, bool bToss)
{
	auto Pickup = Core::SpawnActorEasy(FindObject(_(L"/Script/FortniteGame.FortPickupAthena")), Location, FRotator(0,0,0));

	Pickup->Child(_("ItemDefinition")) = ItemDefinition;
	Pickup->Child<int>(_("Count")) = Count;
	Pickup->Call(_("OnRep_PrimaryPickupItemEntry"));
	Pickup->Call(_("TossPickup"), Location, Pawn, 1, bToss);
}

void Athena::GrantEffect(UObject* Effect, float Level)
{
	if (!Effect) return;
	
	Pawn->Child(_("AbilitySystemComponent"))->Call<FActiveGameplayEffectHandle>(_("BP_ApplyGameplayEffectToSelf"), Effect, Level, FGameplayEffectContextHandle());
}

void Athena::GrantAbility(UObject* Class)
{
	if (!Class) return;
	
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

	//Equip pickaxe for early build manually.
	static auto InventoryContext = FindObject(_(L"/Script/BlueprintContext.Default__BlueprintContextLibrary"))->Call<UObject*>(_("GetContext"), GameViewportClient->Child(_("GameInstance"))->Child<TArray<UObject*>>(_("LocalPlayers"))[0], FindObject(_(L"/Script/FortniteUI.FortInventoryContext")));
	auto Pickaxe = InventoryContext->Call<UObject*>(_("GetQuickBarSlottedItem"), char(0), 0);
	Pawn->Call<UObject*>(_("EquipWeaponDefinition"), Pickaxe->Call<UObject*>(_("GetItemDefinitionBP")), Pickaxe->Call<FGuid>(_("GetItemGuid")));
}

vector<Athena::Looting::LootData> Athena::Looting::PickLootDrops(FName Category)
{
	auto LootTierData = FindObject(_(L"/Game/Items/Datatables/AthenaLootTierData_Client.AthenaLootTierData_Client"), true);
	auto LootPackageTable = FindObject(_(L"/Game/Items/Datatables/AthenaLootPackages_Client.AthenaLootPackages_Client"), true);

	vector<LootData> ReturnValue;

	auto RowNames = DataTableFunctionLibrary->Call<TArray<FName>, 0x8>(_("GetDataTableRowNames"), LootTierData, TArray<FName>());

	//Row name and Weight
	map<string, Struct*> CategoryRowMap;
	map<string, Struct*> LootPackageCalls;
	float MaxWeight = 0.f;

	for (auto Name : RowNames)
	{
		Struct* RowPtr = (Struct*)malloc(*(int32*)(int64(LootTierData->Child(_("RowStruct"))) + offsets::StructSize));
		GetDataTableRow(LootTierData, Name, RowPtr);
		auto TierGroup = RowPtr->Child<FName>(LootTierData->Child(_("RowStruct")), _("TierGroup"));

		if (TierGroup.ToString() == Category.ToString())
			if (RowPtr->Child<float>(LootTierData->Child(_("RowStruct")), _("Weight")) != 0.f)
				CategoryRowMap.insert(make_pair(Name.ToString(), RowPtr));
	}

	for (auto const&[key, val] : CategoryRowMap) MaxWeight += val->Child<float>(LootTierData->Child(_("RowStruct")), _("Weight"));

	//Now pick a random Row from the datatable
	float RandomValue = kismetMathLib->Call<float>(_("RandomFloatInRange"), 0.f, MaxWeight);
	Struct* PickedRow = nullptr;

	for (auto const&[key, val] : CategoryRowMap)
	{
		if (RandomValue < val->Child<float>(LootTierData->Child(_("RowStruct")), _("Weight")))
		{
			PickedRow = val;
			break;
		}

		RandomValue -= val->Child<float>(LootTierData->Child(_("RowStruct")), _("Weight"));
	}

	if (!PickedRow) return {};

	//Now get the num of loot packages to spawn, find the item defs, counts, and return.
	float NumLootPackageDrops = PickedRow->Child<float>(LootTierData->Child(_("RowStruct")), _("NumLootPackageDrops"));
	string LootPackageName = PickedRow->Child<FName>(LootTierData->Child(_("RowStruct")), _("LootPackage")).ToString();

	for (auto const&[key, val] : CategoryRowMap) free(val);
	CategoryRowMap.clear();

	RowNames = DataTableFunctionLibrary->Call<TArray<FName>, 0x8>(_("GetDataTableRowNames"), LootPackageTable, TArray<FName>());

	for (auto Name : RowNames)
	{
		Struct* RowPtr = (Struct*)malloc(*(int32*)(int64(LootPackageTable->Child(_("RowStruct"))) + offsets::StructSize));
		GetDataTableRow(LootPackageTable, Name, RowPtr);

		if (RowPtr->Child<FName>(LootPackageTable->Child(_("RowStruct")), _("LootPackageID")).ToString() == LootPackageName)
		{
			CategoryRowMap.insert(make_pair(Name.ToString(), RowPtr)); //add the name of the row and the ptr to the map
		}
	}
	
	for (float i = 0; i < NumLootPackageDrops; i++)
	{
		auto Iterator = CategoryRowMap.begin();
		advance(Iterator, i);

		if (Iterator == CategoryRowMap.end()) break;

		if (LootPackageName.find(_(".Empty")) != -1) continue;
		
		if (Iterator->second->Child<FString>(LootPackageTable->Child(_("RowStruct")), _("LootPackageCall")).ToString().find(_(".Empty")) != -1)
		{
			NumLootPackageDrops++;
			continue;
		}
		else
		{
			map<string, Struct*> TempLootPackageCalls;
			float TempMaxWeight = 0.f;

			if (!strstr(LootPackageName.c_str(), _("WorldList")))
			{
				for (auto Name : RowNames)
				{
					Struct* RowPtr = (Struct*)malloc(*(int32*)(int64(LootPackageTable->Child(_("RowStruct"))) + offsets::StructSize));
					GetDataTableRow(LootPackageTable, Name, RowPtr);

					if (RowPtr->Child<FName>(LootPackageTable->Child(_("RowStruct")), _("LootPackageID")).ToString() == Iterator->second->Child<FString>(LootPackageTable->Child(_("RowStruct")), _("LootPackageCall")).ToString())
					{
						TempLootPackageCalls.insert(make_pair(Name.ToString(), RowPtr));
					}
				}
			}
			else
			{
				for (auto const&[key, val] : CategoryRowMap) TempLootPackageCalls.insert(make_pair(key, val));
			}

			for (auto const&[key, val] : TempLootPackageCalls) TempMaxWeight += val->Child<float>(LootPackageTable->Child(_("RowStruct")), _("Weight"));
			float TempRandomValue = kismetMathLib->Call<float>(_("RandomFloatInRange"), 0.f, TempMaxWeight);

			for (auto const&[key, val] : TempLootPackageCalls)
			{
				if (TempRandomValue < val->Child<float>(LootPackageTable->Child(_("RowStruct")), _("Weight")))
				{
					LootPackageCalls.insert(make_pair(key, val));
					break;
				}

				TempRandomValue -= val->Child<float>(LootPackageTable->Child(_("RowStruct")), _("Weight"));
				free(val);
			}
		}
	}

	for (auto const&[key, val] : LootPackageCalls)
	{
		auto ItemDefinition = kismetSystemLib->Call<UObject*>(_("Conv_SoftObjectReferenceToObject"), val->Child<SoftObjectPtr>(LootPackageTable->Child(_("RowStruct")), _("ItemDefinition")));
		ReturnValue.push_back(LootData{ItemDefinition, val->Child<int>(LootPackageTable->Child(_("RowStruct")), _("Count"))});
	}

	//before returning, clear the whole CategoryRowMap map and free everything.
	for (auto const&[key, val] : CategoryRowMap) free(val);
	CategoryRowMap.clear();

	for (auto const&[key, val] : LootPackageCalls) free(val);
	LootPackageCalls.clear();
	
	return ReturnValue;
}

void Athena::SpawnFloorLoot()
{
	auto FloorLootActors = GameStatics->Call<TArray<UObject*>, 0x10>(_("GetAllActorsOfClass"), World, FindObject(_(L"Tiered_Athena_FloorLoot_01_C"), false, true), TArray<UObject*>());

	for (auto element : FloorLootActors)
	{
		auto LootToSpawn = Looting::PickLootDrops(kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"Loot_AthenaFloorLoot"))));
		for (auto element_ : LootToSpawn) SpawnPickup(element_.ItemDefinition, element_.Count, element->Call<FVector>(_("K2_GetActorLocation")), false);
	}

	auto FloorLootActors_Warmup = GameStatics->Call<TArray<UObject*>, 0x10>(_("GetAllActorsOfClass"), World, FindObject(_(L"Tiered_Athena_FloorLoot_Warmup_C"), false, true), TArray<UObject*>());

	for (auto element : FloorLootActors_Warmup)
	{
		auto LootToSpawn = Looting::PickLootDrops(kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"Loot_AthenaFloorLoot_Warmup"))));
		for (auto element_ : LootToSpawn) SpawnPickup(element_.ItemDefinition, element_.Count, element->Call<FVector>(_("K2_GetActorLocation")), false);
	}
}

void Athena::SpawnBuildPreviews()
{
	if (GetEngineVersion().ToString().find(_("4.19")) != -1)
	{
		Wall = Core::SpawnActorEasy(FindObject(_(L"BuildingPlayerPrimitivePreview"), false, true), FVector(0,0,0), FRotator(0,0,0));
		Wall->Call<UObject*>(_("GetBuildingMeshComponent"))->Call<bool>(_("SetStaticMesh"), FindObject(_(L"PBW_W1_Solid"), false, true));
		Wall->Call<UObject*>(_("GetBuildingMeshComponent"))->Call(_("SetMaterial"), 0, PlayerController->Child(_("BuildPreviewMarkerMID")));
		Wall->Call(_("OnBuildingActorInitialized"), char(5), char(0));
		Wall->Call(_("SetActorHiddenInGame"), true);
	
		Floor = Core::SpawnActorEasy(FindObject(_(L"BuildingPlayerPrimitivePreview"), false, true), FVector(0,0,0), FRotator(0,0,0));
		Floor->Call<UObject*>(_("GetBuildingMeshComponent"))->Call<bool>(_("SetStaticMesh"), FindObject(_(L"PBW_W1_Floor"), false, true));
		Floor->Call<UObject*>(_("GetBuildingMeshComponent"))->Call(_("SetMaterial"), 0, PlayerController->Child(_("BuildPreviewMarkerMID")));
		Floor->Call(_("OnBuildingActorInitialized"), char(5), char(1));
		Floor->Call(_("SetActorHiddenInGame"), true);
	
		Stairs = Core::SpawnActorEasy(FindObject(_(L"BuildingPlayerPrimitivePreview"), false, true), FVector(0,0,0), FRotator(0,0,0));
		Stairs->Call<UObject*>(_("GetBuildingMeshComponent"))->Call<bool>(_("SetStaticMesh"), FindObject(_(L"PBW_W1_StairW"), false, true));
		Stairs->Call<UObject*>(_("GetBuildingMeshComponent"))->Call(_("SetMaterial"), 0, PlayerController->Child(_("BuildPreviewMarkerMID")));
		Stairs->Call(_("OnBuildingActorInitialized"), char(5), char(5));
		Stairs->Call(_("SetActorHiddenInGame"), true);
	
		Roof = Core::SpawnActorEasy(FindObject(_(L"BuildingPlayerPrimitivePreview"), false, true), FVector(0,0,0), FRotator(0,0,0));
		Roof->Call<UObject*>(_("GetBuildingMeshComponent"))->Call<bool>(_("SetStaticMesh"), FindObject(_(L"PBW_W1_RoofC"), false, true));
		Roof->Call<UObject*>(_("GetBuildingMeshComponent"))->Call(_("SetMaterial"), 0, PlayerController->Child(_("BuildPreviewMarkerMID")));
		Roof->Call(_("OnBuildingActorInitialized"), char(5), char(6));
		Roof->Call(_("SetActorHiddenInGame"), true);
	}
}

void Athena::PlayEmoteItem(UObject* MontageItemDefinition)
{
	if (!FindObject(_(L"GetAnimationHardReference"), false, true)) return;
	
	auto Montage = MontageItemDefinition->Call<UObject*>(_("GetAnimationHardReference"), char(6), char(3));
	if (!IsBadReadPtr(Montage) && Montage)
	{
		Pawn->Child(_("Mesh"))->Call<UObject*>(_("GetAnimInstance"))->Call<float>(_("Montage_Play"), Montage, 1.f, char(0), 0.f, true);
		bIsEmoting = true;
	}
}

void Athena::Tick()
{
	if (bIsEmoting && (PlayerController->Child<bool>(_("bIsPlayerActivelyMoving")) ||
		Pawn->Child(_("Mesh"))->Call<UObject*>(_("GetAnimInstance"))->Child<bool>(_("bIsJumping")) ||
		Pawn->Child(_("Mesh"))->Call<UObject*>(_("GetAnimInstance"))->Child<bool>(_("bIsFalling"))))
	{
		bIsEmoting = !bIsEmoting;
		Pawn->Call(_("ServerRootMotionInterruptNotifyStopMontage"), Pawn->Call<UObject*>(_("GetCurrentMontage")));
	}

	if (g_bIsVehicleVersion)
	{
		bool bIsInVehicle = Pawn->Call<bool>(_("IsInVehicle"));
		Pawn->Child<char>(_("Role")) = bIsInVehicle ? char(2) : char(3);
		if (bIsInVehicle) Pawn->Call<UObject*>(_("GetVehicle"))->Child<char>(_("Role")) = char(2);
	}

	if (Pawn)
	{
		if (bool* game_bIsOutsideSafezone = &Pawn->Child<bool>(_("bIsOutsideSafeZone"));
		!IsBadReadPtr(game_bIsOutsideSafezone))
		{
			if ((*game_bIsOutsideSafezone && !l_bIsOutsideSafeZone) ||
			(!*game_bIsOutsideSafezone && l_bIsOutsideSafeZone))
			{
				l_bIsOutsideSafeZone = !l_bIsOutsideSafeZone;
				Pawn->Call(_("OnRep_IsOutsideSafeZone"));
			}
		}
	}
}

void Athena::OnExitVehicle()
{
	Pawn->Child<char>(_("Role")) = char(3);
	Pawn->Call<UObject*>(_("GetVehicle"))->Child<char>(_("Role")) = char(3);
}

void Athena::DropInventoryItem(FGuid ItemGuid, int Count)
{
	auto Instances = WorldInventory->Child<TArray<UObject*>>(_("ItemInstances"));
	for (auto i = 0; i < Instances.count; i++)
	{
		if (kismetGuidLib->Call<bool>(_("EqualEqual_GuidGuid"), Instances[i]->Call<FGuid>(_("GetItemGuid")), ItemGuid)) {
			auto CurrentItemDef = Instances[i]->Call<UObject*>(_("GetItemDefinitionBP"));

			//remove
			GenericArray_Remove(&WorldInventory->Child<TArray<UObject*>>(_("ItemInstances")), ChildProperty(WorldInventory, _("ItemInstances")), i);
			GenericArray_Remove(&WorldInventory->Child<TArray<char>>(_("ReplicatedEntries")), ChildProperty(WorldInventory, _("ReplicatedEntries")), i);

			//update inventory
			Athena::InventoryUpdate();

			//drop the removed weapon
			Athena::SpawnPickup(CurrentItemDef, Count, Pawn->Call<FVector>(_("K2_GetActorLocation")));
		}
	}
}
