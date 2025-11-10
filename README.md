# ScriptVM
Reimplementation of the script virtual machine for Grand Theft Auto V (Legacy & Enhanced).

It serves as a pipe server to handle script breakpoints for [scrDbg](https://github.com/ShinyWasabi/scrDbg).

## Usage

You can use Ultimate ASI Loader to load the module into the game:
- [dinput8.dll](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/x64-latest/dinput8-x64.zip) for Legacy
- [xinput1_4.dll](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/x64-latest/xinput1_4-x64.zip) for Enhanced

1. Place the appropriate DLL in the game directory along with `ScriptVM.asi`.
2. Launch the game (make sure BattlEye is disabled).
3. Run `scrDbg.exe` to connect to the pipe server.