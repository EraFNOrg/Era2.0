#pragma once

#include <windows.h>
#include <vector>

class Athena
{
public:
	static void SpawnPawn();
	static void ShowSkin();
	static void DestroyLods();
	static void DropLoadingScreen();
	static void AddToInventory(class UObject* itemDef, int Count, char Index, int Slot);
	static void InitializeInventory();
	static void OnServerExecuteInventoryItem(struct FGuid ItemGuid);
	static void OnServerExecuteInventoryWeapon(class UObject* FortWeapon);
	static void GrantDefaultAbilities();
	static void InventoryUpdate();
	static void OnAircraftJump();
	static void RemoveNetDebugUI();
	static void TeleportToSpawnIsland();
	static void ConsoleKey();
	static void CheatScript(const char* script);
	static void ServerHandlePickup(class UObject* Pickup);
	static void FixBuildingFoundations();
	static void Fixbus();
	static void Loot(class UObject* ReceivingActor);
	static void OnServerCreateBuildingActor(PVOID Params);
	static void OnBeginEditActor(class UObject* BuildingPiece);
	static void OnFinishEditActor(class UObject* BuildingActor, class UObject* NewClass, int RotationIteration, bool bMirrored, PVOID Params);
	static void SpawnPickup(class UObject* ItemDefinition, int Count, struct FVector Location, bool bToss = true);
	static void GrantAbility(class UObject* Class);
	static void SpawnWarmupFloorLoot();
	static void SpawnAthenaFloorLoot();
	static void SpawnBuildPreviews();
	static void SetupLooting();

	class Looting
	{
	public:
		struct LootData
		{
			class UObject* ItemDefinition;
			int Count;
		};
		
		static std::vector<LootData> PickLootDrops(struct FName Category);
	};
private:
};