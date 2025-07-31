#include "Memory.hpp"
#include "ScriptVM.hpp"
#include <MinHook.h>

DWORD Main(PVOID)
{
	if (auto pattern = Memory::ScanPattern("49 63 41 1C"))
	{
		auto addr = pattern->Sub(0x24).As<void*>();
		MH_Initialize();
		MH_CreateHook(addr, reinterpret_cast<void*>(RunScriptThread), nullptr);
		MH_EnableHook(addr);
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}

BOOL WINAPI DllMain(HINSTANCE dllInstance, DWORD reason, PVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(dllInstance);
		CreateThread(nullptr, 0, Main, nullptr, 0, nullptr);
	}

	return TRUE;
}