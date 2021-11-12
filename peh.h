#pragma once
#include "core.h"
#include "sdk.h"
#include <string>

inline void* ProcessEvent(UObject* Object, UObject* Function, PVOID Params)
{
	if (Function->GetName().find(_("ReadyToStartMatch")) != -1 &&
		!bLoadedInMatch)
	{
		Core::OnReadyToStartMatch();
	}
	else if (Function->GetName().find(_("ServerLoadingScreenDropped")) != -1 &&
		bLoadedInMatch)
	{
		Core::OnServerLoadingScreenDropped();
	}
	else if (Function->GetName().find(_("ServerExecuteInventoryItem")) != -1)
	{
		Athena::OnServerExecuteInventoryItem(*(FGuid*)(Params));
	}
	else if (Function->GetName().find(_("ServerExecuteInventoryWeapon")) != -1)
	{
		Athena::OnServerExecuteInventoryWeapon(*(UObject**)(Params));
	}
	else if (PlayerController && Object == PlayerController && Function->GetName().find(_("Tick")) != -1)
	{
		Athena::Tick();
	}
	else if ((Function->GetName().find(_("ServerAttemptAircraftJump")) != -1 ||
		Function->GetName().find(_("OnAircraftExitedDropZone")) != -1) &&
		!bDroppedFromAircraft)
	{
		Athena::OnAircraftJump();
	}
	else if (Function->GetName().find(_("CheatScript")) != -1)
	{
		Athena::CheatScript(((FString*)Params)->ToString().c_str());
	}

	return PE(Object, Function, Params);
}
