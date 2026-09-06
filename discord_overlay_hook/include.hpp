#pragma once
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <d3d11.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")

// For DX11, I wanna end up making a vulkan hook eventually
#if defined(_WIN64)
#define DISCORD_DLL "DiscordHook64.dll"
#define DISCORD_PRESENT "55 41 57 41 56 56 57 53 48 83 EC 38 48 8D 6C 24 ? 44 89 C6"
#else
#define DISCORD_DLL "DiscordHook.dll"
#define DISCORD_PRESENT "55 89 E5 53 57 56 83 EC 28 8B 5D ? 8B 75"
#endif

namespace Console
{
	FILE* stream = nullptr;

	void Create(const char* title)
	{
		AllocConsole();
		SetConsoleTitleA(title);
		freopen_s(&stream, "CONOUT$", "w", stdout);
	}

	void Destroy()
	{
		if (stream)
			fclose(stream);

        HWND console = GetConsoleWindow();
        if (console)
            PostMessageA(console, WM_CLOSE, 0, 0);

		FreeConsole();
	}

	void Log(const char* message, ...)
	{
		if (!stream)
			return;

        char format[512];
        snprintf(format, sizeof(format), "%s\n", message);

        va_list args;
        va_start(args, message);
        vfprintf(stream, format, args);
        va_end(args);
	}
}

namespace Sig
{
    std::uintptr_t Scan(const char* moduleName, const char* signature)
    {
        auto getModuleBounds = [](const char* modName, std::uintptr_t& base, std::uintptr_t& size) {
            MODULEINFO modInfo = { 0 };
            HMODULE hModule = GetModuleHandleA(modName);
            if (!hModule) return false;
            GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo));
            base = reinterpret_cast<std::uintptr_t>(modInfo.lpBaseOfDll);
            size = modInfo.SizeOfImage;
            return true;
        };

        std::uintptr_t moduleBase = 0;
        std::uintptr_t moduleSize = 0;
        if (!getModuleBounds(moduleName, moduleBase, moduleSize)) return 0;

        std::vector<int> patternBytes;
        std::stringstream ss(signature);
        std::string byteStr;

        while (ss >> byteStr) {
            if (byteStr == "?" || byteStr == "??") {
                patternBytes.push_back(-1);
            }
            else {
                patternBytes.push_back(std::stoi(byteStr, nullptr, 16));
            }
        }

        std::uint8_t* scanStart = reinterpret_cast<std::uint8_t*>(moduleBase);
        std::uint8_t* scanEnd = scanStart + moduleSize - patternBytes.size();

        for (std::uint8_t* i = scanStart; i < scanEnd; ++i) {
            bool found = true;
            for (std::size_t j = 0; j < patternBytes.size(); ++j) {
                if (patternBytes[j] != -1 && i[j] != patternBytes[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return reinterpret_cast<std::uintptr_t>(i);
            }
        }
        return 0;
    }
}

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "safetyhook/safetyhook.hpp"
#include "hooks.hpp"