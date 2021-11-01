#pragma once
#include "globals.h"
#include "sdk.h"

#if defined(SERVER)

namespace Net
{
	namespace Globals
	{
		inline void (*ActorCallPreReplication)(UObject*, UObject*);
		inline bool (*ActorIsNetStartupActor)(UObject*);
		inline void (*RemoveNetworkActor)(UObject*, UObject*);


		bool Init()
		{
			ActorCallPreReplication = decltype(ActorCallPreReplication)(FindPattern(_("E8 ? ? ? ? 48 83 3F 00 0F B6 6B 14")));
			ActorIsNetStartupActor = decltype(ActorIsNetStartupActor)(FindPattern(_("E8 ? ? ? ? 48 8B 55 07 84 C0")));
			RemoveNetworkActor = decltype(RemoveNetworkActor)(FindPattern(_("E8 ? ? ? ? 48 83 C3 10 48 3B DF 75 E7 48 8B 5C 24 ?")));
		}

	}
}

#endif