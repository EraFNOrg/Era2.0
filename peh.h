#pragma once
#include "core.h"
#include "sdk.h"
#include <string>

inline void* ProcessEvent(UObject* Object, UObject* Function, PVOID Params)
{
	std::string FuncName = Function->GetName();

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

	return PE(Object, Function, Params);
}
