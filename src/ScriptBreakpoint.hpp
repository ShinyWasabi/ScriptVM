#pragma once

class ScriptBreakpoint
{
	struct Breakpoint
	{
		std::uint32_t Script;
		std::uint32_t Pc;
		std::uint32_t Id;
	};

	static inline bool m_SkipThisHit;
	static inline Breakpoint* m_ActiveBreakpoint;
	static inline std::vector<Breakpoint> m_Breakpoints;

public:
	static void OnHit(std::uint32_t script, std::uint32_t pc);

	static bool Add(std::uint32_t script, std::uint32_t pc);
	static bool Remove(std::uint32_t script, std::uint32_t pc);
	static bool Exists(std::uint32_t script, std::uint32_t pc);
	static bool Active();
	static bool Resume();
	static std::vector<Breakpoint> GetAll();
	static void RemoveAll();
};