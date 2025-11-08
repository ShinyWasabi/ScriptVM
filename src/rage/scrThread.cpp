#include "scrThread.hpp"
#include "tlsContext.hpp"
#include "Memory.hpp"

namespace rage
{
	scrThread* scrThread::GetCurrentThread()
	{
		return tlsContext::Get()->m_CurrentScriptThread;
	}

	scrThread* scrThread::GetThread(std::uint32_t hash)
	{
		static bool init = [] {
			if (auto addr = Memory::ScanPattern("48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97"))
				m_Threads = addr->Add(3).Rip().As<atArray<scrThread*>*>();

			return true;
		}();

		if (!m_Threads)
			return nullptr;

		for (auto& thread : *m_Threads)
		{
			if (thread && thread->m_Context.m_ThreadId && thread->m_ScriptHash == hash)
				return thread;
		}

		return nullptr;
	}
}