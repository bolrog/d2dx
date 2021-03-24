# d2gx
D2GX is a Glide-wrapper for Diablo II on modern PC:s

Version 0.9.324

## Features
  - High performance DirectX 11 renderer.
  - Behaves nicely in windowed mode.
  - Modernized fullscreen mode with high quality scaling.
  - Fixes startup glitches in older versions of the game.
  - More graphical enhancements (tba).

## Requirements
  - Diablo 2: LoD 1.13c, 1.13d, or 1.14d (other versions untested).
  - Windows 7 and above.
  - A CPU with SSE2 support.
  - Integrated graphics or discrete GPU with DirectX 11 support (feature level 10.0 required).

## Compatibility
So far it has been tested as working with:
  - D2 LoD 1.13c, 1.13d, 1.14d
  - PlugY
  - D2ModMaker

## Installation
  Copy the included "glide3x.dll" into your Diablo II folder.
  
  Note that in some cases you may have to also download and install the Visual C++ runtime library from Microsoft: https://aka.ms/vs/16/release/vc_redist.x86.exe

  The wrapper should work with PlugY, just make sure you have (at a minimum) -3dfx in the ini file:
  ```
  [LAUNCHING]
  Param=-3dfx
  ```
## Usage
### To run the game in windowed mode
- Game.exe -3dfx -w

### To run the game in default fullscreen mode (800x600 scaled to desktop resolution):
- Game.exe -3dfx

### To run the game in experimental fullscreen mode (800x540 "integer scaled" to 1920x1080):
- Game.exe -3dfx -gx1080

To get rid of the "GX" logo on the title screen, add -gxskiplogo to the command line.

## Release history

### 0.91.324
  - Fix crash when hosting TCP/IP game.
  - Speed up texture cache lookup.
  - Changed to a slightly less insane version numbering scheme.

### 0.9.210323c
  - Fix crash occurring after playing a while.

### 0.9.210323b:
  - Add support for LoD 1.13d.
  - Fix accidental performance degradation in last build.

### 0.9.210323:
  - Add support for LoD 1.13c.
  - Fix the delay/weird characters in the corner on startup in LoD 1.13c.
  - Fix glitchy window movement/sizing on startup in LoD 1.13c.
  - Performance improvements.

### 0.9.210322:
  - Fix line rendering (missing exp. bar, rain, npc crosses on mini map).
  - Fix smudged fonts.
  - Default fullscreen mode now uses the desktop resolution, and uses improved scaling (less fuzzy).

### 0.9.210321b:
  - Fix default fullscreen mode.

### 0.9.210321:
  - Initial release.
