# Hyrule Survivor's

[← Dawnlight overview](../../README.md)

Hyrule Survivor's is a separate game mode included in `dawnlight_mod.dusk`.
Select it in Dusklight's game-mode menu and create or load a Survivors save.
It uses `gczelda2-survivors`, separate from the story and Boss Rush save files.
The title screen, save selection, Link/Epona names and optional TV settings finish
before the arena loads. Selecting the mode alone cannot start a run. Returning
to the title screen disarms the run until another save is selected and started.

## First arena

The first version is a ten-minute run in Dawnlight's isolated Darknut arena.
Start with three hearts and Sword Wave. Move, dodge and collect blue experience
crystals while weapons attack automatically. Manual Jump remains available with
your configured binding; sword attacks and arena exits are disabled.

Leveling up pauses Link and the encounter and offers three upgrades. Controller,
keyboard and touch navigation use Dusklight's native mod window controls. Closing
the choice window does not resume combat or reroll its offers. Excess experience
is retained and multiple earned levels are offered consecutively.

| Upgrade | Effect |
| --- | --- |
| Sword Wave | Automatic aimed, piercing projectiles; additional waves at levels 3 and 5 |
| Whirling Blade | Unlock or improve a periodic attack around Link |
| Light Aura | Unlock or improve a smaller, more frequent area attack |
| Power | Increase automatic weapon damage |
| Haste | Reduce automatic weapon cooldowns |
| Reach | Increase weapon size |
| Magnet | Attract experience from farther away |
| Heart Container | Add one maximum heart and recover one heart |
| Recovery | Recover two hearts |

Equip all three available weapons and up to four different passive upgrades.
Weapons and passives have five levels. Recovery remains available after the
other upgrades reach their caps. Native enemy invulnerability, defense and death
reactions still apply; the mod replaces the native nonlinear damage table only
for its own weapons. Ordinary heart/rupee/item drops are suppressed in this arena.

Bokoblins and Keese appear first; Rats join after 30 seconds. Spawn frequency,
population and Bokoblin health increase during the run. At ten minutes, ordinary
spawning stops and an Elite Bokoblin appears. Defeat it to win. Death opens a
result screen with time, kills, level and weapon levels. **New run** reloads the
arena and resets all run upgrades.

## Records and saving

**Save records** on the result screen opens the game's native save prompt.
Complete that prompt before quitting to persist the records. The mode tracks
completed runs, wins, longest survival, most kills and highest level per save
slot. A new run keeps unsaved records in memory, but closing the application
without saving loses them. Failed runs still count as completed attempts.

Saving during combat does not suspend a run. Loading a Survivors save always
returns to a fresh start screen. Story and Boss Rush slots are not converted.

Dawnlight's combat presets, progression locks, HP scaling, Bullet Time, Flurry
Rush, Dark Link, Dual Wield, stamina, sprint, glide and Gale are overridden for
this mode without rewriting their saved configuration. Manual Jump uses normal
height. Other mods can still change gameplay; cross-mod balance is not enforced.

## Initial scope and device testing

This is the initial arena implementation. It retains the native camera and uses
code-drawn crystals, projectiles and rings. No additional textures, models or
music are required. Larger arenas, camera changes, weapon evolutions, persistent
unlock trees and mid-run suspend are not part of this version.

The budgets are 24 tracked enemies (including pending/deleting actors),
16 attack objects and 64 experience crystals. Excess crystals merge their value.
These limits are starting budgets, not a measured Android performance guarantee.
Spawns require a valid floor and unobstructed line from Link, and are staggered.
A failed elite creation/despawn can be retried without counting as a victory.
Only confirmed deaths of initialized, tracked actors award experience.

Before merging/releasing, test on Android and desktop:

1. Create and reload a separate Survivors slot; verify story and Boss Rush saves.
2. Complete a run using all three weapons; check native enemy death, elite victory
   and no HP scaling leakage. Verify jump, dodge, controller and touch controls.
3. Level up while surrounded, close/reopen the choice window, and earn several
   levels at once. Verify Link/enemies and the timer stay paused during selection.
4. Die, save the result, restart, reload the slot, and decline the save prompt.
5. Reset, change modes and disable/reload Dawnlight while enemies or projectiles
   are present. Confirm no stale actors, collisions or windows remain.
6. Stress the population and crystal caps and check frame time/memory on Android.

Automated tests cover rules, caps, upgrade choice integrity, record encoding,
true-death accounting, deferred spawn cleanup, startup/reset routing and shared
Boss Rush arena admission. They do not replace the in-game checks above.
