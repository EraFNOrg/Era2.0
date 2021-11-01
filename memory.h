#pragma once

#include <windows.h>
#include <psapi.h>
#include <vector> 
#include "detours.h"

namespace PGH {
	static unsigned __int64 oldfunc;
	static unsigned __int64 hookfunc;
	static DWORD Protection;
	static PVOID Handle;

	static long __stdcall Handler(EXCEPTION_POINTERS* Info)
	{
		if (Info->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
		{
			if (Info->ContextRecord->Rip == (unsigned __int64)oldfunc) Info->ContextRecord->Rip = (unsigned __int64)hookfunc;
			Info->ContextRecord->EFlags |= 0x100;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Info->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
		{
			DWORD dwOld;
			VirtualProtect((LPVOID)oldfunc, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &dwOld);
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}

	static void UnHook()
	{
		DWORD old;

		VirtualProtect((LPVOID)oldfunc, 1, Protection, &old);
		RemoveVectoredExceptionHandler(Handle);
	}

	static void Hook(unsigned __int64 oldfunction, unsigned __int64 hookfunction)
	{
		oldfunc = oldfunction;
		hookfunc = hookfunction;

		Handle = AddVectoredExceptionHandler(true, (PVECTORED_EXCEPTION_HANDLER)Handler);

		VirtualProtect((LPVOID)oldfunction, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &Protection);
	}
}

static __forceinline bool IsBadReadPtr(void* p)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) b = true;

		return b;
	}
	return true;
}

static __forceinline uintptr_t FindPattern(const char* signature, bool bRelative = false, uint32_t offset = 0)
{
	uintptr_t base_address = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
	static auto patternToByte = [](const char* pattern)
	{
		auto bytes = std::vector<int>{};
		const auto start = const_cast<char*>(pattern);
		const auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current)
		{
			if (*current == '?')
			{
				++current;
				if (*current == '?') ++current;
				bytes.push_back(-1);
			}
			else { bytes.push_back(strtoul(current, &current, 16)); }
		}
		return bytes;
	};

	const auto dosHeader = (PIMAGE_DOS_HEADER)base_address;
	const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)base_address + dosHeader->e_lfanew);

	const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = patternToByte(signature);
	const auto scanBytes = reinterpret_cast<std::uint8_t*>(base_address);

	const auto s = patternBytes.size();
	const auto d = patternBytes.data();

	for (auto i = 0ul; i < sizeOfImage - s; ++i)
	{
		bool found = true;
		for (auto j = 0ul; j < s; ++j)
		{
			if (scanBytes[i + j] != d[j] && d[j] != -1)
			{
				found = false;
				break;
			}
		}
		if (found)
		{
			uintptr_t address = reinterpret_cast<uintptr_t>(&scanBytes[i]);
			if (bRelative)
			{
				address = ((address + offset + 4) + *(int32_t*)(address + offset));
				return address;
			}
			return address;
		}
	}
	return NULL;
}

inline void DetourHook(void** ppPointer, void* pDetour)
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(ppPointer, pDetour);

	DetourTransactionCommit();
}
