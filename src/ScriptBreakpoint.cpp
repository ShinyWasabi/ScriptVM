#include "ScriptBreakpoint.hpp"
#include "rage/scrThread.hpp"

bool ScriptBreakpoint::OnBreakpoint(std::uint32_t script, std::uint32_t pc, rage::scrThreadContext* context)
{
    if (!context)
        return false;

    if (Exists(script, pc) && !IsActive(script, pc))
    {
        Increment(script, pc);
        if (ShouldPause(script, pc))
        {
            context->m_ProgramCounter = pc;
            Activate(script, pc);

            char message[256];
            std::snprintf(message, sizeof(message), "Breakpoint hit, paused script 0x%X at 0x%X!", script, pc);
            MessageBoxA(0, message, "ScriptVM", MB_OK);
            return true;
        }
    }

    return false;
}

bool ScriptBreakpoint::Add(std::uint32_t script, std::uint32_t pc, bool pause)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return false;
    }

    m_Breakpoints.push_back({ script, pc, 0, false, pause });
    return true;
}

bool ScriptBreakpoint::Remove(std::uint32_t script, std::uint32_t pc)
{
    auto oldSize = m_Breakpoints.size();
    auto it = std::remove_if(m_Breakpoints.begin(), m_Breakpoints.end(), [script, pc](const Breakpoint& bp) {
        return bp.Script == script && bp.Pc == pc && !bp.Active;
    });

    m_Breakpoints.erase(it, m_Breakpoints.end());
    return m_Breakpoints.size() < oldSize;
}

bool ScriptBreakpoint::Resume(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc && bp.Active)
        {
            if (auto thread = rage::scrThread::GetThread(script))
            {
                thread->m_Context.m_State = rage::scrThreadState::RUNNING;
                return true;
            }
        }
    }

    return false;
}

bool ScriptBreakpoint::Exists(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return true;
    }

    return false;
}

int ScriptBreakpoint::GetHitCount(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return bp.HitCount;
    }

    return 0;
}

void ScriptBreakpoint::Activate(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
        {
            bp.Active = true;
            break;
        }
    }
}

void ScriptBreakpoint::Increment(std::uint32_t script, std::uint32_t pc)
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

bool ScriptBreakpoint::IsActive(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
        {
            bool active = bp.Active;
            bp.Active = false;
            return active;
        }
    }

    return false;
}

bool ScriptBreakpoint::ShouldPause(std::uint32_t script, std::uint32_t pc)
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
    __declspec(dllexport) bool ScriptBreakpointAdd(std::uint32_t script, std::uint32_t pc, bool pause)
    {
        return ScriptBreakpoint::Add(script, pc, pause);
    }

    __declspec(dllexport) bool ScriptBreakpointRemove(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Remove(script, pc);
    }

    __declspec(dllexport) bool ScriptBreakpointResume(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Resume(script, pc);
    }

    __declspec(dllexport) bool ScriptBreakpointExists(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::Exists(script, pc);
    }

    __declspec(dllexport) int ScriptBreakpointGetHitCount(std::uint32_t script, std::uint32_t pc)
    {
        return ScriptBreakpoint::GetHitCount(script, pc);
    }
}