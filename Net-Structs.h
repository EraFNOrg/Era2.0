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
			FORCEINLINE bool operator==(const FBitIterator& otherIt) const
			{
				return Index == otherIt.Index;
			}
			FORCEINLINE bool operator!=(const FBitIterator& otherIt) const
			{
				return Index != otherIt.Index;
			}
			FORCEINLINE FBitIterator& operator=(const FBitIterator& other)
			{
				IteratedArray = other.IteratedArray;
				Index = other.Index;
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
			return FBitIterator(*this, 0);
		}
		FORCEINLINE FBitIterator end()
		{
			return FBitIterator(*this);
		}
		FORCEINLINE FBitIterator end() const
		{
			return FBitIterator(*this);
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
		typedef TSparseArrayElementOrListLink<ArrayType> FSparseArrayElement;

	private:
		TArray<FSparseArrayElement> Data;
		TBitArray<TAllocatorBase<int32>> AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:
		class FBaseIterator
		{

			const TSparseArray<ArrayType>& IteratedArray;
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
			FORCEINLINE FSparseArrayElement& operator*()
			{
				return IteratedArray[BitArrayIt.GetIndex()];
			}
			FORCEINLINE FSparseArrayElement& operator->()
			{
				return IteratedArray[BitArrayIt.GetIndex()];
			}
			FORCEINLINE bool operator==(const FBaseIterator& other)
			{
				return BitArrayIt.GetIndex() == other.BitArrayIt.GetIndex();
			}
			FORCEINLINE bool operator!=(const FBaseIterator& other)
			{
				return BitArrayIt.GetIndex() != other.BitArrayIt.GetIndex();
			}
		};

	public:
		FORCEINLINE FBaseIterator begin()
		{
			return FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags, 0));
		}
		FORCEINLINE FBaseIterator begin() const
		{
			return FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags, 0));
		}
		FORCEINLINE FBaseIterator end()
		{
			return FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags));
		}
		FORCEINLINE FBaseIterator end() const
		{
			return FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags));
		}

		FORCEINLINE FSparseArrayElement& operator[](uint32 Index)
		{
			return (FSparseArrayElement*)Data[Index];
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
			TSparseArray::FBaseIterator ElementIt;

		public:
			FORCEINLINE FBaseIterator(const TSparseArray::FBaseIterator& InElementIt)
				: ElementIt(InElementIt)
			{
			}

			FORCEINLINE FBaseIterator& operator++()
			{
				return ElementIt++;
			}
			FORCEINLINE bool operator==(FBaseIterator& otherIt)
			{
				return ElementIt == otherIt.ElementIt;
			}
			FORCEINLINE bool operator!=(FBaseIterator& otherIt)
			{
				return ElementIt != otherIt.ElementIt;
			}
			FORCEINLINE FBaseIterator& operator=(FBaseIterator& otherIt)
			{
				return ElementIt = otherIt.ElementIt;
			}
			FORCEINLINE TSparseArray<SetType>::FSparseArraySetType operator*()
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

	struct FActorPriority
	{
		int32 Priority;		// Update priority, higher = more important.

		FNetworkObjectInfo* ActorInfo;	// Actor info.
		UObject* Channel;				// Actor channel.

		FActorDestructionInfo* DestructionInfo;	// Destroy an actor

		FActorPriority(UObject* InConnection, UObject* InChannel, FNetworkObjectInfo* InActorInfo, const TArray<struct FNetViewer>& Viewers, bool bLowBandwidth)
			: ActorInfo(InActorInfo), Channel(InChannel), DestructionInfo(NULL)
		{
			float Time = Channel ? (InConnection->Child(_("Driver"))->Child<float>(_("Time")) - Channel->LastUpdateTime) : InConnection->Driver->SpawnPrioritySeconds;
			// take the highest priority of the viewers on this connection
			Priority = 0;
			for (int32 i = 0; i < Viewers.Num(); i++)
			{
				Priority = FMath::Max<int32>(Priority, FMath::RoundToInt(65536.0f * ActorInfo->Actor->GetNetPriority(Viewers[i].ViewLocation, Viewers[i].ViewDir, Viewers[i].InViewer, Viewers[i].ViewTarget, InChannel, Time, bLowBandwidth)));
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

				Priority = Priority >= (65536.0f * Time) ? Priority : (65536.0f * Time);
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