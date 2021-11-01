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
struct SpawnActorParams { unsigned char Params[0x40]; };
inline class UObject* (*SpawnActor)(class UObject* World, class UObject* Class, struct FVector* Position, struct FRotator* Rotation, SpawnActorParams params);
inline class UObject* (*StaticFindObject)(class UObject* Class, void* Outer, const wchar_t* Name, bool ExactClass);
inline class UObject* PlayerController;
inline class UObject* GameMode;
inline class UObject* UEngine;
inline class UObject* World;
inline class UObject* Pawn;
inline class UObject* GameState;
inline class UObject* GameStatics;
inline class UObject* CheatManager;
inline class UObject* WorldInventory;
inline class UObject* Quickbars;
inline class UObject* kismetMathLib;
inline class UObject* kismetGuidLib;
inline class UObject* WorldSettings;
inline vector<class UObject*> CharacterPartsArray;
inline bool bLoadedInMatch = false;
inline bool bDroppedFromAircraft = false;

#define _(STR) xorstr(STR).crypt_get()
