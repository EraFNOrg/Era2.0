#pragma once
#include "sdk.h"
#include "Net-Structs.h"

#if defined(SERVER)

namespace Net
{
	int32 ServerReplicateActors_PrepConnections(UObject* NetDriver, float DeltaSeconds)
	{
		TArray<UObject*> ClientConnections = NetDriver->Child<TArray<UObject*>>(_("ClientConnections"));

		int32_t NumClientsToTick = ClientConnections.Num();

		bool bFoundReadyConnection = false;

		for (int ConnIdx = 0; ConnIdx < ClientConnections.Num(); ConnIdx++)
		{
			auto Connection = ClientConnections[ConnIdx];

			//CHECK
			if (!Connection)
			{
				printf(_("the connection with index %d is nullptr.\n"), ConnIdx);
				continue;
			}

			auto OwningActor = Connection->Child<UObject*>(_("OwningActor"));

			auto Driver = Connection->Child<UObject*>(_("Driver"));
			auto Time = Driver->Child<float>(_("Time"));
			auto LastRecieveTime = Connection->Child<double>(_("LastReceiveTime"));

			TArray<UObject*> ChildConnections = Connection->Child<TArray<UObject*>>(_("Children"));

			if (OwningActor && Time - LastRecieveTime < 1.5f)
			{
				printf(_("valid owningactor and time.\n"));

				bFoundReadyConnection = true;

				auto DesiredViewTarget = OwningActor;

				auto ConnectionPlayerController = Connection->Child<UObject*>(_("PlayerController"));

				if (ConnectionPlayerController)
				{
					if (UObject* ViewTarget = ConnectionPlayerController->Call<UObject*>(_("GetViewTarget")))
					{
						DesiredViewTarget = ViewTarget;
					}
				}

				Connection->Child<UObject*>(_("ViewTarget")) = DesiredViewTarget;

				for (int ChildIdx = 0; ChildIdx < ChildConnections.Num(); ChildIdx++)
				{
					auto Child = ChildConnections[ChildIdx];

					auto ChildsPlayerController = Child->Child<UObject*>(_("PlayerController"));

					if (ChildsPlayerController)
					{
						Child->Child<UObject*>(_("ViewTarget")) = ChildsPlayerController->Call<UObject*>(_("GetViewTarget"));
					}
					else
					{
						Child->Child<UObject*>(_("ViewTarget")) = NULL;
					}
				}
			}
			else
			{
				Connection->Child<UObject*>(_("ViewTarget")) = NULL;

				for (int ChildIdx = 0; ChildIdx < ChildConnections.Num(); ChildIdx++)
				{
					auto Child = ChildConnections[ChildIdx];

					Child->Child<UObject*>(_("ViewTarget")) = NULL;
				}
			}
		}
		return bFoundReadyConnection ? NumClientsToTick : 0;
	}

	void ServerReplicateActors_BuildConsiderList(UObject* NetDriver, TArray<FNetworkObjectInfo*> OutConsiderList, float ServerTickTime)
	{/*
		int32_t NumInitiallyDormant = 0;

		const bool bUseAdaptiveNetFrequency = false;////////// should be set by the return value of a function

		TArray<UObject*> ActorsToRemove;

		for (const TSharedPtr<FNetworkObjectInfo>& ObjectInfo : TSet<int>()) // ?????????
		{
			auto ActorInfo = ObjectInfo.Object;

			auto WorldTimeSeconds = GameStatics->Call<float>(_("GetTimeSeconds"));

			if (!ActorInfo->bPendingNetUpdate && WorldTimeSeconds <= ActorInfo->NextUpdateTime)
			{
				continue;
			}

			auto Actor = ActorInfo->Actor;

			auto ActorNetUpdateFrequency = Actor->Child<float>(_("NetUpdateFrequency"));
			auto ActorMinNetUpdateFrequency = Actor->Child<float>(_("MinNetUpdateFrequency"));

			if (Actor->Child<ENetRole>(_("RemoteRole")) == ENetRole::ROLE_None)
			{
				printf(_("Actor added to 'ActorsToRemove' list cause it didn't have a NetRole\n"));
				ActorsToRemove.Add(Actor);
			}

			if (Actor->Child<FName>(_("NetDriverName")) != NetDriver->Child<FName>(_("NetDriverName")));
			{
				printf(_("Actor added to 'ActorsToRemove' list cause it belongs to a different NetDriver\n"));
				ActorsToRemove.Add(Actor);
			}

			UObject* Level = Actor->Outer; // Actor->Outer

			auto CheckLevel = [&](){return Level->Child(_("OwningWorld"))&&(Level == Level->Child(_("OwningWorld"))->Child(_("CurrentLevelPendingVisibility"))|| Level == Level->Child(_("OwningWorld"))->Child(_("CurrentLevelPendingInvisibility")));};

			if (!Level || CheckLevel())
			{
				continue;
			}

			if (Actor && Globals::ActorIsNetStartupActor(Actor) && Actor->Child<ENetDormancy>(_("NetDormancy")) == ENetDormancy::DORM_Initial)
			{
				NumInitiallyDormant++;
				ActorsToRemove.Add(Actor);
				printf(_("skipping Actor %s - its initially dormant!"), ((UObject*)(__int64)Actor)->GetFullName());
				continue;
			}


			if (ActorInfo->LastNetReplicateTime == 0)
			{
				ActorInfo->LastNetReplicateTime = WorldTimeSeconds;
				ActorInfo->OptimalNetUpdateDelta = 1.0f / ActorNetUpdateFrequency;
			}

			const float ScaleDownStartTime = 2.0f;
			const float ScaleDownTimeRange = 5.0f;

			const float LastReplicateDelta = WorldTimeSeconds - ActorInfo->LastNetReplicateTime;

			if (LastReplicateDelta > ScaleDownStartTime)
			{
				if (ActorMinNetUpdateFrequency == 0.0f)
				{
					ActorMinNetUpdateFrequency = 2.0f;
				}

				const float MinOptimalDelta = 1.0f / ActorNetUpdateFrequency;
				const float MaxOptimalDelta = ActorMinNetUpdateFrequency >= MinOptimalDelta ? ActorMinNetUpdateFrequency : MinOptimalDelta;

				auto clamp = [](float X, float Min, float Max)
				{
					return X >= Min ? Min : X >= Max ? X : Max;
				};

				const float Alpha = clamp((LastReplicateDelta - ScaleDownStartTime) / ScaleDownTimeRange, 0.0f, 1.0f);

				ActorInfo->OptimalNetUpdateDelta = std::lerp(MinOptimalDelta, MaxOptimalDelta, Alpha);
			}

			if (!ActorInfo->bPendingNetUpdate)
			{
				const float NextUpdateDelta = bUseAdaptiveNetFrequency ? ActorInfo->OptimalNetUpdateDelta : 1.0f / ActorNetUpdateFrequency;

				ActorInfo->NextUpdateTime = WorldTimeSeconds + rand() * ServerTickTime + NextUpdateDelta;

				ActorInfo->LastNetUpdateTime = NetDriver->Child<float>(_("Time"));
			}

			ActorInfo->bPendingNetUpdate = false;

			OutConsiderList.Add(ActorInfo);

			Globals::ActorCallPreReplication(Actor, NetDriver);
		}

		for (int i = 0; i < ActorsToRemove.Num(); i++)
		{
			Globals::RemoveNetworkActor(NetDriver, ActorsToRemove[i]);
		}
		*/
	}
	/*
	int32 ServerReplicateActors_PrioritizeActors(UObject* NetDriver, UObject* Connection, const TArray<FNetViewer>& ConnectionViewers, const TArray<FNetworkObjectInfo*> ConsiderList, const bool bCPUSaturated, FActorPriority*& OutPriorityList, FActorPriority**& OutPriorityActors)
	{
		int32 FinalSortedCount = 0;
		int32 DeletedCount = 0;

		auto& NetTag = NetDriver->Child<int>(_("NetTag"));
		NetTag++;

		auto& SentTemporaries = NetDriver->Child<TArray<UObject*>>(_("SentTemporaries"));
		for (int32 i = 0; i < SentTemporaries.Num(); i++)
		{
			SentTemporaries[i]->Child<int>(_("NetTag")) = NetTag;
		}

		// Make weak ptr once for IsActorDormant call
		//TWeakObjectPtr<UNetConnection> WeakConnection(Connection);

		const int32 MaxSortedActors = ConsiderList.Num() + DestroyedStartupOrDormantActors.Num();
		if (MaxSortedActors > 0)
		{
			OutPriorityList = new FActorPriority[MaxSortedActors];
			OutPriorityActors = new FActorPriority*[MaxSortedActors];

			for (FNetworkObjectInfo* ActorInfo : ConsiderList)
			{
				AActor* Actor = ActorInfo->Actor;

				UObject* Channel = Connection->FindActorChannelRef(ActorInfo->WeakActor);

				if (!Channel)
				{
					if (!IsLevelInitializedForActor(Actor, Connection))
					{
						continue;
					}

					if (!IsActorRelevantToConnection(Actor, ConnectionViewers))
					{
						continue;
					}
				}

				UNetConnection* PriorityConnection = Connection;

				if (Actor->bOnlyRelevantToOwner)
				{
					bool bHasNullViewTarget = false;

					PriorityConnection = IsActorOwnedByAndRelevantToConnection(Actor, ConnectionViewers, bHasNullViewTarget);

					if (PriorityConnection == nullptr)
					{
						if (!bHasNullViewTarget && Channel != NULL && Time - Channel->RelevantTime >= RelevantTimeout)
						{
							Channel->Close();
						}

						continue;
					}
				}

				if (Actor->NetTag != NetTag)
				{
					Actor->NetTag = NetTag;

					OutPriorityList[FinalSortedCount] = FActorPriority(PriorityConnection, Channel, ActorInfo, ConnectionViewers, bLowNetBandwidth);
					OutPriorityActors[FinalSortedCount] = OutPriorityList + FinalSortedCount;

					FinalSortedCount++;
				}
			}

			// Add in deleted actors
			for (auto It = Connection->GetDestroyedStartupOrDormantActorGUIDs().CreateConstIterator(); It; ++It)
			{
				FActorDestructionInfo& DInfo = *DestroyedStartupOrDormantActors.FindChecked(*It);
				OutPriorityList[FinalSortedCount] = FActorPriority(Connection, &DInfo, ConnectionViewers);
				OutPriorityActors[FinalSortedCount] = OutPriorityList + FinalSortedCount;
				FinalSortedCount++;
				DeletedCount++;
			}*
			// TSets here too im gonna kms

			//Sort(OutPriorityActors, FinalSortedCount, FCompareFActorPriority());
		}

		return FinalSortedCount;
	}
	*/

	int32 ServerReplicateActors(UObject* NetDriver, float DeltaSeconds)
	{
		if (NetDriver->Child<TArray<UObject*>>(_("ReplicationDriver")).Num() == 0)
		{
			return 0;
		}

		if (NetDriver->Child(_("ReplicationDriver")))
		{
			printf(_("ReplicationDriver ServerReplicateActors should be called instead!\n"));
			return 0;
		}

		if (!NetDriver->Child(_("World")))
		{
			printf(_("Error: WORLD is nullptr!\n"));
			return 0;
		}

		int NumClientsToTick = ServerReplicateActors_PrepConnections(NetDriver, DeltaSeconds);

		//DO THAT FOR DEBUG
		//printf(_("NumClientsToTick: %d\n"), NumClientsToTick);

		if (NumClientsToTick == 0)
		{
			return 0;
		}
	}

}


#endif