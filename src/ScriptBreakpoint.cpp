#include "ScriptBreakpoint.hpp"
#include "rage/scrThread.hpp"

void ScriptBreakpoint::OnHit(std::uint32_t script, std::uint32_t pc)
{
    // Remove any breakpoints whose scripts no longer exist
    m_Breakpoints.erase(std::remove_if(m_Breakpoints.begin(), m_Breakpoints.end(), [](const Breakpoint& bp) {
        return !rage::scrThread::GetThread(bp.Script);
    }), m_Breakpoints.end());

    if (auto thread = rage::scrThread::GetThread(script))
    {
        // Remove any breakpoints whose thread ID doesn't match this instance of its script
        m_Breakpoints.erase(std::remove_if(m_Breakpoints.begin(), m_Breakpoints.end(), [thread, script](const Breakpoint& bp) {
            return bp.Script == script && bp.Id != thread->m_Context.m_ThreadId;
        }), m_Breakpoints.end());

        for (auto& bp : m_Breakpoints)
        {
            if (bp.Script != script || bp.Pc != pc)
                continue;

            if (m_SkipThisHit)
            {
                m_SkipThisHit = false;
                return;
            }

            m_ActiveBreakpoint = &bp;

            thread->m_Context.m_ProgramCounter = pc;
            thread->m_Context.m_State = rage::scrThreadState::PAUSED;

            std::string callstack;
            callstack += "Call stack:\n";
            for (uint8_t i = 0; i < thread->m_Context.m_CallDepth && i < 16; i++)
            {
                char entry[32];
                std::snprintf(entry, sizeof(entry), "  [%u] 0x%X\n", i, thread->m_Context.m_CallStack[i]);
                callstack += entry;
            }

            if (thread->m_Context.m_CallDepth == 0)
                callstack += "  (empty)\n";

            char message[512];
            std::snprintf(message, sizeof(message), "Breakpoint hit, paused %s at 0x%X!\n\n%s", thread->m_ScriptName, thread->m_Context.m_ProgramCounter, callstack.c_str());
            MessageBoxA(0, message, "ScriptVM", MB_OK);
            return;
        }
    }
}

bool ScriptBreakpoint::Add(std::uint32_t script, std::uint32_t pc)
{
    if (Exists(script, pc))
        return false;

    if (auto thread = rage::scrThread::GetThread(script))
    {
        m_Breakpoints.push_back({ script, pc, thread->m_Context.m_ThreadId });
        return true;
    }

    return false;
}

bool ScriptBreakpoint::Remove(std::uint32_t script, std::uint32_t pc)
{
    for (auto it = m_Breakpoints.begin(); it != m_Breakpoints.end(); ++it)
    {
        if (it->Script == script && it->Pc == pc)
        {
            if (m_ActiveBreakpoint == &(*it))
            {
                if (auto thread = rage::scrThread::GetThread(m_ActiveBreakpoint->Script))
                {
                    m_ActiveBreakpoint = nullptr;
                    thread->m_Context.m_State = rage::scrThreadState::RUNNING;
                }
            }

            m_Breakpoints.erase(it);
            return true;
        }
    }

    return false;
}

bool ScriptBreakpoint::Exists(std::uint32_t script, std::uint32_t pc)
{
    return std::any_of(m_Breakpoints.begin(), m_Breakpoints.end(), [script, pc](const Breakpoint& bp) {
        return bp.Script == script && bp.Pc == pc;
    });
}

bool ScriptBreakpoint::Active()
{
    return m_ActiveBreakpoint;
}

bool ScriptBreakpoint::Resume()
{
    if (!m_ActiveBreakpoint)
        return false;

    if (auto thread = rage::scrThread::GetThread(m_ActiveBreakpoint->Script))
    {
        m_ActiveBreakpoint = nullptr;
        m_SkipThisHit = true;

        thread->m_Context.m_State = rage::scrThreadState::RUNNING;
        return true;
    }

    return false;
}

std::vector<ScriptBreakpoint::Breakpoint> ScriptBreakpoint::GetAll()
{
    return m_Breakpoints;
}

void ScriptBreakpoint::RemoveAll()
{
    Resume();
    m_Breakpoints.clear();
}