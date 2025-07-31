#pragma once

namespace rage
{
	enum ThreadState : std::uint32_t;
	union scrValue;
	class scrProgram;
	class scrThreadContext;
}

rage::ThreadState RunScriptThread(rage::scrValue* stack, rage::scrValue** globals, rage::scrProgram* program, rage::scrThreadContext* context);