# D2DX

D2DX is a preservation project for running classic Diablo II/LoD on modern PCs. 

Version 0.99.411

## Mission statement
  - Turn the game into a well behaved DirectX 11 title on Windows 10 (7, 8 and 8.1 are also supported).
  - High quality scaling to modern resolutions, including widescreen.
  - Aim to preserve the classic Diablo 2/LoD experience as much as possible.
  
## Implemented
  - High performance DirectX 11 renderer (Glide wrapper).
  - Proper gamma/contrast.
  - Improved fullscreen mode: instant ALT-TAB and low latency.
  - Improved windowed mode.
  - Widescreen support (in vanilla D2/LoD).
  - Seamless windowed/fullscreen switching with (ALT-Enter).
  - Fixes various glitches in the supported game versions.

## Upcoming
  - FXAA or SMAA (if possible without excessive smoothing).
  - Suggestions welcome!

## Requirements
  - Diablo 2: LoD (see Compatibility section below).
  - Windows 7 SP1 and above (10 recommended for latency improvements).
  - A CPU with SSE2 support.
  - Integrated graphics or discrete GPU with DirectX 10 support.

## Compatibility
Game versions supported:
  - All features: 1.12, 1.13c and 1.13d.
  - All features except widescreen: 1.09d, 1.10 and 1.14d.
  - Other versions are unsupported, will display a warning at startup and exhibit glitches.

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

## Usage
### To start the game with D2DX enabled
  ```
  Game.exe -3dfx
  ```
Windowed/fullscreen mode can be switched at any time by pressing ALT-Enter.

### Command-line options

  Hide the "D2DX" logo on the title screen
  ```
  -dxnologo
  ``` 
  
  Disable widescreen mode
  ```
  -dxnowide 
  ```

  Disable built-in resolution mod entirely
  ```
  -dxnoresmod
  ```
  
  Disable v-sync (may be useful on G-Sync/FreeSync displays)
  ```
  -dxnovsync
  ```
  
  In windowed mode, scale the size by 2 or 3:
  ```
  -dxscale2
  -dxscale3
  ```

### Experimental widescreen (windowed and fullscreen) modes 
  PLEASE NOTE: This only works with 1.12, 1.13c and 1.13d at this time.

  D2DX contains a modified version of D2HD to provide widescreen in-game modes.

  Widescreen mode is enabled by default, except if:
  - The option -dxnowide is passed.
  - Running a game version that isn't supported.
  - Running the "Median XL" mod (D2DX will allow it to use 1024x768 instead).

  D2DX will create a file named "d2dx_d2hd.mpq" in the Diablo II folder, and add a new in-game resolution, close to the normal ones but having the aspect ratio of your monitor.
  The goal of this is to achieve proper fullscreen scaling without artifacts when displaying the game on modern PCs.

  - For a 1920x1080 monitor, this is 960x540 (in fullscreen: 2x integer scaling).
  - For a 2560×1440 monitor, this is 1280x720 (in fullscreen: 2x integer scaling).
  - For a 3840x2160 monitor, this is 1280x720 (in fullscreen: 3x integer scaling).

## Troubleshooting

### I get a message box saying "Diablo II is unable to proceed. Unsupported graphics mode."
  You are running the download version of Diablo II from blizzard.com. This can be modified to work with D2DX (Wiki page about this to come).

### It's ugly/slow/buggy.
  Let me know by filing an issue! I'd like to keep improving D2DX (within the scope of the project).

## Third-party libraries
This project uses the following third party libraries:
- FNV1a hash reference implementation, which is in the public domain.
- Detours by Microsoft.
- SlashDiablo-HD/D2HD by Mir Drualga and Bartosz Jankowski, licensed under Affero GPL v3.

## Release history

### 0.99.411
  - D2DX should now work on DirectX 10/10.1 graphics cards.

### 0.99.410
  - Improved non-integer scaling quality, using "anti-aliased nearest" sampling (similar to sharp-bilinear).
  - Specifying -dxnowide will now select a custom screen mode that gives integer scaling, but with ~4:3 aspect ratio.
  - Added -dxnoresmod option, which turns off the built-in SlashDiablo-HD (no custom resolutions).
  - (For res mod authors) Tuned configurator API to disable the built-in SlashDiablo-HD automatically when used.
  - Other internal improvements.

### 0.99.408
  - Fix window size being squashed vertically after alt-entering from fullscreen mode.
  - Fix crash when running on Windows 7.

### 0.99.407
  - For widescreen mode, select 720p rather than 480p in-game resolution on 1440p monitors. 

### 0.99.406
  - Fix bug that could crash the game in the Video Options menu.

### 0.99.405
  - Simplify installation by removing the need to copy SlashDiablo-HD/D2HD DLL and MPQ files.
    Only glide3x.dll needs to be copied to the Diablo II folder.

### 0.99.403b
  - Fix mouse sensitivy being wrong in the horizontal direction in widescreen mode.
  - Fix bug occasionally causing visual corruption when many things were on screen.
  - Fix the well-known issue of the '5' character looking like a '6'. (Shouldn't interfere with other mods.)
  - Fix "tearing" seen due to vsync being disabled. Re-enable vsync by default (use -dxnovsync to disable it).
  - Some small performance improvements.
  - Updated: include the relevant license texts.

### 0.99.402
  - Add seamless alt-enter switching between windowed and fullscreen modes.

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
