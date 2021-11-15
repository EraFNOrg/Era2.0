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
	{
		int32_t NumInitiallyDormant = 0;

		const bool bUseAdaptiveNetFrequency = false;////////// should be set by the return value of a function

		TArray<UObject*> ActorsToRemove;

		//***********************************************************
		TSet<TSharedPtr<FNetworkObjectInfo>> ActiveNetworkObjects; //
		//***********************************************************

		for (const TSharedPtr<FNetworkObjectInfo>& ObjectInfo : ActiveNetworkObjects)
		{
			auto ActorInfo = ObjectInfo.Object;

			auto WorldTimeSeconds = WORLDTIME;

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

				const float Alpha = Utils::Clamp((LastReplicateDelta - ScaleDownStartTime) / ScaleDownTimeRange, 0.0f, 1.0f);

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
	}
	
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
			OutPriorityList = new FActorPriority;
			OutPriorityActors = new FActorPriority*;

			for (FNetworkObjectInfo* ActorInfo : ConsiderList)
			{
				UObject* Actor = ActorInfo->Actor;

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

				UObject* PriorityConnection = Connection;

				if (Actor->bOnlyRelevantToOwner)
				{
					bool bHasNullViewTarget = false;

					PriorityConnection = IsActorOwnedByAndRelevantToConnection(Actor, ConnectionViewers, bHasNullViewTarget);

					if (PriorityConnection == nullptr)
					{
						if (!bHasNullViewTarget && Channel != NULL && NetDriver->Child<float>(_("Time")) - Channel->RelevantTime >= NetDriver->Child<float>(_("RelevantTimeout")))
						{
							Channel->Close();
						}

						continue;
					}
				}

				if (Actor->Child<int32>(_("NetTag ")) != NetTag)
				{
					Actor->Child<int32>(_("NetTag ")) = NetTag;

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
			}
			// TSets here too im gonna kms

			//Sort(OutPriorityActors, FinalSortedCount, FCompareFActorPriority());
		}

		return FinalSortedCount;
	}
	

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



	int32 ServerReplicateActors_ProcessPrioritizedActors(UObject* Driver, UObject* Connection, const TArray<FNetViewer>& ConnectionViewers, FActorPriority** PriorityActors, const int32 FinalSortedCount, int32& OutUpdated)
	{

		int32 ActorUpdatesThisConnection = 0;
		int32 ActorUpdatesThisConnectionSent = 0;
		int32 FinalRelevantCount = 0;

		if (!Connection->IsNetReady(0))
		{
			Globals::GNumSaturatedConnections++;
			return 0;
		}

		static auto NAME_None = kismetStringLib->Call<FName>(_("Conv_StringToName"), FString(_(L"None")));

		for (int32 j = 0; j < FinalSortedCount; j++)
		{
			FNetworkObjectInfo* ActorInfo = PriorityActors[j]->ActorInfo;

			if (ActorInfo == NULL && PriorityActors[j]->DestructionInfo)
			{
				// Make sure client has streaming level loaded
				if (PriorityActors[j]->DestructionInfo->StreamingLevelName != NAME_None && !Connection->ClientVisibleLevelNames.Contains(PriorityActors[j]->DestructionInfo->StreamingLevelName))
				{
					// This deletion entry is for an actor in a streaming level the connection doesn't have loaded, so skip it
					continue;
				}

				UObject* Channel = Connection->CreateChannel(CHTYPE_Actor, 1);
				if (Channel)
				{
					FinalRelevantCount++;

					Channel->SetChannelActorForDestroy(PriorityActors[j]->DestructionInfo);		// Send a close bunch on the new channel
					Connection->RemoveDestructionInfo(PriorityActors[j]->DestructionInfo);		// Remove from connections to-be-destroyed list (close bunch of reliable, so it will make it there)
				}
				continue;
			}


			// Normal actor replication
			UObject* Channel = PriorityActors[j]->Channel;
			if (!Channel || Channel->Child(_("Actor")))
			{
				UObject* Actor = ActorInfo->Actor;
				bool bIsRelevant = false;

				const bool bLevelInitializedForActor = IsLevelInitializedForActor(Actor, Connection);

				if (bLevelInitializedForActor)
				{
					if (!Actor->GetTearOff() && (!Channel || Driver->Child<float>(_("Time")) - Channel->RelevantTime > 1.f))
					{
						if (IsActorRelevantToConnection(Actor, ConnectionViewers))
						{
							bIsRelevant = true;
						}
						else if (DebugRelevantActors)
						{
							LastNonRelevantActors.Add(Actor);
						}
					}
				}
				else
				{
					// Actor is no longer relevant because the world it is/was in is not loaded by client
					// exception: player controllers should never show up here
					printf(_("- Level not initialized for actor %s"), Actor->GetName());
				}

				// if the actor is now relevant or was recently relevant
				const bool bIsRecentlyRelevant = bIsRelevant || (Channel && Driver->Child<float>(_("Time")) - Channel->RelevantTime < RelevantTimeout) || ActorInfo->bForceRelevantNextUpdate;

				ActorInfo->bForceRelevantNextUpdate = false;

				if (bIsRecentlyRelevant)
				{
					FinalRelevantCount++;

					// Find or create the channel for this actor.
					// we can't create the channel if the client is in a different world than we are
					// or the package map doesn't support the actor's class/archetype (or the actor itself in the case of serializable actors)
					// or it's an editor placed actor and the client hasn't initialized the level it's in
					if (Channel == NULL && GuidCache->SupportsObject(Actor->GetClass()) && GuidCache->SupportsObject(Actor->IsNetStartupActor() ? Actor : Actor->GetArchetype()))
					{
						if (bLevelInitializedForActor)
						{
							// Create a new channel for this actor.
							Channel = Connection->CreateChannel(CHTYPE_Actor, 1);
							if (Channel)
							{
								Channel->SetChannelActor(Actor);
							}
						}
						// if we couldn't replicate it for a reason that should be temporary, and this Actor is updated very infrequently, make sure we update it again soon
						else if (Actor->NetUpdateFrequency < 1.0f)
						{
							ActorInfo->NextUpdateTime = WORLDTIME + 0.2f * rand();
						}
					}

					if (Channel)
					{
						// if it is relevant then mark the channel as relevant for a short amount of time
						if (bIsRelevant)
						{
							Channel->RelevantTime = Driver->Child<float>(_("Time")) + 0.5f * rand();
						}
						// if the channel isn't saturated
						if (Channel->IsNetReady(0))
						{
							// replicate the actor
							if (DebugRelevantActors)
							{
								LastRelevantActors.Add(Actor);
							}

							double ChannelLastNetUpdateTime = Channel->LastUpdateTime;

							if (Globals::ActorChannelReplicateActor(Channel))
							{
								ActorUpdatesThisConnectionSent++;
								if (DebugRelevantActors)
								{
									LastSentActors.Add(Actor);
								}

								// Calculate min delta (max rate actor will upate), and max delta (slowest rate actor will update)
								const float MinOptimalDelta = 1.0f / Actor->Child<float>(_("NetUpdateFrequency"));
								const float MaxOptimalDelta = Utils::Max(1.0f / Actor->Child<float>(_("MinNetUpdateFrequency")), MinOptimalDelta);
								const float DeltaBetweenReplications = (WORLDTIME - ActorInfo->LastNetReplicateTime);

								// Choose an optimal time, we choose 70% of the actual rate to allow frequency to go up if needed
								ActorInfo->OptimalNetUpdateDelta = Utils::Clamp(DeltaBetweenReplications * 0.7f, MinOptimalDelta, MaxOptimalDelta);
								ActorInfo->LastNetReplicateTime = WORLDTIME;
							}
							ActorUpdatesThisConnection++;
							OutUpdated++;
						}
						else
						{
							// otherwise force this actor to be considered in the next tick again
							Actor->ForceNetUpdate();
						}
						// second check for channel saturation
						if (!Connection->IsNetReady(0))
						{
							// We can bail out now since this connection is saturated, we'll return how far we got though
							GNumSaturatedConnections++;
							return j;
						}
					}
				}

				// If the actor wasn't recently relevant, or if it was torn off, close the actor channel if it exists for this connection
				if ((!bIsRecentlyRelevant || Actor->GetTearOff()) && Channel != NULL)
				{
					// Non startup (map) actors have their channels closed immediately, which destroys them.
					// Startup actors get to keep their channels open.

					// Fixme: this should be a setting
					if (!bLevelInitializedForActor || !Actor->IsNetStartupActor())
					{
						Channel->Close();
					}
				}
			}
		}

		return FinalSortedCount;
	}


	int32 ServerReplicateActors(UObject* Driver, float DeltaSeconds)
	{

		TArray<UObject*>& ClientConnections = Driver->Child<TArray<UObject*>>(_("ClientConnections"));


		if (ClientConnections.Num() == 0)
		{
			return 0;
		}

		if (Driver->Child(_("ReplicationDriver")))
		{
			return ServerReplicateActors(Driver->Child(_("ReplicationDriver")), DeltaSeconds);
		}

		
		// Bump the ReplicationFrame value to invalidate any properties marked as "unchanged" for this frame.
		ReplicationFrame++;

		int32 Updated = 0;

		const int32 NumClientsToTick = ServerReplicateActors_PrepConnections(Driver, DeltaSeconds);

		if (NumClientsToTick == 0)
		{
			// No connections are ready this frame
			return 0;
		}


		UObject* WorldSettings = nullptr; // ULevel->WorldSettings

		if (Driver->Child(_("World"))->Child(_("PersistentLevel")))
		{
			WorldSettings = Driver->Child(_("World"))->Child(_("PersistentLevel"))->Child(_("WorldSettings"));
		}

		bool bCPUSaturated = false;
		float ServerTickTime = Globals::EngineGetMaxTickRate(UEngine, DeltaSeconds, false);
		if (ServerTickTime == 0.f)
		{
			ServerTickTime = DeltaSeconds;
		}
		else
		{
			ServerTickTime = 1.f / ServerTickTime;
			bCPUSaturated = DeltaSeconds > 1.2f * ServerTickTime;
		}

		TArray<FNetworkObjectInfo*> ConsiderList;
		ConsiderList.Reserve(GetNetworkObjectList().GetActiveObjects().Num());

		// Build the consider list (actors that are ready to replicate)
		ServerReplicateActors_BuildConsiderList(Driver, ConsiderList, ServerTickTime);

		//FMemMark Mark(FMemStack::Get()); Its a thing for the future when we need to optimize stuff

		for (int32 i = 0; i < ClientConnections.Num(); i++)
		{
			UObject* Connection = ClientConnections[i];

			// net.DormancyValidate can be set to 2 to validate all dormant actors against last known state before going dormant
			if (CVarNetDormancyValidate.GetValueOnAnyThread() == 2)
			{
				TMap<TWeakObjectPtr<UObject>, TSharedRef<FObjectReplicator> > DormantReplicatorMap;

				for (auto It = Connection->DormantReplicatorMap.CreateIterator(); It; ++It)
				{
					FObjectReplicator& Replicator = It.Value().Get();

					if (Replicator.OwningChannel != nullptr)
					{
						Replicator.ValidateAgainstState(Replicator.OwningChannel->Child(_("Actor")));
					}
				}
			}

			// if this client shouldn't be ticked this framep
			if (i >= NumClientsToTick)
			{
				// then mark each considered actor as bPendingNetUpdate so that they will be considered again the next frame when the connection is actually ticked
				for (int32 ConsiderIdx = 0; ConsiderIdx < ConsiderList.Num(); ConsiderIdx++)
				{
					UObject* Actor = ConsiderList[ConsiderIdx]->Actor;
					// if the actor hasn't already been flagged by another connection,
					if (Actor != NULL && !ConsiderList[ConsiderIdx]->bPendingNetUpdate)
					{
						// find the channel
						UObject* Channel = Globals::ConnectionFindActorChannelRef(Connection, ConsiderList[ConsiderIdx]->WeakActor);
						// and if the channel last update time doesn't match the last net update time for the actor
						if (Channel != NULL && Channel->LastUpdateTime < ConsiderList[ConsiderIdx]->LastNetUpdateTime)
						{
							//UE_LOG(LogNet, Log, TEXT("flagging %s for a future update"),*Actor->GetName());
							// flag it for a pending update
							ConsiderList[ConsiderIdx]->bPendingNetUpdate = true;
						}
					}
				}
				// clear the time sensitive flag to avoid sending an extra packet to this connection
				Connection->TimeSensitive = false;
			}
			else if (Connection->Child(_("ViewTarget")))
			{
				// Make a list of viewers this connection should consider (this connection and children of this connection)
				TArray<FNetViewer>& ConnectionViewers = WorldSettings->Child<TArray<struct FNetViewer>>(_("ReplicationViewers"));

				auto& ConnectionChildren = Connection->Child<TArray<UObject*>>(_("Children"));

				ConnectionViewers.Reset();
				new(ConnectionViewers)FNetViewer(Connection, DeltaSeconds);
				for (int32 ViewerIndex = 0; ViewerIndex < ConnectionChildren.Num(); ViewerIndex++)
				{
					if (ConnectionChildren[ViewerIndex]->Child(_("ViewTarget")) != NULL)
					{
						new(ConnectionViewers)FNetViewer(ConnectionChildren[ViewerIndex], DeltaSeconds);
					}
				}

				// send ClientAdjustment if necessary
				// we do this here so that we send a maximum of one per packet to that client; there is no value in stacking additional corrections
				if (Connection->Child(_("PlayerController")))
				{
					Globals::ControllerSendClientAdjustment(Connection->Child(_("PlayerController")));
				}

				for (int32 ChildIdx = 0; ChildIdx < ConnectionChildren.Num(); ChildIdx++)
				{
					if (ConnectionChildren[ChildIdx]->Child(_("PlayerController")) != NULL)
					{
						Globals::ControllerSendClientAdjustment(ConnectionChildren[ChildIdx]->Child(_("PlayerController")));
					}
				}

				//FMemMark RelevantActorMark(FMemStack::Get()); // Maybe later when we commit optimization

				FActorPriority* PriorityList = NULL;
				FActorPriority** PriorityActors = NULL;

				// Get a sorted list of actors for this connection
				const int32 FinalSortedCount = ServerReplicateActors_PrioritizeActors(Driver, Connection, ConnectionViewers, ConsiderList, bCPUSaturated, PriorityList, PriorityActors);

				// Process the sorted list of actors for this connection
				const int32 LastProcessedActor = ServerReplicateActors_ProcessPrioritizedActors(Driver, Connection, ConnectionViewers, PriorityActors, FinalSortedCount, Updated);

				// relevant actors that could not be processed this frame are marked to be considered for next frame
				for (int32 k = LastProcessedActor; k < FinalSortedCount; k++)
				{
					if (!PriorityActors[k]->ActorInfo)
					{
						// A deletion entry, skip it because we dont have anywhere to store a 'better give higher priority next time'
						continue;
					}

					UObject* Actor = PriorityActors[k]->ActorInfo->Actor;

					UObject* Channel = PriorityActors[k]->Channel;

					static auto IsActorRelevantToConnection = [](UObject* Actor, const TArray<FNetViewer>& ConnectionViewers)
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

					if (Channel != NULL && Driver->Child<float>(_("Time")) - Channel->RelevantTime <= 1.f)
					{
						PriorityActors[k]->ActorInfo->bPendingNetUpdate = true;
					}
					else if (IsActorRelevantToConnection(Actor, ConnectionViewers))
					{
						// If this actor was relevant but didn't get processed, force another update for next frame
						PriorityActors[k]->ActorInfo->bPendingNetUpdate = true;
						if (Channel != NULL)
						{
							Channel->RelevantTime = Time + 0.5f * rand();
						}
					}
				}
				RelevantActorMark.Pop();

				ConnectionViewers.Reset();
			}
		}

		// shuffle the list of connections if not all connections were ticked
		if (NumClientsToTick < ClientConnections.Num())
		{
			int32 NumConnectionsToMove = NumClientsToTick;
			while (NumConnectionsToMove > 0)
			{
				// move all the ticked connections to the end of the list so that the other connections are considered first for the next frame
				UObject* Connection = ClientConnections[0];
				ClientConnections.RemoveAt(0, 1);
				ClientConnections.Add(Connection);
				NumConnectionsToMove--;
			}
		}
		//Mark.Pop();

		if (DebugRelevantActors)
		{
			PrintDebugRelevantActors();
			LastPrioritizedActors.Empty();
			LastSentActors.Empty();
			LastRelevantActors.Empty();
			LastNonRelevantActors.Empty();

			DebugRelevantActors = false;
		}

		return Updated;
	}


}


#endif