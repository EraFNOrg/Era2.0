#pragma once
#include "globals.h"
#include "sdk.h"

// toggeling SERVER code is in globals.h

#define WORLDTIME GameStatics->Call<float>(_("GetTimeSeconds"))

#if defined(SERVER)

namespace Net
{
	namespace Globals
	{
		double GReplicateActorTimeSeconds = 0.0;
		int32  GNumSaturatedConnections;
		int32  GNumSaturatedConnections = 0;
		int32  GNumReplicateActorCalls = 0;

		inline void (*ActorCallPreReplication)(UObject*, UObject*);
		inline bool (*ActorIsNetStartupActor)(UObject*);
		inline void (*RemoveNetworkActor)(UObject*, UObject*);
		inline float (*ActorGetNetPriority)(UObject*, FVector&, FVector&, UObject*, UObject*, UObject*, float, bool);
		inline int64 (*ActorChannelReplicateActor)(UObject*);
		inline float (*EngineGetMaxTickRate)(UObject*, float, bool);
		inline UObject* (*ConnectionFindActorChannelRef)(UObject*, const TSharedRef<UObject*>&);
		inline void (*ControllerSendClientAdjustment)(UObject*);
		inline bool (*ActorIsNetRelevantFor)(UObject*, const UObject*, const UObject*, const FVector&);
		inline void (*ActorForceNetUpdate)(UObject*);
		inline int64(*ActorChanelClose)(UObject*);

		inline static auto IsActorRelevantToConnection = [](UObject* Actor, const TArray<FNetViewer>& ConnectionViewers)
		{
			for (int32 viewerIdx = 0; viewerIdx < ConnectionViewers.Num(); viewerIdx++)
			{
				if (Globals::ActorIsNetRelevantFor(Actor, ConnectionViewers[viewerIdx].InViewer, ConnectionViewers[viewerIdx].ViewTarget, ConnectionViewers[viewerIdx].ViewLocation))
				{
					return true;
				}
			}

			return false;
		};

		bool Init()
		{
			ActorCallPreReplication = decltype(ActorCallPreReplication)(FindPattern(_("E8 ? ? ? ? 48 83 3F 00 0F B6 6B 14")));
			ActorIsNetStartupActor = decltype(ActorIsNetStartupActor)(FindPattern(_("E8 ? ? ? ? 48 8B 55 07 84 C0")));
			RemoveNetworkActor = decltype(RemoveNetworkActor)(FindPattern(_("E8 ? ? ? ? 48 83 C3 10 48 3B DF 75 E7 48 8B 5C 24 ?")));
			ActorGetNetPriority = decltype(ActorGetNetPriority)(FindPattern(_("Some Pattern")));
			ActorChannelReplicateActor = decltype(ActorChannelReplicateActor)(FindPattern(_("Some Pattern")));
			EngineGetMaxTickRate = decltype(EngineGetMaxTickRate)(FindPattern(_("Some Pattern")));
			ConnectionFindActorChannelRef = decltype(ConnectionFindActorChannelRef)(FindPattern(_("Some Pattern")));
			ControllerSendClientAdjustment = decltype(ControllerSendClientAdjustment)(FindPattern(_("Some Pattern")));
			ActorIsNetRelevantFor = decltype(ActorIsNetRelevantFor)(FindPattern(_("Some Pattern")));
			ActorForceNetUpdate = decltype(ActorForceNetUpdate)(FindPattern(_("Some Pattern")));
			ActorChanelClose = decltype(ActorChanelClose)(FindPattern(_("Some Pattern")));

			if (ActorCallPreReplication && ActorIsNetStartupActor && RemoveNetworkActor && ActorGetNetPriority
				&& ActorChannelReplicateActor && EngineGetMaxTickRate && ConnectionFindActorChannelRef
				&& ControllerSendClientAdjustment && ActorIsNetRelevantFor && ActorForceNetUpdate && ActorChanelClose)
			{
				return true;
			}

			printf(_("U mom giey"));
			return false;
		}

	}

	namespace Utils
	{
		template<typename T>
		FORCEINLINE T Max(T A, T B)
		{
			return (A >= B) ? A : B;
		}

		template<typename T>
		FORCEINLINE T Clamp(T X, T Min, T Max)
		{
			return X < Min ? Min : X < Max ? X : Max;
		}
	}
}

#endif