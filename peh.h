#pragma once
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
	else if (FuncName.find(_("ServerCreateBuildingActor")) != -1)
	{
		Athena::OnServerCreateBuildingActor();
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

		Athena::OnFinishEditActor(ParamsInstance.BuildingActor, ParamsInstance.NewClass, ParamsInstance.RotationIteration, ParamsInstance.bMirrored);
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

	return PE(Object, Function, Params);
}
