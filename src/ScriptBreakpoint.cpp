#include "ScriptBreakpoint.hpp"

void ScriptBreakpoint::AddImpl(std::uint32_t script, std::uint32_t pc, bool pause)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return;
    }
    m_Breakpoints.push_back({ script, pc, 0, false, pause });
}

void ScriptBreakpoint::RemoveImpl(std::uint32_t script, std::uint32_t pc)
{
    auto it = std::remove_if(m_Breakpoints.begin(), m_Breakpoints.end(), [script, pc](const BreakPoint& bp) {
        return bp.Script == script && bp.Pc == pc;
    });
    m_Breakpoints.erase(it, m_Breakpoints.end());
}

void ScriptBreakpoint::ResumeImpl(std::uint32_t script, std::uint32_t pc)
{
    // TO-DO
}

void ScriptBreakpoint::IncrementHitCountImpl(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
        {
            ++bp.HitCount;
            break;
        }
    }
}

void ScriptBreakpoint::SkipImpl(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
        {
            bp.Skip = true;
            break;
        }
    }
}

std::uint32_t ScriptBreakpoint::GetHitCountImpl(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return bp.HitCount;
    }
    return 0;
}

bool ScriptBreakpoint::HasImpl(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return true;
    }
    return false;
}

bool ScriptBreakpoint::ShouldSkipImpl(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
        {
            bool skip = bp.Skip;
            bp.Skip = false; // reset after checking
            return skip;
        }
    }
    return false;
}

bool ScriptBreakpoint::ShouldPauseImpl(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return bp.Pause;
    }
    return false;
}

// Exports

extern "C"
{
    __declspec(dllexport) void ScriptBreakpointAdd(std::uint32_t script, std::uint32_t pc, bool pause)
    {
        ScriptBreakpoint::Add(script, pc, pause);
    }

    __declspec(dllexport) void ScriptBreakpointRemove(std::uint32_t script, std::uint32_t pc)
    {
        ScriptBreakpoint::Remove(script, pc);
    }

    __declspec(dllexport) void ScriptBreakpointResume(std::uint32_t script, std::uint32_t pc)
    {
        ScriptBreakpoint::Resume(script, pc);
    }

    __declspec(dllexport) std::uint32_t ScriptBreakpointGetHitCount(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::GetHitCount(script, pc);
    }

    __declspec(dllexport) bool ScriptBreakpointHas(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Has(script, pc);
    }
}