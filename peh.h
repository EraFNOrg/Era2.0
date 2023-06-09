#pragma once
#include <functional>

#include "core.h"
#include "sdk.h"
#include "Athena.h"
#include <string>

inline struct FVector EditComponentLocation;
inline struct FRotator EditComponentRotation;

inline void* ProcessEvent(UObject* Object, UObject* Function, PVOID Params)
{
	//better performance 
	string FuncName = Function->GetName();

	if (FuncName.find(_("ReadyToStartMatch")) != -1 &&
		!bLoadedInMatch)
	{
		Core::OnReadyToStartMatch();
	}
	else if (FuncName.find(_("ServerHandlePickup")) != -1)
	{
		Athena::ServerHandlePickup(*(UObject**)(Params));
	}
	else if (FuncName.find(_("ToggleInfiniteAmmo")) != -1 && bLoadedInMatch)
	{
		bInfiniteAmmo = !bInfiniteAmmo;
		kismetSystemLib->Call(_("SetBoolPropertyByName"), PlayerController, kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"bInfiniteAmmo"))), bInfiniteAmmo);
		return NULL; //Some version got this func, not to execute it twice we will only use our own implementation.
	}
	else if (FuncName.find(_("GiveWeapon")) != -1)
	{
		struct ParamsStruct
		{
			FString Item;
			int Slot;
			int Count;
		};

		auto StructInstance = *reinterpret_cast<ParamsStruct*>(Params);
		
		auto ItemDefinition = FindObject(StructInstance.Item.ToWString(), false, true);

		if (ItemDefinition) Athena::AddToInventory(ItemDefinition, StructInstance.Count, char(0), StructInstance.Slot);
		Athena::InventoryUpdate();
		return NULL;
	}
	else if (FuncName.find(_("ServerCreateBuildingActor")) != -1)
	{
		Athena::OnServerCreateBuildingActor(Params);
	}
	else if (FuncName.find(_("ServerBeginEditingBuildingActor")) != -1)
	{
		Athena::OnBeginEditActor(*(UObject**)(Params));
	}
	else if (FuncName.find(_("OnSuccessfulMatchInteract")) != -1)
	{
		if (auto EditActor = &PlayerController->Child(_("EditBuildingActor")); !IsBadReadPtr(EditActor))
		{
			if (auto Preview = &(*EditActor)->Child(_("EditModeSupport"))->Child(_("PreviewComponent")); !IsBadReadPtr(Preview))
			{
				EditComponentLocation = (*Preview)->Call<FVector>(_("K2_GetComponentLocation"));
				EditComponentRotation = (*Preview)->Call<FRotator>(_("K2_GetComponentRotation"));
			}
		}
	}
	else if (FuncName.find(_("ServerEditBuildingActor")) != -1)
	{
		struct ParamsStruct
		{
			class UObject* BuildingActor;
			class UObject* NewClass;
			int RotationIteration;
			bool bMirrored;
		};

		auto ParamsInstance = *(ParamsStruct*)(Params);

		Athena::OnFinishEditActor(ParamsInstance.BuildingActor, ParamsInstance.NewClass, ParamsInstance.RotationIteration, ParamsInstance.bMirrored, Params);
	}
	else if (FuncName.find(_("ServerLoadingScreenDropped")) != -1 &&
		bLoadedInMatch)
	{
		Core::OnServerLoadingScreenDropped();
	}
	else if (FuncName.find(_("ServerExecuteInventoryItem")) != -1)
	{
		Athena::OnServerExecuteInventoryItem(*(FGuid*)(Params));
	}
	else if (FuncName.find(_("ServerExecuteInventoryWeapon")) != -1)
	{
		Athena::OnServerExecuteInventoryWeapon(*(UObject**)(Params));
	}
	else if ((FuncName.find(_("ServerAttemptAircraftJump")) != -1 ||
		FuncName.find(_("OnAircraftExitedDropZone")) != -1) &&
		!bDroppedFromAircraft)
	{
		Athena::OnAircraftJump();
	}
	else if (FuncName.find(_("CheatScript")) != -1)
	{
		Athena::CheatScript(((FString*)Params)->ToString().c_str());
	}
	else if (FuncName.find(_("ServerAttemptInteract")) != -1)
	{
		Athena::Loot(*(UObject**)(Params));
	}
	else if (FuncName.find(_("ReturnToMainMenu")) != -1)
	{
		PlayerController->Call(_("SwitchLevel"), FString(_(L"Frontend")));
		bInFrontend = !bInFrontend;
	}
	else if ((FuncName.find(_("BndEvt__PlayButton_K2Node_ComponentBoundEvent_0_CommonButtonClicked__DelegateSignature")) != -1 ||
		FuncName.find(_("BP_PlayButton")) != -1) &&
		!bPressedPlay)
	{
		Core::PlayButton();
	}
	else if (FuncName.find(_("ServerPlayEmoteItem")) != -1)
	{
		Athena::PlayEmoteItem(*(UObject**)Params);
	}
	else if (FuncName.find(_("OnPawnEnterVehicle")) != -1)
	{
		g_bIsVehicleVersion = true;
	}
	else if (FuncName.find(_("ServerAttemptExitVehicle")) != -1)
	{
		Athena::OnExitVehicle();
	}
	else if ((FuncName.find(_("ServerSpawnInventoryDrop")) != -1) || 
		(FuncName.find(_("ServerAttemptInventoryDrop")) != -1))
	{
		Athena::DropInventoryItem(*(FGuid*)Params, *(int*)(int64(Params) + sizeof(FGuid)));
	}
	else if (FuncName.find(_("Tick")) != -1 && Object == PlayerController)
	{
		Athena::Tick();
	}

	return PE(Object, Function, Params);
}

inline void* CopyScriptStructHook(UObject* Struct, void* OutPtr, void* InPtr, int ArraySize)
{
	static auto LootTierDataRowStruct = FindObject(_(L"/Script/FortniteGame.FortLootTierData"));
	static auto LootPackagesRowStruct = FindObject(_(L"/Script/FortniteGame.FortLootPackageData"));

	if (!Struct) return NULL;
	
	if (Struct == LootTierDataRowStruct ||
		Struct == LootPackagesRowStruct)
	{
		//copy it manually
		memcpy(OutPtr, InPtr, *(int32*)(int64(Struct) + offsets::StructSize));
		
		return NULL;
	}
	
	return CopyScriptStruct(Struct, OutPtr, InPtr, ArraySize);
}
