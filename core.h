#pragma once

#include <windows.h>
#include <string>
#include <iostream>

class Core
{
public:
	static void Setup();
	static void InitializeHook();
	static void InitializeGlobals();
	static void OnReadyToStartMatch();
	static void OnServerLoadingScreenDropped();
	static class UObject* SpawnActorEasy(class UObject* Class, struct FVector Location);
};