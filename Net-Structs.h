#pragma once
#include "Net-Globals.h"

#if defined(SERVER)

namespace Net
{
	template<typename ElementType>
	class TAllocatorBase
	{
		void* LinearAllocator;
		ElementType* Elements;

	public:
		template<typename Type = void>
		inline Type* GetAllocation()
		{
			return (Type*)Elements;
		}

		template<typename Type = void>
		inline const Type* GetAllocation() const 
		{
			return (const Type*)Elements;
		}
	};

	template<class AllocatorType = TAllocatorBase<uint32>>
	class TBitArray
	{
		AllocatorType AllocatorInstance;
		int32 NumBits;
		int32 MaxBits;

		class FRelativeBitReference
		{
		public:
			__forceinline explicit FRelativeBitReference(int32 BitIndex)
				: DWORDIndex(BitIndex >> ((int32)5))
				, Mask(1 << (BitIndex & (((int32)32) - 1)))
			{
			}

			int32  DWORDIndex;
			uint32 Mask;
		};

		class FBitIterator : public FRelativeBitReference
		{
			int32 Index;
			TBitArray<AllocatorType>& IteratedArray;
			
			FBitIterator(TBitArray<AllocatorType>& ToIterate, int32 StartIndex)
				: IteratedArray(ToIterate), Index(StartIndex)
			{
			}
			FBitIterator(TBitArray<AllocatorType>& ToIterate)
				: IteratedArray(ToIterate), Index(ToIterate.MaxBits)
			{
			}

			inline explicit operator bool()
			{
				return Index < IteratedArray.Num();
			}
			inline FBitIterator& operator++()
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
			inline bool operator*() const
			{
				return IteratedArray.ByIndex(Index);
			}
			inline bool operator==(const FBitIterator& otherIt) const
			{
				return Index == otherIt.Index;
			}
			inline bool operator!=(const FBitIterator& otherIt) const
			{
				return Index != otherIt.Index;
			}
			inline FBitIterator& operator=(const FBitIterator& other)
			{
				IteratedArray = other.IteratedArray;
				Index = other.Index;
			}

			inline int32 GetIndex() const
			{
				return Index;
			}
		};

		FBitIterator begin()
		{
			return FBitIterator(*this, 0);
		}
		FBitIterator begin() const
		{
			return FBitIterator(*this, 0);
		}
		FBitIterator end()
		{
			return FBitIterator(*this);
		}
		FBitIterator end() const
		{
			return BitArrayBaseIterator(*this);
		}

		inline int32 Num() const
		{
			return NumBits;
		}
		inline int32 Max() const
		{
			return MaxBits;
		}
		inline bool ByIndex() const
		{
			return (AllocatorInstance.GetAllocation()[this->DWORDIndex] <<= this->Mask) & 1;
		}
	};

	template<typename ArrayType>
	class TSparseArray
	{
		template<bool bConst>
		class TBaseIterator
		{

		};
	};
	/*
	template<typename SetType>
	class TSet
	{

		template<typename IteratorType>
		class TBaseIterator
		{
			TBaseIterator()
				:
			{
			}

			inline TBaseIterator<IteratorType>& operator++()
			{

			}
			inline TBaseIterator<IteratorType>& operator++(int32)
			{

			}
			inline bool operator==(TBaseIterator<IteratorType>& otherIt)
			{

			}
			inline bool operator!=(TBaseIterator<IteratorType>& otherIt)
			{

			}
			inline TBaseIterator<IteratorType>& operator=(TBaseIterator<IteratorType>& other)
			{

			}

		};


		TBaseIterator begin()
		{
			return TBaseIterator(*this, 0);
		}
		TBaseIterator begin() const
		{
			return TBaseIterator(*this, 0);
		}
		TBaseIterator end()
		{
			return TBaseIterator(*this);
		}
		TBaseIterator end() const
		{
			return TBaseIterator(*this);
		}

	};
	*/
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