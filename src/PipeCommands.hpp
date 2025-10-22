#pragma once

enum class ePipeCommands : std::uint8_t
{
    BREAKPOINT_SET,
    BREAKPOINT_EXISTS,
    BREAKPOINT_ACTIVE,
    BREAKPOINT_RESUME,
    BREAKPOINT_GET_ALL,
    BREAKPOINT_REMOVE_ALL
};

struct PipeCommandSetBreakpoint
{
    std::uint32_t Script;
    std::uint32_t Pc;
    bool Set;
};

struct PipeCommandBreakpointExists
{
    std::uint32_t Script;
    std::uint32_t Pc;
};

struct PipeCommandBreakpointGetAll
{
    std::uint32_t Script;
    std::uint32_t Pc;
};