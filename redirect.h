#pragma once

#include <Windows.h>
#include <regex>
#include "sdk.h"
#include "memory.h"
#include "core.h"
#include "Athena.h"
#include "globals.h"

using namespace std;

namespace Redirect
{

	inline INT(*CurlSet)(LPVOID, INT, va_list) = 0;
	inline INT CurlSetHook(LPVOID A, INT B, ...)
	{
		va_list argument;
		va_start(argument, B);

		INT result = CurlSet(A, B, argument);

		va_end(argument);

		return result;
	}

	inline INT(*CurlEasy)(LPVOID, INT, ...) = 0;
	inline INT CurlEasyHook(LPVOID A, INT B, ...)
	{
		if (!A) return 43;

		INT returnValue = 0;
		const regex EpicURL(_("https:\\/\\/(.*)\\.ol\\.epicgames.com"));
		const string EraURL(_("http://erafnbackend.herokuapp.com"));

		va_list argument;
		va_list CopiedArgument;
		va_start(argument, B);
		va_copy(CopiedArgument, argument);

		if (B == 64) returnValue = CurlSetHook(A, B, 0);
		else if (B == 10002)
		{
			string Url(va_arg(CopiedArgument, PCHAR));

			if (regex_search(Url, EpicURL)) {
				Url = regex_replace(Url, EpicURL, EraURL);

				if (Url.find(_("/account/api/oauth/token")) != -1)
				{
					PGH::UnHook();
					DetourHook(&(void*&)CurlEasy, CurlEasyHook);
				}

				if (Url.find(_("/client/ClientQuestLogin")) != -1) {
					Core::InitializeHook();
					Core::InitializeGlobals();
				}

				if (Url.find(_("matchmakingservice")) != -1) PlayerController->Call(_("SwitchLevel"), FString(_(L"Athena_Terrain")));
			}

			returnValue = CurlSetHook(A, B, Url.c_str());
		}
		else if (B == 10004) returnValue = CurlSetHook(A, B, "");
		else returnValue = CurlSet(A, B, argument);

		va_end(argument);
		va_end(CopiedArgument);

		return returnValue;
	}
}
