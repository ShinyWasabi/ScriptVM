#pragma once

class ScriptBreakpoint
{
	struct Breakpoint
	{
		std::uint32_t Script;
		std::uint32_t Pc;
	};

	static inline bool m_SkipThisHit;
	static inline Breakpoint* m_ActiveBreakpoint;
	static inline std::vector<Breakpoint> m_Breakpoints;

public:
	static bool Process();

	static bool Add(std::uint32_t script, std::uint32_t pc);
	static bool Remove(std::uint32_t script, std::uint32_t pc);
	static bool Exists(std::uint32_t script, std::uint32_t pc);
	static bool Active();
	static bool Resume();
	static std::vector<Breakpoint> GetAll();
	static void RemoveAll();
};