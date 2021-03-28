# d2dx
D2DX is a Glide-wrapper for Diablo II on modern PC:s

(The project is being renamed from "d2gx" due to a name clash with another Diablo 2 mod project.)

Version 0.91.327

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

To scale the window by 2x or 3x, add -gxscale2 or -gxscale3 to the command line. Note that if the Window doesn't fit on the desktop, the scale factor will be lowered.

Note that in-game resolution should be set to 800x600. Support for 640x480 has not been added yet.

## Release history

### 0.91.327
  - Fix two types of crashes in areas where there are many things on screen at once.
  - Fix window not being movable.

### 0.91.325b
  - Fix game being scaled wrong in LoD 1.13c/d.

### 0.91.325
  - Fix mode switching flicker on startup in fullscreen modes.
  - Fix mouse cursor being "teleported" when leaving the window in windowed mode.
  - Fix mouse speed not being consistent with the desktop.
  - Fix game looking fuzzy on high DPI displays.
  - Improve frame time consistency/latency.
  - Add experimental window scaling support.
  - More performance improvements.

### 0.91.324b

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
