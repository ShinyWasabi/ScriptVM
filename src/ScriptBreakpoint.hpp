#pragma once

class ScriptBreakpoint
{
	struct Breakpoint
	{
		std::uint32_t Script;
		std::uint32_t Pc;
	};

	static inline bool m_SkipThisHit;
	static inline bool m_WasGamePaused;

	static inline bool m_PauseGame;
	static inline std::optional<Breakpoint> m_ActiveBreakpoint;
	static inline std::vector<Breakpoint> m_Breakpoints;

	static void ClearActive();
	static void PauseGame(bool pause);

public:
	static bool Process();

	static bool Add(std::uint32_t script, std::uint32_t pc);
	static bool Remove(std::uint32_t script, std::uint32_t pc);
	static bool Exists(std::uint32_t script, std::uint32_t pc);
	static bool Resume();
	static void SetPauseGame(bool pause);
	static std::optional<Breakpoint> GetActive();
	static std::vector<Breakpoint> GetAll();
	static void RemoveAll();
};