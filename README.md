# COALITION Lobby
An Arma Reforger addon that provides the mission lobby and slotting system used by [COALITION](https://coalitiongroup.net) for our events. It's designed to function off of a dependency like [COALITION Reforger Framework](https://github.com/CoalitionArma/Coalition-Reforger-Framework).

## Features
- **Slotting system**: squads, teams, crews and specialist roles are defined as data-driven `CRF_SlottingGroup` configs (see `Configs/SlottingGroups`), each with its own roster of `CRF_EGearRole` slots.
- **Per-slot / per-group respawn pools**: optional respawn limits, tracked either per role or shared across a whole group, for use with Slot-Based respawn missions.
- **Slot lottery**: players sign up with `/roll <faction> [squad]`, admins draw with `/runlottery` to randomly fill open roles across all factions.
- **Lobby UI**: briefing and slotting phase layouts, player/squad list boxes, and supporting icons under `UI/`.
- **Workbench mission slotting editor**: a `WorkbenchPlugin` (`Configure Slots`) for quickly wiring up a mission's BLUFOR/OPFOR/INDFOR/CIV slot arrays from the World Editor, with a guided quick-setup dialog for first-time setup.

## Structure
| Folder | Contents |
| --- | --- |
| `Scripts/Game/Systems/Slotting` | Runtime slotting manager, slot data, lottery, and config classes |
| `Scripts/WorkbenchGame/MissionPlugins` | Workbench editor plugin for configuring mission slots |
| `Configs/SlottingGroups` | Squad/team/crew/specialist role group definitions |
| `Configs/Map` | Lobby map configuration |
| `Prefabs` | The `CRF_Lobby` entity prefab |
| `UI` | Lobby layouts and images (briefing, slotting, listbox elements) |

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
