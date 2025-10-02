#include "ScriptBreakpoint.hpp"
#include "rage/scrThread.hpp"

void ScriptBreakpoint::OnBreakpoint(std::uint32_t script, std::uint32_t pc, rage::scrThreadContext* context)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
        {
            if (bp.Skip)
            {
                bp.Skip = false; // skip this hit once so we don't end up bouncing
                return;
            }

            if (!bp.Active)
            {
                context->m_ProgramCounter = pc;
                context->m_State = rage::scrThreadState::PAUSED;

                bp.HitCount++;
                bp.Active = true;

                if (auto thread = rage::scrThread::GetCurrentThread())
                {
                    char message[128];
                    std::snprintf(message, sizeof(message), "Breakpoint hit, paused %s at 0x%X!", thread->m_ScriptName, thread->m_Context.m_ProgramCounter);
                    MessageBoxA(0, message, "ScriptVM", MB_OK | MB_TOPMOST);
                }
            }
        }
    }
}

bool ScriptBreakpoint::Add(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return false;
    }

    m_Breakpoints.push_back({ script, pc, 0, false, false });
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

bool ScriptBreakpoint::RemoveAll()
{
    auto oldSize = m_Breakpoints.size();

    auto it = std::remove_if(m_Breakpoints.begin(), m_Breakpoints.end(), [](const Breakpoint& bp) {
        return !bp.Active;
    });

    m_Breakpoints.erase(it, m_Breakpoints.end());
    return m_Breakpoints.size() < oldSize;
}

bool ScriptBreakpoint::RemoveAllScript(std::uint32_t script)
{
    auto oldSize = m_Breakpoints.size();

    auto it = std::remove_if(m_Breakpoints.begin(), m_Breakpoints.end(), [script](const Breakpoint& bp) {
        return bp.Script == script && !bp.Active;
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

                bp.Active = false;
                bp.Skip = true;
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

bool ScriptBreakpoint::IsActive(std::uint32_t script, std::uint32_t pc)
{
    for (auto& bp : m_Breakpoints)
    {
        if (bp.Script == script && bp.Pc == pc)
            return bp.Active;
    }

    return false;
}

int ScriptBreakpoint::GetCount()
{
    return static_cast<int>(m_Breakpoints.size());
}

int ScriptBreakpoint::GetCountScript(std::uint32_t script)
{
    return static_cast<int>(std::count_if(m_Breakpoints.begin(), m_Breakpoints.end(), [script](const Breakpoint& bp) {
        return bp.Script == script;
    }));
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