#pragma once

namespace rage
{
	class scrThreadContext;
};

class ScriptBreakpoint
{
public:
	static void OnBreakpoint(std::uint32_t script, std::uint32_t pc, rage::scrThreadContext* context);

	static bool Add(std::uint32_t script, std::uint32_t pc);
	static bool Remove(std::uint32_t script, std::uint32_t pc);
	static bool RemoveAll();
	static bool RemoveAllScript(std::uint32_t script);
	static bool Resume(std::uint32_t script, std::uint32_t pc);
	static bool Exists(std::uint32_t script, std::uint32_t pc);
	static bool IsActive(std::uint32_t script, std::uint32_t pc);
	static int GetCount();
	static int GetCountScript(std::uint32_t script);
	static int GetHitCount(std::uint32_t script, std::uint32_t pc);

private:
	struct Breakpoint
	{
		std::uint32_t Script;
		std::uint32_t Pc;
		std::uint32_t HitCount;
		bool Active;
		bool Skip;
	};

	static inline std::vector<Breakpoint> m_Breakpoints;
};