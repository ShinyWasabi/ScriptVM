#pragma once
#include "scrThreadContext.hpp"
#include "atArray.hpp"

namespace rage
{
	union scrValue;

	class scrThread
	{
	public:
		virtual ~scrThread() = default;
		virtual void Reset(std::uint64_t scriptHash, void* args, std::uint32_t argCount) = 0;
		virtual scrThreadState RunImpl() = 0;
		virtual scrThreadState Run() = 0;
		virtual void Kill() = 0;
		virtual void GetInfo(void* info) = 0;

		scrThreadContext m_Context;
		scrValue* m_Stack;
		char m_Pad1[0x04];
		std::uint32_t m_ArgSize;
		std::uint32_t m_ArgLoc;
		char m_Pad2[0x04];
		char m_ErrorMessage[128];
		std::uint32_t m_ScriptHash;
		char m_ScriptName[64];

		static scrThread* GetCurrentThread();
		static scrThread* GetThread(std::uint32_t hash);

		static inline atArray<scrThread*>* m_Threads;
	};
	static_assert(sizeof(scrThread) == 0x198);
}