# d2gx
D2GX is a Glide-wrapper for Diablo II on modern PC:s


## Features
  - Game is rendered with DirectX 11.
  - Behaves nicely in windowed mode.
  - Enhanced fullscreen modes.
  - Fixes startup glitches in older versions of the game.
  - More graphical enhancements (tba).

## Requirements
  - Diablo 2: LoD 1.13c / 1.14d (other versions untested).
  - Windows 7 and above.
  - Integrated graphics or discrete GPU with DirectX 11 support (feature level 10.0 required).

## Installation
  Copy the included "glide3x.dll" into your Diablo II folder.
  
  Note that in some cases you may have to also download and install the Visual C++ runtime library from Microsoft: https://aka.ms/vs/16/release/vc_redist.x86.exe

## Usage
### To run the game in windowed mode
- Game.exe -3dfx -w

### To run the game in default fullscreen mode (800x600 scaled to desktop resolution):
- Game.exe -3dfx

### To run the game in experimental fullscreen mode (800x540 scaled to 1920x1080):
- Game.exe -3dfx -gx1080

To get rid of the "GX" logo on the title screen, add -gxskiplogo to the command line.
