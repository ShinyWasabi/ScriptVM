#pragma once

namespace rage
{
	class scrThreadContext;
};

class ScriptBreakpoint
{
public:
	static bool OnBreakpoint(std::uint32_t script, std::uint32_t pc, rage::scrThreadContext* context);

	static bool Add(std::uint32_t script, std::uint32_t pc, bool pause);
	static bool Remove(std::uint32_t script, std::uint32_t pc);
	static bool Resume(std::uint32_t script, std::uint32_t pc);
	static bool Exists(std::uint32_t script, std::uint32_t pc);
	static int GetHitCount(std::uint32_t script, std::uint32_t pc);

private:
	struct Breakpoint
	{
		std::uint32_t Script;
		std::uint32_t Pc;
		std::uint32_t HitCount;
		bool Active;
		bool Pause;
	};

	static void Activate(std::uint32_t script, std::uint32_t pc);
	static void Increment(std::uint32_t script, std::uint32_t pc);
	static bool IsActive(std::uint32_t script, std::uint32_t pc);
	static bool ShouldPause(std::uint32_t script, std::uint32_t pc);

	static inline std::vector<Breakpoint> m_Breakpoints;
};