#pragma once

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

inline void* (*PE)(class UObject* Object, class UObject* Function, PVOID Params) = nullptr;
inline class UObject* (*StaticFindObject)(class UObject* Class, void* Outer, const wchar_t* Name, bool ExactClass);
inline void* (*Realloc)(void* Block, SIZE_T NewSize, uint32 Alignment);
inline struct FString (*GetEngineVersion)();
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
inline class UObject* WorldSettings;
inline vector<class UObject*> CharacterPartsArray;
inline bool bLoadedInMatch = false;
inline bool bDroppedFromAircraft = false;
inline bool bPressedPlay = false;

namespace offsets
{
	inline int32 Children = 0x38;
	inline int32 Next = 0x28;
	inline int32 ParamsSize = 0x8E;
	inline int32 ReturnValueOffset = 0x90;
	inline int32 SuperClass = 0x30;
	inline int32 StructSize = 0x40;
}

#define _(STR) xorstr(STR).crypt_get()
