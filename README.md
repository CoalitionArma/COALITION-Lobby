# COALITION Lobby
An Arma Reforger addon providing the core mission-flow foundation used by [COALITION](https://coalitiongroup.net) for our events: the gamemode state machine, player slotting, gearscript roles/loadouts, player/character identity, spawn points, admin tooling, and the spectator system. It's designed to be depended on by a gameplay-focused addon like [COALITION Reforger Framework](https://github.com/CoalitionArma/Coalition-Reforger-Framework), which builds specific game modes and mission content on top of it.

## Features
- **Gamemode state machine**: `COA_Gamemode`, the mission entity driving the Briefing → Slotting → Game → AAR phases.
- **Slotting system**: squads, teams, crews and specialist roles are defined as data-driven `COA_SlottingGroup` configs (see `Configs/SlottingGroups`), each with its own roster of `COA_EGearRole` slots.
- **Per-slot / per-group respawn pools**: optional respawn limits, tracked either per role or shared across a whole group, for use with Slot-Based respawn missions, backed by `COA_RespawnManager` and a vanilla `SCR_SpawnRequestComponent` override.
- **Slot lottery**: players sign up with `/roll <faction> [squad]`, admins draw with `/runlottery` to randomly fill open roles across all factions.
- **Gearscript role catalog**: `COA_GearscriptManager`/`COA_RoleConfig` map each `COA_EGearRole` to its slotting type, icon, and spawn prefab (`Configs/Gearscripts/COA_Global_Roles_Config.conf`); `COA_VehicleGearscriptManager` handles vehicle/supply-truck loadouts.
- **Player & character identity**: `COA_PlayerController` and its manager suite (camera, chat commands, keybinds, settings, RPC authority/ownership, poly zones, scripted markers), the `COA_PlayerCharacter`/`COA_SpectatorCharacter`/`COA_GearscriptCharacter` entity classes, and region-specific identity pools under `Configs/Identities`.
- **Spawn points**: group, rally, static, and vehicle spawn point entities used to place players into the world once slotted.
- **Admin tooling**: an in-pause-menu `COA_AdminMenuManager`/Admin Menu UI covering game mode control, teleport, heal, gear, respawn control, and a player ticket/report queue.
- **Poly zone & manual marker system**: `COA_GameBorder` triggers with faction-restriction and screen-blur effects (used for safestart/forward-deploy boundaries), plus a manual map-marker placement system.
- **Spectator system**: spectator camera, freecam menu, damage reports, and entity/label overlays for observing matches before spawning or after death.
- **CVON integration**: overrides layered on top of the community CVON VON addon (`ModdedOverrides/CVON`) for group/radio/VON behavior.
- **Lobby UI**: briefing, slotting, respawn, and admin menu layouts, player/squad/ticket list boxes, spectator HUD, hint display, and supporting icons under `UI/`.
- **Prefab library**: pre-built per-faction character loadouts (`Prefabs/Characters`), squad/team compositions (`Prefabs/Groups`), and logistics vehicles (`Prefabs/Vehicles`), plus drag-and-drop mission-building helpers under `PrefabsMissionMaking/` (spawn points, markers, poly zones) for designers to wire up a mission without touching script.
- **Workbench tooling**: a `WorkbenchPlugin` (`Configure Slots`) for wiring up a mission's BLUFOR/OPFOR/INDFOR/CIV slot arrays from the World Editor, with a guided quick-setup dialog for first-time setup, plus gearscript config-list generation and validation plugins.

## Structure
| Folder | Contents |
| --- | --- |
| `Scripts/Game/!Define` | Load-marker `#define` so dependent addons can detect Lobby is loaded |
| `Scripts/Game/!Systems/Core/Entities` | `COA_Gamemode`, `COA_PlayerController`, plus `Characters` and `SpawnPoints` subfolders |
| `Scripts/Game/!Systems/Core/Managers/Gamemode` | `COA_GamemodeManager`, `COA_SlottingManager`, `COA_RespawnManager`, `COA_SafestartManager`, and `Gearscript`/`Misc`/`Replication`/`UI` manager subfolders (incl. `COA_AdminMenuManager`, `COA_GarbageManager`, `COA_PermissionManager`, `COA_RplBroadcastManager`, `COA_BandwidthTelemetryManager`) |
| `Scripts/Game/!Systems/Core/Managers/PlayerController` | `COA_PlayerControllerManager`, `COA_PlayerCameraManager`, and `Misc`/`Replication`/`UI` manager subfolders (chat commands, keybinds, settings, RPC authority/ownership, poly zones, scripted markers) |
| `Scripts/Game/!Systems/Core/Helpers` | Static helper classes (role, weapon, mission, player, replication, inventory, logging, entity, initialization, clothing, damage) |
| `Scripts/Game/!Systems/Core/Containers & Configs` | Config container classes, replicable containers (`COA_SlotData`, `COA_SpawnPointData`), resource cache, and JSON-backed moderator config |
| `Scripts/Game/!Systems/Components` | `COA_ObjectSpawner` and `COA_VehicleSpawner` trigger-entity components |
| `Scripts/Game/!Systems/ModdedOverrides/CVON` | Overrides layered on the CVON VON addon |
| `Scripts/Game/!Systems/Spectator` | Spectator camera, menu, damage report, and label/icon UI |
| `Scripts/Game/!Systems/UI/CustomListbox` | Shared listbox widget framework used by menus |
| `Scripts/Game/!Systems/UI/HUDs` | Outro, safestart, game timer, and hint HUD elements |
| `Scripts/Game/!Systems/UI/MapMarkers` | Manual marker placement and poly zone (safestart/forward-deploy boundary) system |
| `Scripts/Game/!Systems/UI/Menus` | Slotting, briefing, admin, spectator, pause, and respawn menu scripts |
| `Scripts/Game/!Systems/VanillaOverrides` | `COA_SCR_*` overrides of stock Chimera/Arma classes (inventory, groups manager, damage manager, VON, spawn request) |
| `Scripts/WorkbenchGame/MissionPlugins` | Workbench editor plugin for configuring mission slots |
| `Scripts/WorkbenchGame/GearscriptConfigList`, `GearscriptValidator` | Workbench tools for generating/validating gearscript configs |
| `Configs/SlottingGroups` | Squad/team/crew/specialist role group definitions |
| `Configs/Gearscripts` | Global role catalog and per-faction/era loadout configs |
| `Configs/Identities` | Regional character identity/appearance pools |
| `Configs/Systems` | Engine/menu/input overrides: chat channels, control hints, placeable entity registries, radio frequencies, map config, menu/input configs |
| `Missions` | `COA_BaseMissionConfig.conf`, a base mission header template for new mission files |
| `Prefabs/Systems/!Lobby/COA_Lobby.et` | The main gamemode entity prefab |
| `Prefabs/Characters`, `Prefabs/Groups`, `Prefabs/Vehicles` | Pre-built per-faction character loadouts, squad/team compositions, and logistics vehicles |
| `PrefabsMissionMaking` | Designer-facing spawn point, marker, and poly zone prefabs for building missions in the World Editor |
| `UI` | Lobby, admin, respawn, spectator, and listbox layouts and images |
| `Sounds/VON` | Lobby VON sound bank |
| `Worlds/Arland` | `COA_LobbyTestWorld`, a test world for developing the lobby gamemode |

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
