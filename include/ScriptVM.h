#pragma once

extern "C"
{
    // Adds a breakpoint at the specified PC in the specified script.
    // Returns true if the breakpoint was successfully added, false if it already exists.
    __declspec(dllimport) bool ScriptBreakpointAdd(unsigned int script, unsigned int pc);
    
    // Removes the breakpoint at the specified PC in the specified script.
    // Returns true if the breakpoint was removed, false if it didn't exist or was active.
    __declspec(dllimport) bool ScriptBreakpointRemove(unsigned int script, unsigned int pc);
    
    // Removes all breakpoints that are currently inactive.
    // Returns true if any breakpoints were removed.
    __declspec(dllimport) bool ScriptBreakpointRemoveAll();
    
    // Removes all inactive breakpoints for the specified script.
    // Returns true if any breakpoints were removed.
    __declspec(dllimport) bool ScriptBreakpointRemoveAllScript(unsigned int script);
    
    // Resumes execution of a previously hit breakpoint at the specified PC in the specified script.
    // Returns true if the breakpoint was successfully resumed, false otherwise.
    __declspec(dllimport) bool ScriptBreakpointResume(unsigned int script, unsigned int pc);
    
    // Checks if a breakpoint exists at the specified PC in the specified script.
    // Returns true if the breakpoint exists, false otherwise.
    __declspec(dllimport) bool ScriptBreakpointExists(unsigned int script, unsigned int pc);
    
    // Checks if a breakpoint at the specified PC in the specified script is currently active (hit/paused).
    // Returns true if active, false otherwise.
    __declspec(dllimport) bool ScriptBreakpointIsActive(unsigned int script, unsigned int pc);
    
    // Returns the total number of breakpoints currently registered (both active and inactive).
    __declspec(dllimport) int ScriptBreakpointGetCount();
    
    // Returns the number of breakpoints registered for the specified script (both active and inactive).
    __declspec(dllimport) int ScriptBreakpointGetCountScript(unsigned int script);
    
    // Returns the hit count of the breakpoint at the specified PC in the specified script.
    // Returns 0 if the breakpoint does not exist.
    __declspec(dllimport) int ScriptBreakpointGetHitCount(unsigned int script, unsigned int pc);
}