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

	int64 ReplicateActor(UObject* ActorChannel)
	{

		const UObject* const ActorWorld = ActorChannel->Child(_("Actor"))->GetWorld();

		const bool bReplay = ActorWorld && (ActorWorld->DemoNetDriver == ActorChannel->Child(_("Connection"))->Child(_("Driver")));

		FSimpleScopeSecondsCounter ScopedSecondsCounter(Globals::GReplicateActorTimeSeconds, !bReplay);
		if (!bReplay)
		{
			Globals::GNumReplicateActorCalls++;
		}

		if (bPausedUntilReliableACK)
		{
			if (NumOutRec > 0)
			{
				return 0;
			}
			bPausedUntilReliableACK = 0;
		}

		const TArray<FNetViewer>& NetViewers = ActorWorld->GetWorldSettings()->ReplicationViewers;
		bool bIsNewlyReplicationPaused = false;
		bool bIsNewlyReplicationUnpaused = false;

		if (OpenPacketId.First != -1 && NetViewers.Num() > 0)
		{
			bool bNewPaused = true;

			for (const FNetViewer& NetViewer : NetViewers)
			{
				if (!Actor->IsReplicationPausedForConnection(NetViewer))
				{
					bNewPaused = false;
					break;
				}
			}

			const bool bOldPaused = IsReplicationPaused();

			// We were paused and still are, don't do anything.
			if (bOldPaused && bNewPaused)
			{
				return 0;
			}

			bIsNewlyReplicationUnpaused = bOldPaused && !bNewPaused;
			bIsNewlyReplicationPaused = !bOldPaused && bNewPaused;
			SetReplicationPaused(bNewPaused);
		}

		bool WroteSomethingImportant = bIsNewlyReplicationUnpaused || bIsNewlyReplicationPaused;

		// triggering replication of an Actor while already in the middle of replication can result in invalid data being sent and is therefore illegal
		if (bIsReplicatingActor)
		{
			FString Error(FString::Printf(TEXT("Attempt to replicate '%s' while already replicating that Actor!"), *Actor->GetName()));
			UE_LOG(LogNet, Log, TEXT("%s"), *Error);
			ensureMsgf(false, TEXT("%s"), *Error);
			return 0;
		}

		// Create an outgoing bunch, and skip this actor if the channel is saturated.
		FOutBunch Bunch(this, 0);

		if (Bunch.IsError())
		{
			return 0;
		}

		if (bIsNewlyReplicationPaused)
		{
			Bunch.bReliable = true;
			Bunch.bIsReplicationPaused = true;

		}

		bIsReplicatingActor = true;
		FReplicationFlags RepFlags;

		// Send initial stuff.
		if (OpenPacketId.First != INDEX_NONE && !Connection->bResendAllDataSinceOpen)
		{
			if (!SpawnAcked && OpenAcked)
			{
				// After receiving ack to the spawn, force refresh of all subsequent unreliable packets, which could
				// have been lost due to ordering problems. Note: We could avoid this by doing it in FActorChannel::ReceivedAck,
				// and avoid dirtying properties whose acks were received *after* the spawn-ack (tricky ordering issues though).
				SpawnAcked = 1;
				for (auto RepComp = ReplicationMap.CreateIterator(); RepComp; ++RepComp)
				{
					RepComp.Value()->ForceRefreshUnreliableProperties();
				}
			}
		}
		else
		{
			RepFlags.bNetInitial = true;
			Bunch.bClose = Actor->bNetTemporary;
			Bunch.bReliable = true; // Net temporary sends need to be reliable as well to force them to retry
		}

		// Owned by connection's player?
		UObject* OwningConnection = Actor->GetNetConnection();
		if (OwningConnection == Connection || (OwningConnection != NULL && OwningConnection->IsA(FindObject(_(L"Class Engine.ChildConnection"))) && OwningConnection->Child(_("Parent")) == ActorChannel->Child(_("Connection"))))
		{
			RepFlags.bNetOwner = true;
		}
		else
		{
			RepFlags.bNetOwner = false;
		}

		// ----------------------------------------------------------
		// If initial, send init data.
		// ----------------------------------------------------------

		if (RepFlags.bNetInitial && OpenedLocally)
		{
			Connection->PackageMap->SerializeNewActor(Bunch, this, Actor);
			WroteSomethingImportant = true;

			Actor->OnSerializeNewActor(Bunch);
		}

		// Possibly downgrade role of actor if this connection doesn't own it
		FScopedRoleDowngrade ScopedRoleDowngrade(Actor, RepFlags);

		RepFlags.bNetSimulated = (Actor->GetRemoteRole() == ROLE_SimulatedProxy);
		RepFlags.bRepPhysics = Actor->ReplicatedMovement.bRepPhysics;
		RepFlags.bReplay = bReplay;
		//RepFlags.bNetInitial	= RepFlags.bNetInitial;

		UE_LOG(LogNetTraffic, Log, TEXT("Replicate %s, bNetInitial: %d, bNetOwner: %d"), *Actor->GetName(), RepFlags.bNetInitial, RepFlags.bNetOwner);

		FMemMark	MemMark(FMemStack::Get());	// The calls to ReplicateProperties will allocate memory on FMemStack::Get(), and use it in ::PostSendBunch. we free it below

		// ----------------------------------------------------------
		// Replicate Actor and Component properties and RPCs
		// ---------------------------------------------------

		if (!bIsNewlyReplicationPaused)
		{
			// The Actor
			WroteSomethingImportant |= ActorReplicator->ReplicateProperties(Bunch, RepFlags);

			// The SubObjects
			WroteSomethingImportant |= Actor->ReplicateSubobjects(this, &Bunch, &RepFlags);

			if (Connection->bResendAllDataSinceOpen)
			{
				if (WroteSomethingImportant)
				{
					SendBunch(&Bunch, 1);
				}

				MemMark.Pop();

				bIsReplicatingActor = false;

				return WroteSomethingImportant;
			}

			// Look for deleted subobjects
			for (auto RepComp = ReplicationMap.CreateIterator(); RepComp; ++RepComp)
			{
				if (!RepComp.Key().IsValid())
				{
					if (RepComp.Value()->ObjectNetGUID.IsValid())
					{
						// Write a deletion content header:
						WriteContentBlockForSubObjectDelete(Bunch, RepComp.Value()->ObjectNetGUID);

						WroteSomethingImportant = true;
						Bunch.bReliable = true;
					}
					else
					{
						UE_LOG(LogNetTraffic, Error, TEXT("Unable to write subobject delete for (%s), object replicator has invalid NetGUID"), *GetPathNameSafe(Actor));
					}

					RepComp.Value()->CleanUp();
					RepComp.RemoveCurrent();
				}
			}
		}

		// -----------------------------
		// Send if necessary
		// -----------------------------

		int64 NumBitsWrote = 0;
		if (WroteSomethingImportant)
		{
			FPacketIdRange PacketRange = SendBunch(&Bunch, 1);

			if (!bIsNewlyReplicationPaused)
			{
				for (auto RepComp = ReplicationMap.CreateIterator(); RepComp; ++RepComp)
				{
					RepComp.Value()->PostSendBunch(PacketRange, Bunch.bReliable);
				}

				// If there were any subobject keys pending, add them to the NakMap
				if (PendingObjKeys.Num() > 0)
				{
					// For the packet range we just sent over
					for (int32 PacketId = PacketRange.First; PacketId <= PacketRange.Last; ++PacketId)
					{
						// Get the existing set (its possible we send multiple bunches back to back and they end up on the same packet)
						FPacketRepKeyInfo& Info = SubobjectNakMap.FindOrAdd(PacketId % SubobjectRepKeyBufferSize);
						if (Info.PacketID != PacketId)
						{
							UE_LOG(LogNetTraffic, Verbose, TEXT("ActorChannel[%d]: Clearing out PacketRepKeyInfo for new packet: %d"), ChIndex, PacketId);
							Info.ObjKeys.Empty(Info.ObjKeys.Num());
						}
						Info.PacketID = PacketId;
						Info.ObjKeys.Append(PendingObjKeys);

						FString VerboseString;
						for (auto KeyIt = PendingObjKeys.CreateIterator(); KeyIt; ++KeyIt)
						{
							VerboseString += FString::Printf(TEXT(" %d"), *KeyIt);
						}

					}
				}

				if (Actor->bNetTemporary)
				{
					ActorChannel->Child(_("Connection"))->Child<TArray<UObject*>(_("SentTemporaries")).Add(Actor);
				}
			}
			NumBitsWrote = Bunch.GetNumBits();
		}

		PendingObjKeys.Empty();

		// If we evaluated everything, mark LastUpdateTime, even if nothing changed.
		LastUpdateTime = ActorChannel->Child(_("Connection"))->Child(_("Driver"))->Child(_("Time"));

		MemMark.Pop();

		bIsReplicatingActor = false;

		bForceCompareProperties = false;		// Only do this once per frame when set


		return NumBitsWrote;
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

				UObject* Channel = (UActorChannel*)Connection->CreateChannel(CHTYPE_Actor, 1);
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
							Channel = (UActorChannel*)Connection->CreateChannel(CHTYPE_Actor, 1);
							if (Channel)
							{
								Channel->SetChannelActor(Actor);
							}
						}
						// if we couldn't replicate it for a reason that should be temporary, and this Actor is updated very infrequently, make sure we update it again soon
						else if (Actor->NetUpdateFrequency < 1.0f)
						{
							UE_LOG(LogNetTraffic, Log, TEXT("Unable to replicate %s"), *Actor->GetName());
							ActorInfo->NextUpdateTime = World->TimeSeconds + 0.2f * FMath::FRand();
						}
					}

					if (Channel)
					{
						// if it is relevant then mark the channel as relevant for a short amount of time
						if (bIsRelevant)
						{
							Channel->RelevantTime = Time + 0.5f * FMath::SRand();
						}
						// if the channel isn't saturated
						if (Channel->IsNetReady(0))
						{
							// replicate the actor
							UE_LOG(LogNetTraffic, Log, TEXT("- Replicate %s. %d"), *Actor->GetName(), PriorityActors[j]->Priority);
							if (DebugRelevantActors)
							{
								LastRelevantActors.Add(Actor);
							}

							double ChannelLastNetUpdateTime = Channel->LastUpdateTime;

							if (Channel->ReplicateActor())
							{
#if USE_SERVER_PERF_COUNTERS
								if (const FNetworkObjectInfo* const ObjectInfo = Actor->FindNetworkObjectInfo())
								{
									// A channel time of 0.0 means this is the first time the actor is being replicated, so we don't need to record it
									if (ChannelLastNetUpdateTime > 0.0)
									{
										Connection->GetActorsStarvedByClassTimeMap().FindOrAdd(Actor->GetClass()->GetName()).Add((World->RealTimeSeconds - ChannelLastNetUpdateTime) * 1000.0f);
									}
								}
#endif

								ActorUpdatesThisConnectionSent++;
								if (DebugRelevantActors)
								{
									LastSentActors.Add(Actor);
								}

								// Calculate min delta (max rate actor will upate), and max delta (slowest rate actor will update)
								const float MinOptimalDelta = 1.0f / Actor->NetUpdateFrequency;
								const float MaxOptimalDelta = FMath::Max(1.0f / Actor->MinNetUpdateFrequency, MinOptimalDelta);
								const float DeltaBetweenReplications = (World->TimeSeconds - ActorInfo->LastNetReplicateTime);

								// Choose an optimal time, we choose 70% of the actual rate to allow frequency to go up if needed
								ActorInfo->OptimalNetUpdateDelta = FMath::Clamp(DeltaBetweenReplications * 0.7f, MinOptimalDelta, MaxOptimalDelta);
								ActorInfo->LastNetReplicateTime = World->TimeSeconds;
							}
							ActorUpdatesThisConnection++;
							OutUpdated++;
						}
						else
						{
							UE_LOG(LogNetTraffic, Log, TEXT("- Channel saturated, forcing pending update for %s"), *Actor->GetName());
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
						UE_LOG(LogNetTraffic, Log, TEXT("- Closing channel for no longer relevant actor %s"), *Actor->GetName());
						Channel->Close();
					}
				}
			}
		}

		return FinalSortedCount;
	}

}


#endif