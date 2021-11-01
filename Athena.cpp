#include "Athena.h"
#include "memory.h"
#include "redirect.h"
#include "sdk.h"
#include "core.h"

void Athena::SpawnPawn()
{
	Pawn = Core::SpawnActorEasy(FindObject(_(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C")), FVector(0,0,5000));
	PlayerController->Call(_("Possess"), Pawn);
}

void Athena::ShowSkin()
{
	static auto BodyClass = FindObject(_(L"/Script/FortniteGame.CustomCharacterBodyPartData"));
	static auto HeadClass = FindObject(_(L"/Script/FortniteGame.CustomCharacterHeadData"));
	static auto HatData = FindObject(_(L"/Script/FortniteGame.CustomCharacterHatData"));
	static auto BackpackData = FindObject(_(L"/Script/FortniteGame.CustomCharacterBackpackData"));

	static auto CharacterParts = PlayerController->Child(_("StrongMyHero"))
		->Child<TArray<UObject*>>(_("CharacterParts"));

	if (CharacterPartsArray.size() == 0)
		for (auto i = 0; i < CharacterParts.count; i++) CharacterPartsArray.push_back(CharacterParts[i]);

	for (auto i = 0; i < CharacterPartsArray.size(); i++)
	{
		if (CharacterPartsArray[i]->Child(_("AdditionalData"))->IsA(BodyClass))
			Pawn->Call(_("ServerChoosePart"), char(1), CharacterPartsArray[i]);
		else if (CharacterPartsArray[i]->Child(_("AdditionalData"))->IsA(HeadClass))
			Pawn->Call(_("ServerChoosePart"), char(0), CharacterPartsArray[i]);
		else if (CharacterPartsArray[i]->Child(_("AdditionalData"))->IsA(HatData))
			Pawn->Call(_("ServerChoosePart"), char(2), CharacterPartsArray[i]);
		else if (CharacterPartsArray[i]->Child(_("AdditionalData"))->IsA(BackpackData))
			Pawn->Call(_("ServerChoosePart"), char(3), CharacterPartsArray[i]);
	}

	PlayerController->Child(_("PlayerState"))->Call(_("OnRep_CharacterParts"));
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
	if (auto MAP = &(GameState->Child<FSlateBrush>(_("AircraftPathBrush"))); !IsBadReadPtr(MAP)) (*MAP).ObjectResource = WorldSettings->Child<FSlateBrush>(_("AircraftPathBrush")).ObjectResource;

	GameState->Child<FSlateBrush>(_("MinimapBackgroundBrush")).ObjectResource = WorldSettings->Child<FSlateBrush>(_("AthenaMapImage")).ObjectResource;

	//Playlist
	auto PlayList = FindObject(_(L"/Game/Athena/Playlists/Playground/Playlist_Playground.Playlist_Playground"));
	if (PlayList) {
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
	static auto Size = *(int32*)(int64(FindObject(_(L"/Script/FortniteGame.FortItemEntry"))) + offsets::StructSize);

	auto ItemInstance = item->Call<UObject*>(_("CreateTemporaryItemInstanceBP"), Count, 1);
	WorldInventory->Child<TArray<UObject*>>(_("ItemInstances")).Add(ItemInstance);

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
	}

	Quickbars->Call(_("ServerAddItemInternal"), ItemInstance->Call<FGuid>(_("GetItemGuid")), Index, Slot);
}

void Athena::InitializeInventory()
{
	WorldInventory = PlayerController->Child(_("WorldInventory"));

	if (auto QB = &PlayerController->Child(_("ClientQuickBars")); !IsBadReadPtr(QB)) Quickbars = *QB;
	else {
		Quickbars = Core::SpawnActorEasy(FindObject(_(L"/Script/FortniteGame.FortQuickBars")), FVector(0, 0, 0));
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
	
	Athena::InventoryUpdate();
}

void Athena::OnServerExecuteInventoryItem(FGuid ItemGuid)
{
	auto Instances = WorldInventory->Child<TArray<UObject*>>(_("ItemInstances"));
	for (int i = 0; i < Instances.count; i++) {
		if (kismetGuidLib->Call<bool>(_("EqualEqual_GuidGuid"), Instances[i]->Call<FGuid>(_("GetItemGuid")), ItemGuid)) 
			Pawn->Call<UObject*>(_("EquipWeaponDefinition"), Instances[i]->Call<UObject*>(_("GetItemDefinitionBP")), Instances[i]->Call<FGuid>(_("GetItemGuid")));
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
}

void Athena::OnAircraftJump()
{
	auto Location = PlayerController->Call<FVector>(_("GetFocalLocation")); 

	Pawn = Core::SpawnActorEasy(FindObject(_(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C")), Location);

	PlayerController->Call(_("Possess"), Pawn);

	Pawn->Call(_("OnRep_CustomizationLoadout"));

	Athena::ShowSkin();

	bDroppedFromAircraft = true;
}