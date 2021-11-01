#pragma once
#include "sdk.h"
#include "Net-Structs.h"

#if defined(SERVER)

namespace Net
{
	int32 ServerReplicateActors_PrepConnections(UObject* NetDriver, float DeltaSeconds)
	{
		TArray<UObject*> ClientConnections = NetDriver->Child<TArray<UObject*>>("ClientConnections");

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

			auto OwningActor = Connection->Child<UObject*>("OwningActor");

			auto Driver = Connection->Child<UObject*>("Driver");
			auto Time = Driver->Child<float>("Time");
			auto LastRecieveTime = Connection->Child<double>("LastReceiveTime");

			TArray<UObject*> ChildConnections = Connection->Child<TArray<UObject*>>("Children");

			if (OwningActor && Time - LastRecieveTime < 1.5f)
			{
				printf(_("valid owningactor and time.\n"));

				bFoundReadyConnection = true;

				auto DesiredViewTarget = OwningActor;

				auto ConnectionPlayerController = Connection->Child<UObject*>("PlayerController");

				if (ConnectionPlayerController)
				{
					if (UObject* ViewTarget = ConnectionPlayerController->Call<UObject*>("GetViewTarget"))
					{
						DesiredViewTarget = ViewTarget;
					}
				}

				Connection->Child<UObject*>("ViewTarget") = DesiredViewTarget;

				for (int ChildIdx = 0; ChildIdx < ChildConnections.Num(); ChildIdx++)
				{
					auto Child = ChildConnections[ChildIdx];

					auto ChildsPlayerController = Child->Child<UObject*>("PlayerController");

					if (ChildsPlayerController)
					{
						Child->Child<UObject*>("ViewTarget") = ChildsPlayerController->Call<UObject*>("GetViewTarget");
					}
					else
					{
						Child->Child<UObject*>("ViewTarget") = NULL;
					}
				}
			}
			else
			{
				Connection->Child<UObject*>("ViewTarget") = NULL;

				for (int ChildIdx = 0; ChildIdx < ChildConnections.Num(); ChildIdx++)
				{
					auto Child = ChildConnections[ChildIdx];

					Child->Child<UObject*>("ViewTarget") = NULL;
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

		for (const TSharedPtr<FNetworkObjectInfo>& ObjectInfo : TSet<int>()) // ?????????
		{
			auto ActorInfo = ObjectInfo.Object;

			auto WorldTimeSeconds = GameStatics->Call<float>("GetTimeSeconds");

			if (!ActorInfo->bPendingNetUpdate && WorldTimeSeconds <= ActorInfo->NextUpdateTime)
			{
				continue;
			}

			auto Actor = ActorInfo->Actor;

			auto ActorNetUpdateFrequency = Actor->Child<float>("NetUpdateFrequency");
			auto ActorMinNetUpdateFrequency = Actor->Child<float>("MinNetUpdateFrequency");

			if (Actor->Child<ENetRole>("RemoteRole") == ENetRole::ROLE_None)
			{
				printf(_("Actor added to 'ActorsToRemove' list cause it didn't have a NetRole\n"));
				ActorsToRemove.Add(Actor);
			}

			if (Actor->Child<FName>("NetDriverName") != NetDriver->Child<FName>("NetDriverName"));
			{
				printf(_("Actor added to 'ActorsToRemove' list cause it belongs to a different NetDriver\n"));
				ActorsToRemove.Add(Actor);
			}

			UObject* Level = Actor->Outer; // Actor->Outer

			auto CheckLevel = [&](){return Level->Child("OwningWorld")&&(Level == Level->Child("OwningWorld")->Child("CurrentLevelPendingVisibility")|| Level == Level->Child("OwningWorld")->Child("CurrentLevelPendingInvisibility"));};

			if (!Level || CheckLevel())
			{
				continue;
			}

			if (Actor && Globals::ActorIsNetStartupActor(Actor) && Actor->Child<ENetDormancy>("NetDormancy") == ENetDormancy::DORM_Initial)
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

				ActorInfo->LastNetUpdateTime = NetDriver->Child<float>("Time");
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

}


#endif