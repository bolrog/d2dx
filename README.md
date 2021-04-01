# D2DX

D2DX is a preservation project for running classic Diablo II/LoD on modern PCs. 

Version 0.99.401c

## Mission statement
  - Preserve the classic Diablo 2/LoD experience as much as possible.
  - Turn the game into a well behaved DirectX 11 title on Windows 8/10.
  - Allow integer scaling to modern resolutions, including widescreen.

## Implemented
  - High performance DirectX 11 renderer (Glide wrapper).
  - Proper gamma/contrast support.
  - Improved fullscreen mode: instant ALT-TAB and low latency.
  - Improved windowed mode.
  - Widescreen support (in vanilla D2/LoD, via bundled version of SlashDiablo-HD).
  - Fixed various glitches in the supported game versions.

## Upcoming
  - Seamless windowed/fullscreen switching. 
  - Better scaling.

## Requirements
  - Diablo 2: LoD (see Compatibility section below).
  - Windows 8 and above (10 recommended).
  - A CPU with SSE2 support.
  - Integrated graphics or discrete GPU with DirectX 11 support (feature level 10.0 required).

## Compatibility
Game versions supported:
  - 1.09d, 1.10, 1.12, 1.13c, 1.13d and 1.14d.
  - Other versions will display a warning at startup and exhibit glitches.

D2DX has been tested working with the following mods:
  - MedianXL (1024x768)
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
  If you wish to use the widescreen modes, also copy the included "D2DX_SlashDiabloHD.dll" and "SlashDiabloHD.mpq" into your Diablo II folder.

## Usage
### Windowed mode
  ```
  Game.exe -3dfx -w
  ```

### Default fullscreen mode (note that this may cause aliasing artifacts due to non-integer scaling):
  ```
  Game.exe -3dfx
  ```

### Experimental widescreen (windowed and fullscreen) modes 
  PLEASE NOTE: This only works with 1.12, 1.13c and 1.13d at this time.

  Ensure the "D2DX_SlashDiabloHD.dll" and "SlashDiabloHD.mpq" files are in your Diablo II folder, and run the game as above.
  When it is present, D2DX will enable a new in-game resolution close to the normal ones - but with the aspect ratio of your monitor.
  The goal of this is to achieve integer scaling which eliminates scaling artifacts when displaying the game on modern PCs.

  - For a 1920x1080 monitor, this is 960x540 (in fullscreen: 2x integer scaling).
  - For a 2560×1440 monitor, this is 853x480 (in fullscreen: 3x integer scaling).
  - For a 3840x2160 monitor, this is 960x540 (in fullscreen: 2x integer scaling).

### Miscellaneous
- To get rid of the "DX" logo on the title screen, add -gxskiplogo to the command line.
- To scale the window by 2x or 3x, add -gxscale2 or -gxscale3 to the command line. Note that if the Window doesn't fit on the desktop, the scale factor will be lowered.

## Troubleshooting

### I get a message box saying "Diablo II is unable to proceed. Unsupported graphics mode."
  You are running the download version of Diablo II from blizzard.com. Upgrade to 1.14d.

### It's ugly/slow/buggy.
  Let me know! I'd like to keep improving D2DX (within the scope of the project).

## Acknowledgements
This project uses the following third party libraries:
- FNV1a hash reference implementation, which is in the public domain
- Detours by Microsoft
- SlashDiablo-HD by Mir Drualga and Bartosz Jankowski

## Release history

### 0.99.401c
  - Add experimental support for widescreen modes using a fork of SlashDiablo-HD by Mir Drualga and Bartosz Jankowski.
  - Remove the use of "AA bilinear" filtering, in favor of point filtering. This is part of a work in progress and will be tweaked further.
  - Cut VRAM footprint by 75% and reduce performance overhead.
  - Source code is now in the git.
  - Updated: fix occasional glitches due to the wrong texture cache policy being used.
  - Updated again: forgot to include SlashDiabloHD.mpq, which is required. 

### 0.99.329b
  - Add support for 1024x768, tested with MedianXL which now seems to work.
  - Fix window being re-centered by the game even if user moved it.
  - Fix occasional crash introduced in 0.99.329.

### 0.99.329
  - Add support for 640x480 mode, and polish behavior around in-game resolution switching.

### 0.98.329
  - Add support for LoD 1.09d, 1.10 and 1.12.
  - Add warning dialog on startup when using an unsupported game version.

### 0.91.328
  - Fix mouse pointer "jumping" when opening inventory and clicking items in fullscreen or scaled-window modes.

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
