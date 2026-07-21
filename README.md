# COALITION Lobby
An Arma Reforger addon providing the core mission-flow foundation used by [COALITION](https://coalitiongroup.net) for our events: the gamemode state machine, player slotting, gearscript roles/loadouts, player/character identity, spawn points, and the spectator system. It's designed to be depended on by a gameplay-focused addon like [COALITION Reforger Framework](https://github.com/CoalitionArma/Coalition-Reforger-Framework), which builds specific game modes, admin tooling, and mission content on top of it.

## Features
- **Gamemode state machine**: `CRF_Gamemode`, the mission entity driving the Briefing → Slotting → Game → AAR phases.
- **Slotting system**: squads, teams, crews and specialist roles are defined as data-driven `CRF_SlottingGroup` configs (see `Configs/SlottingGroups`), each with its own roster of `CRF_EGearRole` slots.
- **Per-slot / per-group respawn pools**: optional respawn limits, tracked either per role or shared across a whole group, for use with Slot-Based respawn missions.
- **Slot lottery**: players sign up with `/roll <faction> [squad]`, admins draw with `/runlottery` to randomly fill open roles across all factions.
- **Gearscript role catalog**: `CRF_GearscriptManager`/`CRF_RoleConfig` map each `CRF_EGearRole` to its slotting type, icon, and spawn prefab (`Configs/Gearscripts/CRF_Global_Roles_Config.conf`); `CRF_VehicleGearscriptManager` handles vehicle/supply-truck loadouts.
- **Player & character identity**: `CRF_PlayerController` and its manager suite (camera, chat commands, keybinds, settings, RPC authority/ownership, poly zones, scripted markers), plus the `CRF_PlayerCharacter`/`CRF_SpectatorCharacter` entity classes.
- **Spawn points**: group, rally, static, and vehicle spawn point entities used to place players into the world once slotted.
- **Spectator system**: spectator camera, freecam menu, damage reports, and entity/label overlays for observing matches before spawning or after death.
- **Lobby UI**: briefing and slotting phase layouts, player/squad list boxes, spectator HUD, and supporting icons under `UI/`.
- **Workbench tooling**: a `WorkbenchPlugin` (`Configure Slots`) for wiring up a mission's BLUFOR/OPFOR/INDFOR/CIV slot arrays from the World Editor, with a guided quick-setup dialog for first-time setup, plus gearscript config-list generation and validation plugins.

## Structure
| Folder | Contents |
| --- | --- |
| `Scripts/Game/Systems/CRF_Gamemode.c` | The core gamemode entity: mission-flow state machine, slotting/respawn/faction attributes |
| `Scripts/Game/Systems/Slotting` | Runtime slotting manager, slot data, lottery, and config classes |
| `Scripts/Game/Systems/Gearscript` | Role catalog and vehicle gearscript managers/configs |
| `Scripts/Game/Systems/PlayerController` | `CRF_PlayerController` and its camera/chat/keybind/settings/RPC/UI managers |
| `Scripts/Game/Systems/Characters` | `CRF_PlayerCharacter` and `CRF_SpectatorCharacter` entity classes |
| `Scripts/Game/Systems/SpawnPoints` | Group/rally/static/vehicle spawn point entities and spawn point data |
| `Scripts/Game/Systems/Spectator` | Spectator camera, menu, damage report, and label/icon UI |
| `Scripts/Game/Systems/UI/Menus` | Briefing and slotting menu scripts |
| `Scripts/WorkbenchGame/MissionPlugins` | Workbench editor plugin for configuring mission slots |
| `Scripts/WorkbenchGame/GearscriptConfigList`, `GearscriptValidator` | Workbench tools for generating/validating gearscript configs |
| `Configs/SlottingGroups` | Squad/team/crew/specialist role group definitions |
| `Configs/Gearscripts` | Global role catalog (`CRF_Global_Roles_Config.conf` + CVON variant) |
| `Configs/Map` | Lobby map configuration |
| `Prefabs` | The `CRF_Lobby` gamemode entity prefab and spectator prefabs |
| `UI` | Lobby, spectator, and listbox layouts and images |
| `Sounds/VON` | Lobby VON sound bank |

## Taking Part in Events
COALITION hosts events and weekly sessions open to the public on this framework. If you would like to take part in our Arma Reforger events, feel free to join our [Discord](https://discord.gg/the-coalition)!

## Contributing and Reporting Issues/Feature Requests
This addon is open to be used by and contributed to by other Reforger Groups, it is licensed under the Arma Public License.
If you would like to report Bugs or Feedback feel free to create an Issue on the Github page as well as letting us know in our [Discord](https://discord.gg/the-coalition) if that is easier for you.
If you would like to contribute to this repo, feel free to fork and PR as you see fit.

## Further Info
Most of the discussion relating to this addon takes place on our [Discord](https://discord.gg/the-coalition), feel free to take part or just read up.

## Support Us
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/thecoalitiongroup)

<img src="http://coalitiongroup.net/coalition.png">
