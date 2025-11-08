#include "ScriptBreakpoint.hpp"
#include "Memory.hpp"
#include "rage/scrThread.hpp"

// TO-DO: Consider making this optional.
void PauseGame(bool pause)
{
    static void(*PauseGameNow)() = nullptr;
    static void(*UnpauseGameNow)() = nullptr;
    static void(*TogglePausedRenderPhases)(bool, int) = nullptr;
    static std::uint8_t* PauseGameNowPatch = nullptr;

    static bool Init = [] {
        if (auto addr = Memory::ScanPattern("56 48 83 EC 20 80 3D ? ? ? ? ? 75 ? 48 8D 0D"))
            PauseGameNow = addr->As<decltype(PauseGameNow)>();

        if (auto addr = Memory::ScanPattern("56 57 53 48 83 EC 20 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D"))
            UnpauseGameNow = addr->As<decltype(UnpauseGameNow)>();

        if (auto addr = Memory::ScanPattern("80 3D ? ? ? ? ? 74 ? 48 83 3D ? ? ? ? ? 74 ? 89 D0"))
            TogglePausedRenderPhases = addr->As<decltype(TogglePausedRenderPhases)>();

        if (auto addr = Memory::ScanPattern("80 88 ? ? ? ? ? EB ? E8"))
            PauseGameNowPatch = addr->Sub(0x1E).As<std::uint8_t*>();

        return true;
    }();

    if (!PauseGameNow || !UnpauseGameNow || !TogglePausedRenderPhases || !PauseGameNowPatch)
        return;

    if (pause)
    {
        *PauseGameNowPatch = 0xEB;
        PauseGameNow();
    }
    else
    {
        UnpauseGameNow();
        TogglePausedRenderPhases(true, 0);
        *PauseGameNowPatch = 0x74;
    }
}

bool ScriptBreakpoint::Process()
{
    auto thread = rage::scrThread::GetCurrentThread();
    if (!thread)
        return false;

    for (auto& bp : m_Breakpoints)
    {
        if (thread->m_ScriptHash != bp.Script || thread->m_Context.m_ProgramCounter != bp.Pc)
            continue;

        if (m_SkipThisHit)
        {
            m_SkipThisHit = false;
            return false;
        }

        // Show the message first so scrDbg doesn't attempt to resume when MessageBoxA is active
        char message[128];
        std::snprintf(message, sizeof(message), "Breakpoint hit, paused %s at 0x%X!", thread->m_ScriptName, thread->m_Context.m_ProgramCounter);
        MessageBoxA(0, message, "ScriptVM", MB_OK);

        m_ActiveBreakpoint = &bp;
        thread->m_Context.m_State = rage::scrThreadState::PAUSED;

        PauseGame(true);
        return true; // Return here to signal the VM that a BP is active, no need to check for others as there can only be one active at a time.
    }

    return false;
}

bool ScriptBreakpoint::Add(std::uint32_t script, std::uint32_t pc)
{
    if (Exists(script, pc))
        return false;

    m_Breakpoints.push_back({ script, pc });
    return true;
}

bool ScriptBreakpoint::Remove(std::uint32_t script, std::uint32_t pc)
{
    for (auto it = m_Breakpoints.begin(); it != m_Breakpoints.end(); it++)
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
                PauseGame(false);
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
    return m_ActiveBreakpoint != nullptr;
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

        PauseGame(false);
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