#pragma once

#include <map>
#include <windows.h>
#include <vector>
#include "Libraries/xorstr.hpp"

using namespace std;

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

struct SpawnActorParams
{
	unsigned char Parms[0x40];
};

inline void* (*PE)(class UObject* Object, class UObject* Function, PVOID Params) = nullptr;
inline class UObject* (*StaticFindObject)(class UObject* Class, void* Outer, const wchar_t* Name, bool ExactClass);
inline void* (*Realloc)(void* Block, SIZE_T NewSize, uint32 Alignment);
inline class UObject* (*SpawnActor)(class UObject* World, class UObject* Class, struct FVector* Location, struct FRotator* Rotation, const SpawnActorParams& Params);
inline bool (*GetDataTableRow)(class UObject* DataTable, struct FName Name, void* OutRowPtr);
inline void* (*CopyScriptStruct)(class UObject* Struct, void* OutPtr, void* InPtr, int ArraySize);
inline class UObject* (*StaticLoadObject)(class UObject*, class UObject*, const wchar_t*, const wchar_t*, int, void*, bool);
inline void (*GenericArray_Remove)(PVOID TargetArray, PVOID Property, int ItemIndex);
inline struct FString (*GetEngineVersion)();
inline struct GObjects* GObjectArray;
inline class UObject* PlayerController;
inline class UObject* GameMode;
inline class UObject* UEngine;
inline class UObject* GameViewportClient;
inline class UObject* World;
inline class UObject* Pawn;
inline class UObject* GameState;
inline class UObject* GameStatics;
inline class UObject* CheatManager;
inline class UObject* WorldInventory;
inline class UObject* Quickbars;
inline class UObject* kismetMathLib;
inline class UObject* kismetGuidLib;
inline class UObject* kismetStringLib;
inline class UObject* kismetSystemLib;
inline class UObject* DataTableFunctionLibrary;
inline class UObject* WorldSettings;
inline class UObject* EditToolItem;
inline class UObject* Wall;
inline class UObject* Floor;
inline class UObject* Stairs;
inline class UObject* Roof;
inline vector<class UObject*> CharacterPartsArray;
inline map<string, class UObject*> CallCache;
inline map<string, int32> OffsetCache;
inline map<pair<PVOID, string>, int32> StructOffsetCache;
inline bool bLoadedInMatch = false;
inline bool bDroppedFromAircraft = false;
inline bool bPressedPlay = false;
inline bool bInFrontend = true;
inline bool bCoreInitialized = false;
inline bool bInfiniteAmmo = true;
inline bool bIsEmoting = false;
inline bool g_bIsVehicleVersion = false;
inline bool l_bIsOutsideSafeZone = false;

namespace offsets
{
	inline int32 Class = 0x70;
	inline int32 Offset = 0x44;
	inline int32 Children = 0x38;
	inline int32 Next = 0x28;
	inline int32 ParamsSize = 0x8E;
	inline int32 ReturnValueOffset = 0x90;
	inline int32 SuperClass = 0x30;
	inline int32 StructSize = 0x40;
}

#define _(STR) xorstr(STR).crypt_get()
