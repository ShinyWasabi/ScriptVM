#pragma once

namespace rage
{
	enum scrThreadState : std::uint32_t;
	union scrValue;
	class scrProgram;
	class scrThreadContext;
}

rage::scrThreadState RunScriptThread(rage::scrValue* stack, rage::scrValue** globals, rage::scrProgram* program, rage::scrThreadContext* context);