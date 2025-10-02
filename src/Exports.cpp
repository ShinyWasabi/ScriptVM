#include "ScriptBreakpoint.hpp"

extern "C"
{
    __declspec(dllexport) bool ScriptBreakpointAdd(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Add(script, pc);
    }

    __declspec(dllexport) bool ScriptBreakpointRemove(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Remove(script, pc);
    }

    __declspec(dllexport) bool ScriptBreakpointRemoveAll()
    {
        return ScriptBreakpoint::RemoveAll();
    }

    __declspec(dllexport) bool ScriptBreakpointRemoveAllScript(std::uint32_t script)
    {
        return ScriptBreakpoint::RemoveAllScript(script);
    }

    __declspec(dllexport) bool ScriptBreakpointResume(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Resume(script, pc);
    }

    __declspec(dllexport) bool ScriptBreakpointExists(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Exists(script, pc);
    }

    __declspec(dllexport) bool ScriptBreakpointIsActive(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::IsActive(script, pc);
    }

    __declspec(dllexport) int ScriptBreakpointGetCount()
    {
        return ScriptBreakpoint::GetCount();
    }

    __declspec(dllexport) int ScriptBreakpointGetCountScript(std::uint32_t script)
    {
        return ScriptBreakpoint::GetCountScript(script);
    }

    __declspec(dllexport) int ScriptBreakpointGetHitCount(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::GetHitCount(script, pc);
    }
}