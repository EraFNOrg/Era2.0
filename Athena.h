#pragma once

#include <windows.h>

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
	static void Tick();
	static void CheatScript(const char* script);
	static void ServerHandlePickup(class UObject* Pickup);
	static void FixBuildingFoundations();
	static void Fixbus();
	static void SpawnPickup(class UObject* ItemDefinition, int Count, struct FVector Location);
	static void GrantAbility(class UObject* Class);
private:
};