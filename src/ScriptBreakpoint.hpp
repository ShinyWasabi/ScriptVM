#pragma once

class ScriptBreakpoint
{
public:
	static void Add(std::uint32_t script, std::uint32_t pc, bool pause)
	{
		GetInstance().AddImpl(script, pc, pause);
	}

	static void Remove(std::uint32_t script, std::uint32_t pc)
	{
		GetInstance().RemoveImpl(script, pc);
	}

	static void Resume(std::uint32_t script, std::uint32_t pc)
	{
		GetInstance().ResumeImpl(script, pc);
	}

	static void IncrementHitCount(std::uint32_t script, std::uint32_t pc)
	{
		GetInstance().IncrementHitCountImpl(script, pc);
	}

	static void Skip(std::uint32_t script, std::uint32_t pc)
	{
		GetInstance().SkipImpl(script, pc);
	}

	static std::uint32_t GetHitCount(std::uint32_t script, std::uint32_t pc)
	{
		return GetInstance().GetHitCountImpl(script, pc);
	}

	static bool Has(std::uint32_t script, std::uint32_t pc)
	{
		return GetInstance().HasImpl(script, pc);
	}

	static bool ShouldSkip(std::uint32_t script, std::uint32_t pc)
	{
		return GetInstance().ShouldSkipImpl(script, pc);
	}

	static bool ShouldPause(std::uint32_t script, std::uint32_t pc)
	{
		return GetInstance().ShouldPauseImpl(script, pc);
	}

private:
	struct BreakPoint
	{
		std::uint32_t Script;
		std::uint32_t Pc;
		std::uint32_t HitCount;
		bool Skip;
		bool Pause;
	};

	static ScriptBreakpoint& GetInstance()
	{
		static ScriptBreakpoint instance;
		return instance;
	}

	void AddImpl(std::uint32_t script, std::uint32_t pc, bool pause);
	void RemoveImpl(std::uint32_t script, std::uint32_t pc);
	void ResumeImpl(std::uint32_t script, std::uint32_t pc);
	void IncrementHitCountImpl(std::uint32_t script, std::uint32_t pc);
	void SkipImpl(std::uint32_t script, std::uint32_t pc);
	std::uint32_t GetHitCountImpl(std::uint32_t script, std::uint32_t pc);
	bool HasImpl(std::uint32_t script, std::uint32_t pc);
	bool ShouldSkipImpl(std::uint32_t script, std::uint32_t pc);
	bool ShouldPauseImpl(std::uint32_t script, std::uint32_t pc);

	static inline std::vector<BreakPoint> m_Breakpoints;
};