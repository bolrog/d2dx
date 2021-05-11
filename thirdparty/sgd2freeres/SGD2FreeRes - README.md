# SlashGaming Diablo II Free Resolution (SGD2FreeRes)
This is a project aimed at modding Diablo II to support any resolution.

## Features
- Enables selecting user-specified resolutions from the Video Options menu.
- Automatically adjusts the positions of inventory and UI elements, using 800x600 as a base for positions.
- Allows limited customization of the UI.

## Compatibility
- Supports 1.09D, 1.13C, 1.13D, 1.14C, and 1.14D.
- Supports multiple video modes.
  - Supports standard GDI mode. Resolutions are unrestricted.
  - Supports standard DirectDraw and Direct3D modes, restricted to "standard" resolutions.
  - Supports CnC-DDraw wrapper. Resolutions are unrestricted.
  - Supports [Sven's Glide wrapper](http://www.svenswrapper.de/english/), [nGlide](https://www.zeus-software.com/), and [D2DX](https://github.com/bolrog/d2dx) Glide wrapper. Resolutions are unrestricted.
- Compatibility with select modsystems.
  - [PlugY](http://plugy.free.fr/en/index.html)
  - [D2SE](https://snej.org/forum/index2.php?topic=18954.msg459574#msg459574)

## Usage
The DLL does nothing on its own and must be loaded into the game via external tools. Once loaded, the game will need to be run in order to generate the config file for the first time. Singleplayer resolutions, along with other configuration options can be configured to the preferences of the user.

## Multiplayer Use
Multiplayer use is restricted to certain gateways. This cannot be changed easily and prevents unintentional abuse where it is not allowed (i.e. Battle.net). If used in unpermitted environments, the resolution is hardcoded to run with only 640x480 or 800x600. It is also still possible to be detected by anti-cheat systems and be banned for unauthorized modification of the game client, even if the functionality is ineffective. Where it is permitted, the resolutions are preset for the server.

Any server owner that wishes to authorize the usage of SGD2FreeRes on their server should contact Mir Drualga on the SlashDiablo Discord.

Servers that permit SGD2FreeRes:
- [Diablo 09](https://www.diablo09.com/): 640x480, 800x600, 856x480, 1068x600
- [SlashDiablo](https://slashdiablo.net/): 640x480, 800x600, 856x480, 1068x600
- [Resurgence](https://resurgence.slashgaming.net/): 640x480, 800x600, 856x480, 1068x600
- [Project Diablo 2](https://www.projectdiablo2.com/): 640x480, 800x600, 856x480, 1068x600
- [D2LOD.NET](http://d2lod.net/): 640x480, 800x600, 856x480, 1024x768, 1068x600

## Contribution
SGD2FreeRes is coded in C++20. It currently is intended to compile with Visual Studio 2019.

The linker needs to be configured to link the DLL to version.lib, MDCc.lib, MDCcpp98.lib and SGD2MAPI.lib.

When you submit a pull request, you certify that the code in the pull request is AGPLv3+ compatible. You also certify that you have authorization to submit the pull request with the code changes. You certify that the merging of the pull request with those changes is authorized under the license terms of the AGPLv3+. Finally, you certify that the contribution is licensed under the AGPLv3+.

## Thanks
- [/r/SlashDiablo](https://www.reddit.com/r/slashdiablo/): The community that got me started. The whole reason the project took off.
- [PhrozenKeep](https://d2mods.info/): Community with plenty of information on Diablo II, modding, and resources.
- [balrog](https://github.com/bolrog): For providing an extended API to enable interfacing with the D2DX Glide wrapper.

## Legal
The project is licensed under the terms of the Affero General Public License, version 3 or higher, with exceptions. Components may be licensed under other terms, so check LICENSE for more details. If you wish to apply for a proprietary license exception, please contact Mir Drualga on the SlashDiablo Discord channel.

Diablo II and Diablo II: Lord of Destruction are registered trademarks of Blizzard Entertainment. This project is not affiliated with Blizzard Entertainment in any way.
