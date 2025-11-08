#include "Memory.hpp"
#include "ScriptVM.hpp"
#include "PipeServer.hpp"
#include <MinHook.h>

DWORD Main(PVOID)
{
	auto pattern = Memory::ScanPattern("49 63 41 1C");
	if (!pattern)
		return EXIT_FAILURE;

	auto addr = pattern->Sub(0x24).As<void*>();
	MH_Initialize();
	MH_CreateHook(addr, reinterpret_cast<void*>(RunScriptThread), nullptr);
	MH_EnableHook(addr);

	if (!PipeServer::Init("scrDbg"))
		return EXIT_FAILURE;

	PipeServer::Run();
	return EXIT_SUCCESS;
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