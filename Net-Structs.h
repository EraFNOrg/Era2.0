#pragma once
#include "Net-Globals.h"

#if defined(SERVER)

namespace Net
{
	template<typename ElementType>
	class TAllocatorBase
	{
		ElementType* Elements;

	public:
		template<typename Type = void>
		FORCEINLINE Type* GetAllocation()
		{
			return (Type*)Elements;
		}

		template<typename Type = void>
		FORCEINLINE const Type* GetAllocation() const
		{
			return (const Type*)Elements;
		}
	};

	template<class AllocatorType = TAllocatorBase<uint32>>
	class TBitArray
	{

	private:
		AllocatorType AllocatorInstance;
		int32 NumBits;
		int32 MaxBits;

		class FRelativeBitReference
		{
		public:
			FORCEINLINE explicit FRelativeBitReference(int32 BitIndex)
				: DWORDIndex(BitIndex >> ((int32)5))
				, Mask(1 << (BitIndex & (((int32)32) - 1)))
			{
			}

			int32  DWORDIndex;
			uint32 Mask;
		};

	public:
		class FBitIterator : public FRelativeBitReference
		{
			int32 Index;
			const TBitArray<AllocatorType>& IteratedArray;

		public:
			FORCEINLINE FBitIterator(TBitArray<AllocatorType>& ToIterate, int32 StartIndex)
				: IteratedArray(ToIterate), Index(StartIndex)
			{
			}
			FORCEINLINE FBitIterator(TBitArray<AllocatorType>& ToIterate)
				: IteratedArray(ToIterate), Index(ToIterate.MaxBits)
			{
			}

			FORCEINLINE explicit operator bool()
			{
				return Index < IteratedArray.Num();
			}
			FORCEINLINE FBitIterator& operator++()
			{
				++Index;
				this->Mask <<= 1;
				if (!this->Mask)
				{
					this->Mask = 1;
					++this->DWORDIndex;
				}
				return *this;
			}
			FORCEINLINE bool operator*() const
			{
				return IteratedArray.ByIndex(Index);
			}
			FORCEINLINE bool operator==(const FBitIterator& OtherIt) const
			{
				return Index == OtherIt.Index;
			}
			FORCEINLINE bool operator!=(const FBitIterator& OtherIt) const
			{
				return Index != OtherIt.Index;
			}
			FORCEINLINE FBitIterator& operator=(const FBitIterator& Other)
			{
				IteratedArray = Other.IteratedArray;
				Index = Other.Index;
			}

			FORCEINLINE int32 GetIndex() const
			{
				return Index;
			}
		};

	public:
		FORCEINLINE FBitIterator begin()
		{
			return FBitIterator(*this, 0);
		}
		FORCEINLINE FBitIterator begin() const
		{
			return const FBitIterator(*this, 0);
		}
		FORCEINLINE FBitIterator end()
		{
			return FBitIterator(*this);
		}
		FORCEINLINE FBitIterator end() const
		{
			return const FBitIterator(*this);
		}

		FORCEINLINE int32 Num() const
		{
			return NumBits;
		}
		FORCEINLINE int32 Max() const
		{
			return MaxBits;
		}
		FORCEINLINE bool ByIndex(int32 Index) const
		{
			return (AllocatorInstance.GetAllocation()[Index / 8] <<= this->Mask) & 1;
		}
	};

	template<typename ElementType>
	union TSparseArrayElementOrListLink
	{
		/** If the element is allocated, its value is stored here. */
		ElementType ElementData;

		struct
		{
			/** If the element isn't allocated, this is a link to the previous element in the array's free list. */
			int32 PrevFreeIndex;

			/** If the element isn't allocated, this is a link to the next element in the array's free list. */
			int32 NextFreeIndex;
		};
	};

	template<typename ArrayType>
	class TSparseArray
	{
	public:
		typedef TSparseArrayElementOrListLink<ArrayType> FSparseArrayElement;

	private:
		TArray<FSparseArrayElement> Data;
		TBitArray<TAllocatorBase<int32>> AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:
		class FBaseIterator
		{

			TSparseArray<ArrayType>& IteratedArray;
			TBitArray<TAllocatorBase<int32>> BitArrayIt;

		public:
			FORCEINLINE FBaseIterator(const TSparseArray<ArrayType>& Array, TBitArray<TAllocatorBase<int32>>& BitIterator)
				: IteratedArray(Array), BitArrayIt(BitIterator)
			{
			}

			FORCEINLINE FBaseIterator& operator++()
			{
				while (true)
				{
					++BitArrayIt;
					
					if (*BitArrayIt)
						break;
				}
				return *this;
			}
			FORCEINLINE ArrayType& operator*()
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE const ArrayType& operator*() const
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE ArrayType& operator->()
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE const ArrayType& operator->() const
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE bool operator==(const FBaseIterator& Other) const
			{
				return BitArrayIt.GetIndex() == Other.BitArrayIt.GetIndex();
			}
			FORCEINLINE bool operator!=(const FBaseIterator& Other) const
			{
				return BitArrayIt.GetIndex() != Other.BitArrayIt.GetIndex();
			}
		};

	public:
		FORCEINLINE FBaseIterator begin()
		{
			return FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags, 0));
		}
		FORCEINLINE FBaseIterator begin() const
		{
			return const FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags, 0));
		}
		FORCEINLINE FBaseIterator end()
		{
			return FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags));
		}
		FORCEINLINE FBaseIterator end() const
		{
			return const FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags));
		}

		FORCEINLINE FSparseArrayElement& operator[](uint32 Index)
		{
			return (FSparseArrayElement*)Data[Index];
		}
		FORCEINLINE const FSparseArrayElement& operator[](uint32 Index) const
		{
			return (const FSparseArrayElement*)Data[Index];
		}
	};
	
	template<typename SetType>
	class TSet
	{
		TSparseArray<SetType> Elements;

		mutable void* Hash;
		mutable int32 HashSize;

		class FBaseIterator
		{
			TSparseArray<SetType>::FBaseIterator& ElementIt;

		public:
			FORCEINLINE FBaseIterator(TSparseArray<SetType>::FBaseIterator& InElementIt)
				: ElementIt(InElementIt)
			{
			}

			FORCEINLINE FBaseIterator& operator++()
			{
				return ElementIt++;
			}
			FORCEINLINE bool operator==(const FBaseIterator& OtherIt) const
			{
				return ElementIt == OtherIt.ElementIt;
			}
			FORCEINLINE bool operator!=(const FBaseIterator& OtherIt) const
			{
				return ElementIt != OtherIt.ElementIt;
			}
			FORCEINLINE FBaseIterator& operator=(FBaseIterator& OtherIt)
			{
				return ElementIt = OtherIt.ElementIt;
			}
			FORCEINLINE SetType& operator*()
			{
				return *ElementIt; 
			}
			FORCEINLINE const SetType& operator*() const
			{
				return *ElementIt;
			}
			FORCEINLINE SetType& operator->()
			{
				return *ElementIt;
			}
			FORCEINLINE const SetType& operator->() const
			{
				return *ElementIt;
			}
		};

	public:
		FORCEINLINE FBaseIterator begin()
		{
			return TBaseIterator(Elements.begin());
		}
		FORCEINLINE FBaseIterator begin() const
		{
			return TBaseIterator(Elements.begin());
		}
		FORCEINLINE FBaseIterator end()
		{
			return TBaseIterator(Elements.end());
		}
		FORCEINLINE FBaseIterator end() const
		{
			return TBaseIterator(Elements.end());
		}
	};

	template<typename KeyType, typename ValueType>
	class TPair
	{
	public:
		KeyType First;
		ValueType Second;

		FORCEINLINE KeyType& Key()
		{
			return First;
		}
		FORCEINLINE const KeyType& Key() const
		{
			return First;
		}
		FORCEINLINE ValueType& Value()
		{
			return Second;
		}
		FORCEINLINE const ValueType& Value() const
		{
			return Second;
		}
	};

	template<typename KeyType, typename ValueType>
	class TMap
	{
		typedef TPair<KeyType, ValueType> ElementType;

		ElementType Pairs;

		class FBaseIterator
		{
			TMap<KeyType, ValueType>& ItMap;
			TSet<ElementType>::FBaseIterator SetIt;

			FBaseIterator(TMap<KeyType, ValueType>& Map)
				: ItMap(Map)
			{
			}
			FORCEINLINE ElementType& operator*()
			{

			}
			FORCEINLINE const ElementType& operator*() const
			{
				return *SetIt;
			}
			FORCEINLINE bool operator==(const FBaseIterator& Other) const
			{
				return SetIt == Other.SetIt;
			}
			FORCEINLINE bool operator!=(const FBaseIterator& Other) const
			{
				return SetIt != Other.SetIt;
			}
		};

		template<typename ComparisonFunction>
		FORCEINLINE ValueType& operator[](const KeyType& Key, ComparisonFunction* comp = nullptr)
		{
			if (comp)
			{
				for (ElementType Pair : *this)
				{
					if (comp(Pair.First, Key))
					{
						return Pair.Second;
					}
				}
			}
			else
			{
				for (ElementType Pair : *this)
				{
					if (Pair.Key == Key)
					{
						return Pair.Second;
					}
				}
			}			
		}

	};
	
	template<class ObjectType>
	class TSharedPtr
	{
	public:
		ObjectType* Object;

		int32_t SharedReferenceCount;
		int32_t WeakReferenceCount;
	};

	template<class PtrType>
	class TWeakObjectPtr
	{
		int32 ObjectIndex;
		int32 ObjectSerialNumber;

		// FISCHSALAT: I'd implement a get function to get be able to "dereference" the pointer, but well, danii removed GObjects
	};

	struct FActorDestructionInfo
	{
		TWeakObjectPtr<UObject>		Level;
		TWeakObjectPtr<UObject>		ObjOuter;
		FVector						DestroyedPosition;
		uint32						NetGUID;
		FString						PathName;

		FName						StreamingLevelName;
	};

	struct FNetViewer
	{
		UObject* Connection;
		UObject* InViewer;
		UObject* ViewTarget;
		FVector ViewLocation;
		FVector ViewDir;

		FNetViewer()
			: Connection(0)
			, InViewer(0)
			, ViewTarget(0)
			, ViewLocation(FVector())
			, ViewDir(FVector())
		{
		}

		FNetViewer(UObject* InConnection, float DeltaSeconds) {
			auto ViewingController = InConnection->Child<UObject*>("PlayerController");

			// Get viewer coordinates.
			ViewLocation = ViewTarget->Call<FVector>("K2_GetActorLocation");
			if (ViewingController)
			{
				FRotator ViewRotation = ViewingController->Child<FRotator>("ControlRotation");
				//ViewingController->GetPlayerViewPoint(ViewLocation, ViewRotation);
				//ViewDir = ViewRotation.Vector();
				// that's all for now
			}
		}
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

	struct FActorPriority
	{
		int32 Priority;		// Update priority, higher = more important.

		FNetworkObjectInfo* ActorInfo;	// Actor info.
		UObject* Channel;				// Actor channel.

		FActorDestructionInfo* DestructionInfo;	// Destroy an actor

		FActorPriority() 
			: Priority(0), ActorInfo(NULL), Channel(NULL), DestructionInfo(NULL)
		{
		}

		FActorPriority(UObject* InConnection, UObject* InChannel, FNetworkObjectInfo* InActorInfo, const TArray<struct FNetViewer>& Viewers, bool bLowBandwidth)
			: ActorInfo(InActorInfo), Channel(InChannel), DestructionInfo(NULL)
		{
			float Time = Channel ? (InConnection->Child(_("Driver"))->Child<float>(_("Time")) - Channel->LastUpdateTime) : InConnection->Child(_("Driver"))->Child<float>(_("SpawnPrioritySeconds"));
			// take the highest priority of the viewers on this connection
			Priority = 0;
			//for (int32 i = 0; i < Viewers.Num(); i++)
			static auto RoundToInt = [](float F) { return _mm_cvt_ss2si(_mm_set_ss(F + F + 0.5f)) >> 1; };

			for (auto Viewer : Viewers)
			{
				Priority =MAX(Priority, RoundToInt(65536.0f * Globals::ActorGetNetPriority(ActorInfo->Actor, Viewer.ViewLocation, Viewer.ViewDir, Viewer.InViewer, Viewer.ViewTarget, InChannel, Time, bLowBandwidth)));
			}
		}

		FActorPriority(class UObject* InConnection, struct FActorDestructionInfo* Info, const TArray<struct FNetViewer>& Viewers)
			: ActorInfo(NULL), Channel(NULL), DestructionInfo(Info)
		{

			Priority = 0;

			for (int32 i = 0; i < Viewers.Num(); i++)
			{
				float Time = InConnection->Child(_("Driver"))->Child<float>(_("SpawnPrioritySeconds"));

				FVector Dir = DestructionInfo->DestroyedPosition - Viewers[i].ViewLocation;
				float DistSq = Dir.SizeSquared();

				// adjust priority based on distance and whether actor is in front of viewer
				if ((Viewers[i].ViewDir | Dir) < 0.f)
				{
					if (DistSq > (2000.f* 2000.f))
						Time *= 0.2f;
					else if (DistSq > (500.0f*500.0f))
						Time *= 0.4f;
				}
				else if (DistSq > (3162.0f* 3162.0f))
					Time *= 0.4f;

				Priority = MAX(Priority, 65536.0f * Time);
			}
		}
	};



	/** Structure to hold and pass around transient flags used during replication. */
	struct FReplicationFlags
	{
		union
		{
			struct
			{
				/** True if replicating actor is owned by the player controller on the target machine. */
				uint32 bNetOwner : 1;
				/** True if this is the initial network update for the replicating actor. */
				uint32 bNetInitial : 1;
				/** True if this is actor is RemoteRole simulated. */
				uint32 bNetSimulated : 1;
				/** True if this is actor's ReplicatedMovement.bRepPhysics flag is true. */
				uint32 bRepPhysics : 1;
				/** True if this actor is replicating on a replay connection. */
				uint32 bReplay : 1;
				/** True if this actor's RPCs should be ignored. */
				uint32 bIgnoreRPCs : 1;
			};

			uint32	Value;
		};
		FReplicationFlags()
		{
			Value = 0;
		}
	};


	static_assert(sizeof(FReplicationFlags) == 4, "FReplicationFlags has invalid size.");

	enum class ENetRole : uint8
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


	/**
	* Utility class to capture time passed in seconds, adding delta time to passed
	* in variable. Not useful for reentrant functions
	*/
	class FSimpleScopeSecondsCounter
	{
	public:
		/** Ctor, capturing start time. */
		FSimpleScopeSecondsCounter(double& InSeconds, bool bInEnabled = true)
			: StartTime(FPlatformTime::Seconds())
			, Seconds(InSeconds)
			, bEnabled(bInEnabled)
			, RecursionDepth(nullptr)
		{}
		/** Ctor, capturing start time. */
		FSimpleScopeSecondsCounter(double& InSeconds, int32* InRecursionDepth)
			: StartTime(FPlatformTime::Seconds())
			, Seconds(InSeconds)
			, bEnabled(*InRecursionDepth == 0)
			, RecursionDepth(InRecursionDepth)
		{
			(*RecursionDepth)++;
		}
		/** Dtor, updating seconds with time delta. */
		~FSimpleScopeSecondsCounter()
		{
			if (bEnabled)
			{
				Seconds += FPlatformTime::Seconds() - StartTime;
			}

			if (RecursionDepth)
			{
				(*RecursionDepth)--;
			}
		}
	private:
		/** Start time, captured in ctor. */
		double StartTime;
		/** Time variable to update. */
		double& Seconds;
		/** Is the timer enabled or disabled */
		bool bEnabled;
		/** Recursion depth */
		int32* RecursionDepth;
	};

	// Helper class to downgrade a non owner of an actor to simulated while replicating
	class FScopedRoleDowngrade
	{
	public:
		FScopedRoleDowngrade(UObject* InActor, const FReplicationFlags RepFlags) : Actor(InActor), ActualRemoteRole(Actor->Child<ENetRole>(_("RemoteRole")))
		{
			// If this is actor is autonomous, and this connection doesn't own it, we'll downgrade to simulated during the scope of replication
			if (ActualRemoteRole == ENetRole::ROLE_AutonomousProxy)
			{
				if (!RepFlags.bNetOwner)
				{
					Actor->SetAutonomousProxy(false, false);
				}
			}
		}

		~FScopedRoleDowngrade()
		{
			// Upgrade role back to autonomous proxy if needed
			if (Actor->Child<ENetRole>(_("RemoteRole")) != ActualRemoteRole)
			{
				Actor->SetReplicates(ActualRemoteRole != ENetRole::ROLE_None);

				if (ActualRemoteRole == ENetRole::ROLE_AutonomousProxy)
				{
					Actor->SetAutonomousProxy(true, false);
				}
			}
		}

	private:
		UObject* Actor;
		const ENetRole	ActualRemoteRole;
	};

}


#endif