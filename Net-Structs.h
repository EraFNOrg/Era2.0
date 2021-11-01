#pragma once
#include "Net-Globals.h"

#if defined(SERVER)

namespace Net
{
	template<class ObjectType>
	class TSharedPtr
	{
	public:
		ObjectType* Object;

		int32_t SharedReferenceCount;
		int32_t WeakReferenceCount;
	};

	struct FNetworkObjectInfo
	{
		UObject* Actor;
		uint8_t bDirtyForReplay : 1;
		uint8_t bPendingNetUpdate : 1;
		uint8_t bSwapRolesOnReplicate;
		uint32_t ForceRelevantFrame;
		double LastNetReplicateTime;
		float LastNetUpdateTime;
		double NextUpdateTime;
		float OptimalNetUpdateDelta;

		FNetworkObjectInfo(UObject* InActor)
		{
			this->Actor = InActor;
			this->NextUpdateTime = 0.0;
			this->LastNetReplicateTime = 0.0;
			this->OptimalNetUpdateDelta = 0.0f;
			this->LastNetUpdateTime = 0.0f;
			this->bPendingNetUpdate = false;
			this->bDirtyForReplay = false;
			this->bSwapRolesOnReplicate = false;
		}
	};

	enum class ENetRole
	{
		/** No role at all. */
		ROLE_None,
		/** Locally simulated proxy of this actor. */
		ROLE_SimulatedProxy,
		/** Locally autonomous proxy of this actor. */
		ROLE_AutonomousProxy,
		/** Authoritative control over the actor. */
		ROLE_Authority,
		ROLE_MAX,
	};

	enum class ENetDormancy
	{
		DORM_Never,
		/** This actor can go dormant, but is not currently dormant. Game code will tell it when it go dormant. */
		DORM_Awake,
		/** This actor wants to go fully dormant for all connections. */
		DORM_DormantAll,
		/** This actor may want to go dormant for some connections, GetNetDormancy() will be called to find out which. */
		DORM_DormantPartial,
		/** This actor is initially dormant for all connection if it was placed in map. */
		DORM_Initial,

		DORM_MAX
	};
}


#endif