#include "include.hpp"

DWORD WINAPI Main(LPVOID lpParam) 
{
    Console::Create("~ Love from astroux ~");
    Console::Log("Hello!");

    Hooks::Load();

    while (!GetAsyncKeyState(VK_END)) 
        Sleep(100);

    Console::Log("Unloading...");

    Console::Destroy();
    Hooks::Unload();

	FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, Main, hModule, 0, nullptr);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

