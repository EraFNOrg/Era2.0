#pragma once
#include "core.h"
#include "sdk.h"
#include <string>

inline void* ProcessEvent(UObject* Object, UObject* Function, PVOID Params)
{
	//better performance 
	string FuncName = Function->GetName();

	if (FuncName.find(_("ReadyToStartMatch")) != -1 &&
		!bLoadedInMatch)
	{
		Core::OnReadyToStartMatch();
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
	else if (PlayerController && Object == PlayerController && FuncName.find(_("Tick")) != -1)
	{
		Athena::Tick();
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
	else if ((FuncName.find(_("BndEvt__PlayButton_K2Node_ComponentBoundEvent_0_CommonButtonClicked__DelegateSignature")) != -1 ||
		FuncName.find(_("BP_PlayButton")) != -1) &&
		!bPressedPlay)
	{
		//Season check
		if (GetEngineVersion().ToString().substr(34, 4).starts_with(_("11."))) {
			PlayerController->Call(_("SwitchLevel"), FString(_(L"Apollo_Terrain")));
			bPressedPlay = !bPressedPlay;
		}
		else
		{
			PlayerController->Call(_("SwitchLevel"), FString(_(L"Athena_Terrain")));
			bPressedPlay = !bPressedPlay;
		}
	}

	return PE(Object, Function, Params);
}
