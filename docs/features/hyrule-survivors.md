# Hyrule Survivors

[← Dawnlight overview](../../README.md) · [Player guide and Luck chart](../../SURVIVORS.md)

Hyrule Survivors is a separate game mode included in `dawnlight_mod.dusk`.
Select it in Dusklight's game-mode menu and create or load a Survivors save.
It uses `gczelda2-survivors`, separate from the story and Boss Rush save files.
The title screen, save selection, Link/Epona names and optional TV settings finish
before the arena loads. Selecting the mode alone cannot start a run. Returning
to the title screen disarms the run until another save is selected and started.

## Arena selection

After file selection, choose an arena with controller, keyboard or touch. The
right-hand preview shows the selected map directly from the game, including active
texture mods. Selecting another map loads its preview; combat, enemy spawns and the
run clock stay paused until **Start run**. **New run** returns to this selection.
No screenshot files or copied game textures are bundled for these previews.

| Arena | Native area | Layout |
| --- | --- | --- |
| Darknut Arena | `D_DLBR0`, room 51 | Original compact arena |
| Hyrule Field | `F_SP121`, room 6 | Faron main field |
| Gerudo Desert | `F_SP124`, room 0 | Desert and ruins |
| Palace of Twilight | `D_MN08`, room 0 | Exterior entrance platforms |
| Hyrule Castle Throne Room | `D_MN09A`, room 50 | Intact throne room |

Each uses native player point 0 on layer 0. Room/spawn identifiers were checked
against [TPGZ stage metadata](https://github.com/zsrtp/tpgz/tree/522f6c167996423a7634af53fe5840e30b217fd3/res/stage_info)
and the native stage loader. No guessed world-space spawn coordinates are used.

Stage enemies, NPCs, story-event triggers and vanilla reward chests are filtered
before creation only in the active Survivors arena. Native geometry, lighting and
doors remain. Magical curtains seal only native scene-exit polygons, exit
triggers and doorways. Open mesh edges, cliffs and ordinary room/material seams
do not create curtains. Curtains are tinted green in the field, amber in the
desert, violet in the palace and red in the throne room. Door interactions and
native scene exits are disabled.
Enemy and chest placement reject other rooms and paths across these barriers.
If the arena collision cannot be prepared, Start run stays disabled and another
map can still be selected. Story mode and Boss Rush retain their own rules.

## Run

Call Midna during a run on any arena, including the throne room, using the usual
controller binding or Midna touch button. Her native dialog offers only **Cancel**
and **End Run**. Cancel (also the back button) resumes play; End Run closes the
dialog and opens the existing results screen without awarding a victory. Run time,
automatic attacks and enemies pause while talking. Calling Midna interrupts
automatic attacks; chest interaction keeps priority if both are pressed together.

Random horde and scheduled-elite positions require dry native ground, safe
footprints and a sampled walkable path to Link. Native water-volume checks reject
riverbeds while allowing bridges above water; gaps, steep steps, walls and exit
barriers reject a candidate instead of falling back to an unsafe position.
Random Field/Desert guards use the same path check. Guard/pot footprints also
reject submerged or hazardous ground.

Every arena hosts a twenty-minute run.
Start with three hearts and automatic normal sword strikes. Sword Wave starts
locked, like Whirling Blade. Move, dodge and collect blue experience
crystals while weapons attack automatically. Manual Jump remains available with
your configured binding; manual sword attacks and arena exits are disabled.
Normal strikes aim at nearby enemies and use Link's native sword animation and
collision. They wait for the sword to be drawn and do not interrupt jumps, dodges,
damage reactions or item actions.
Automatic strikes advance through the native four-hit combo, including its
directional/lock-on variants and finisher. Follow-up strikes respect the native
hit and cancel windows; interruptions or two seconds without a new strike reset
the sequence. Normal swings, finishers and spin attacks all retain stick-controlled
movement through the native ground and wall collision handling. During normal
swings and finishers, facing follows the same live enemy every frame after movement,
without changing travel direction or camera lock-on. Dead, blocked, out-of-range
or elevated targets are released; a nearby reachable target can replace them.

Normal sword attacks also target nearby unbroken arena pots when no reachable
enemy is available; carried pots are excluded. Native sword collision breaks them.

The HUD shows time, level, kills, experience and the next chest price.
Active small chests also show the current price floating above them; opening and
free large chests have no floating price. Labels track the camera and shared
price rule, including after another chest is purchased. Only chests within
Link's 1,400-unit item-target range show prices, independently of camera zoom.
Weapon ranks and power-up details are available in the item wheel and Collection
overview; they are not repeated during combat.

Leveling up pauses Link and the encounter and offers three upgrades in the same
side-by-side card layout as chests. Every card uses the native Hidden Skills scroll
icon, its power-up name/description and red before/after changes. Controller,
keyboard and direct touch/click navigation use Dusklight's native mod window controls. Closing
the choice window does not resume combat or reroll its offers. Excess experience
is retained and multiple earned levels are offered consecutively.

| Upgrade | Effect |
| --- | --- |
| Sword Wave | Unlock aimed, piercing projectiles released during a normal sword swing; additional waves at levels 3 and 5 |
| Whirling Blade | Unlock or improve a native Spin Attack with freely steerable movement |
| Shield Attack | Requires a chest shield; automatic native shield bash, with damage, range and cooldown upgrades; retains stagger |
| Fire Arrows | Requires Hero's Bow; ignite enemies for 1 damage/second over 3 seconds; +1 periodic damage per rank and +1 second after every three further upgrades (rank 4) |
| Hero's Bow | Chest unlock; stronger/faster arrows, spread shots at ranks 3 and 5 |
| Boomerang | Chest unlock; Gale wind effect and one extra target at rank 1; each upgrade adds another hop and improves damage/size/cooldown |
| Bombs | Chest unlock; aimed ballistic throws with larger, stronger blasts and shorter cooldowns |
| Ball and Chain | Native throw/pull/catch animation and chain links; pierces to full range, damages each enemy once per leg and retains stagger |
| Power | Increase automatic weapon damage |
| Haste | Reduce automatic weapon cooldowns |
| Reach | Increase weapon size |
| Magnet | Attract XP, rupees, hearts and heart pieces from farther away |
| Heart Container | Add one maximum heart without healing |
| Luck | +10% Luck per rank (20 ranks, +200% max); weighted rarity chances; drop bonuses cap at +50% |
| Rupees | Receive 20 rupees per reward unit without healing |

All passive upgrades are independently available, including Heart Container
until the native maximum of 20 hearts. Weapons, Fire Arrows, Shield Attack and
passives have up to 30 levels; Luck retains its +200% cap. Rupees remain available after the
other upgrades reach their caps. Native enemy defense and death reactions still apply. Ordinary nonlethal hits
keep their hit effects and damage but do not interrupt enemy movement or attacks.
Shield Attack and Ball and Chain retain native stagger. Living knocked-down
Bokoblins keep their target spheres enabled before collision registration, so
normal attacks and abilities can still hit during knockback, prone and get-up
animations. Rats keep their real hit cooldown instead of resetting it each frame
of the damage animation. Native brief hit cooldowns, death states and the ball's
once-per-leg hit rule remain; ordinary hits do not restart the prone animation. The ball can hit an enemy
once on each leg, including a return hit during recovery from its outbound hit; the mod replaces the native nonlinear damage table only
for its own weapons and automatic sword strikes. Sword Wave always uses the first normal swing, never the combo finisher.
Sword Wave uses the two smaller
normal-spin particle effects, following the moving-emitter approach of Great Spin
Projectile. It does not use the large Great Spin effect or a debug sphere.

Each confirmed ordinary enemy kill has a base 5% chance to drop a native heart,
45% to drop a rupee and 50% to drop nothing. Luck multiplies the heart and rupee
chances; at +50% Luck these become 7.5%, 67.5% and 25%. A second roll chooses the
rupee's color. Experience crystals are awarded independently. Native random drops
are suppressed to prevent duplicate rewards. Level-ups never heal Link.

| Rupee color | Value | Base chance within rupee drops |
| --- | ---: | ---: |
| Green | 1 | 83% |
| Blue | 5 | 10% |
| Yellow | 10 | 4% |
| Red | 20 | 2% |
| Purple | 50 | 0.8% |
| Orange | 100 | 0.15% |
| Silver | 200 | 0.05% |

Luck also multiplies the non-green color chances, with green taking the remainder.
Hearts and all rupee colors use native pickup behavior. Magnet's range is
150 + 65 units per rank for XP and run-owned rupees, hearts and heart pieces.
Native pickup actors move toward Link only while waiting during active gameplay;
their item events, sounds and health/currency handling remain native. Owned
pickups pause with menus, retain pending creation through cleanup and retire at
run end. Story items are never attracted.

## Rarities and Luck

Each of the three level-up or chest choices independently rolls a rarity when
its offers are generated. Rarity controls how many ranks of the selected reward
are granted; it does not choose additional random items after confirmation.

The player-facing [Luck chart](../../SURVIVORS.md#rarities-and-luck) lists the
rounded probabilities. With `k = 1 + Luck / 100`, weights for Common through
Legendary are `60 / 25k / 10k² / 4k³ / k⁴`, normalized by their sum. Integer-scaled
weights and rejection sampling avoid modulo bias. All three cards roll separately
for level-ups, paid chests and free elite chests; no level/source bonus applies.
Luck has 20 ranks (+10% each, max +200%) and resets each run. Heart, heart-piece,
rupee frequency and rupee-quality bonuses retain their previous +50% effective
Luck cap. Existing offers never reroll merely because Luck changes or a window
is reopened; previews consume no randomness.

The reward name and icon border show its rarity color; each card also says
**Uncommon x2**, **Rare x3**, etc. Before/after values include the full grant. Native
Get Item names use the same rarity and quantity while changed values remain red.
Luck caps at 20 ranks; ordinary upgrades cap at 30. Each unit exceeding the cap becomes **20 rupees**,
shown in the preview/message and capped by the wallet when granted. This also
applies to extra Ordon Sword units and potion units beyond available bottle slots.
One chest still charges only once. Price increases are +5, +7, +9, +11 rupees, etc.

Bokoblins and Keese appear first; Rats, club Bulblins and bow Bulblins join after
30 seconds. Timed elites are Rat at **5:00**, Keese at **10:00**, bow Bulblin at
**15:00**, and Bokoblin at **20:00**. Victory requires 20 minutes and all four
confirmed elite kills. New horde spawns and guard respawns stop at 20:00; combat
continues for surviving timed elites. Menus, Get Item and Midna dialogue pause
the timer. All elite species have 120 base HP (before difficulty), double-sized
models and matching target radii.

Bulblins use their native club/bow AI and effects. Owned actors disable
`one_hit_kill`; scoped arrow classification avoids the native pre-damage HP-zero
branch. Damage/crit calculations are shared across the two native power checks
per hit. Burns and multihits enter native damage handling; nonlethal regular
hits preserve movement and animations, while Ball/Shield retain stagger.
Native enemy arrows have independent ownership for pause, damage difficulty,
shield blocks and deferred cleanup, even after their shooter dies.

Additional stationary elite guards wait at these map locations:

| Map | Guards |
| --- | --- |
| Hyrule Field | Four random, separated positions across the room's floor |
| Gerudo Desert | Four random, separated positions across the room's floor |
| Darknut Arena | None; scheduled elites only |
| Hyrule Castle Throne Room | None; scheduled elites only |
| Palace of Twilight | One in front of each authored building doorway |

Guards use native idle animation, physics, collision and damage handling, but
skip idle detection/social alerts. Only a damaging hit from Link wakes them.
Once provoked they pursue Link at any distance, just like ordinary and scheduled
enemies. Standing nearby alone does not provoke a guard. Placement uses loaded
DZB/KCL floor geometry and authored Palace door records, with floor, room,
hazard, footprint and exit-barrier checks. Retained Door/DRTG records recover
Palace entrances even when their actors are suspended. Darknut chest-landmark
capture and throne entrance/far-wall placement code have been removed entirely.
No map archives or assets are bundled.

Killed map guards respawn after **three minutes of active run time**, dormant
until attacked again. Hyrule Field and Gerudo Desert choose a new safe random
position, at least 1,800 units from the previous position and other guards;
Palace guards return to their landmark. Failed placement/creation retries without
resetting the timer. Scheduled five-minute elites do not respawn.
Living elites (guards and scheduled enemies) appear on the minimap using the
native Tear-of-Light bug marker: same texture, palette and size, updated to their
current position with the native room rotation/offset. Markers disappear on death
and return when the replacement has loaded. No story flags or treasure records
are modified.

Each killed elite leaves **one free large chest** at its death position (or its
last safe floor position). It uses native Dalways large-chest model 13 and lid
animation 8, not a scaled small chest. Opening shows the same three-card
controller/touch selection as paid chests, labelled as free. Choose one item;
only then does its native Get Item sequence start. Cancel keeps the offers.
Free chests cost zero rupees, never advance the paid-chest price, and disappear
once the reward completes. A separate pool retains all unopened drops from earlier guard lives, independent
of the six replenishing paid chests and the one-use map chests. Failed creation is retried; deletion, cleanup and repeated
death callbacks never generate extra loot. Guards and elites do not age-despawn.

The four scheduled elites additionally leave a gold-yellow EXP crystal, five times
ordinary crystal size, about 160 units from the corpse on connected dry floor.
Failed safe placement retries. Walking within 100 units consumes it and marks all
existing XP orbs for global attraction; later XP uses normal Magnet behavior.
The drop uses the same code drawing as EXP, without a native item actor, story
flag or Get Item hook. Map guards do not drop it. All four slots reset with the run.

Spawn frequency, ordinary population and Bokoblin/Bulblin health increase during the run.
Death opens a result screen with time, kills, level and weapon levels. **New run**
reloads the arena and resets all run upgrades and elite encounters.

## Breakable pots

Each run distributes native small pots at random safe floor positions throughout
the arena. Counts scale with loaded floor area (one per 2,000,000 square world
units), clamped to these map-specific budgets:

| Map | Simultaneous pots |
| --- | --- |
| Darknut Arena | 6–10 |
| Hyrule Castle Throne Room | 8–12 |
| Palace of Twilight | 12–24 |
| Hyrule Field | 40–72 |
| Gerudo Desert | 40–72 |

Creation is staggered to avoid loading every actor in one frame. Placement checks
floor, room, hazards, walls, exit barriers, other pots, guard landmarks and Link's
current position. Pots use native models, carrying/throwing, collision, fragments
and sound. Breaking one yields one native pickup: a Piece of Heart (2% base
chance) or a green, blue, yellow, red, purple, orange or silver rupee. Higher-value
rupees are rarer; Luck multiplies the heart-piece and better-color chances.
Heart pieces use the native Get Item sequence and five-piece heart progression;
subsequent Heart Container upgrades preserve already collected pieces. Their
capacity is reset with the rest of the run on restart.

Destroyed pots retry spawning after 600 playable ticks (20 seconds), at least
300 units from their break position. The old actor must finish deletion first. Failed creation is retried, while
unloading/deleting a pot never grants loot. Only these tracked pots bypass the
Survivors suppression of vanilla drops. Pots and enemies pause during menus and
native Get Item; the native reward event itself continues. Run end, mode changes
and restart retire the pots, including actors still loading.

## Treasure chests

At run start, seed 3/18/18/6/4 one-use small chests in Darknut/Field/Desert/Palace/
Throne Room. Their separate 18-slot pool retains placement flags after purchase;
they always cost 40, never advance regular prices, and share selection/Get Item.
Safe-position retries are bounded to 24 candidates per update. Affordability,
preview, payment, floating labels and paid-reroll reservations use the same price.
Starting chests use area-weighted room-floor sampling with local dry-ground,
room, exit and footprint checks. They do not require a straight walking line
from Link: that rejects distant sites around terrain obstacles and clusters
chests near the entrance. Minimum spacing from Link and between these chests is
half the square root of floor area per chest, clamped to 250–2,500 units.

Six replenishing paid treasure chests can be present at once, in addition to free elite and one-use map chests. The first appears after 20–30 seconds;
subsequent spawns are attempted at random 25–45 second intervals. Positions need
flat ground, a clear footprint and an unobstructed line from Link. Failed/full
placements retry after three seconds. Closed chests remain until purchased or the
run ends; opened chests disappear immediately when their native reward sequence finishes.

Approach a chest and use the native **Open** action. Without enough rupees
it has no interaction prompt. Opening an affordable chest takes priority over
automatic attacks and other actions: press Open to interrupt them immediately.
Auto-aim/spin facing cannot hide the prompt; distance and line of sight still apply.
There is no purchase/confirmation dialog. The HUD
shows the next chest price on a separate line: **20 rupees**, then **25**, **32**, **41**, **52**,
and so on (`20 + opened * (opened + 4)`). Opening shows three distinct random rewards side by side, with native
item icons, descriptions and red before/after upgrade values. Use controller
Left/Right and Confirm, keyboard navigation, or tap/click anywhere on a card.
Link, enemies and the run timer pause during selection. Cancel returns to combat
without a charge; reopening the same chest keeps its offers. Capped/unusable
rewards are excluded; if fewer than three remain, only those are shown. Cached
offers reroll only after every offered reward has become unusable.
Link receives only the selected reward through the native Get Item lift,
holding pose and item message while enemies and the run timer pause. The lid
opens during this sequence. Currency, reward and price are committed once the
sequence completes; failed loading or cancellation does not charge rupees.
The native Get Item text lists the upgrades changed by that purchase. Changed
values are red and show before/after values, for example **Block chance: 5% -> 10%**.
The preview is frozen with the selected reward; an unavailable message cancels
the attempt before creating the event or spending rupees.

Each chest samples its offers without replacement from the eligible reward types: Hero's Bow,
Boomerang, Bombs, Ball and Chain, Master Sword, an Ordon Sword for Dual Wield,
a shield upgrade, Dark Link and a bottle of Red Potion.
The four automatic item weapons unlock at rank 1; duplicates add their rarity's number of ranks up to 30, and unlocked weapons also enter the normal level-up pool.

Master Sword has five chest ranks. Rank 1 equips it and adds 3 damage;
each later rank adds 1, and **rank 5 equips the Light Sword** with its
native light effect (7 bonus sword damage total). Sword damage applies to melee,
Whirling Blade and Sword Wave, without the vanilla light-material 100-damage override.
Get Item uses a private instance of Link's sword model instead of the native rupee placeholder.
An Ordon Sword reward enables and equips Dawnlight's existing Dual Wield with an
Ordon offhand sword. At rank 1 each accepted normal sword strike also hits the next nearest reachable
enemy; ranks 3, 6 and 9 add one target each, up to five total. Dual Wield caps at rank 10.
Targets must be close to Link, at reachable height, and not behind a wall.
A target receives at most one multihit per swing; the native sword capsule cannot
also damage the same secondary target. Ranks do not change story preferences.

Dark Link uses the native **Poe Soul** icon and Get Item presentation. Rank 1
activates every 20 seconds of active gameplay for 5 seconds and doubles all
Survivors damage, including burning and Dual Wield hits. Its chest upgrades
alternate **−2 seconds activation interval** and **+1 second duration**. Rank 11
becomes permanently active (10-second interval meets 10-second duration) and
leaves the reward pool. Activation starts after the first interval; menus,
level-up choices and chest presentations freeze both timers. It uses Dawnlight's
existing Dark Link tint/eyes/aura without enabling manual activation, consuming
the story gauge or changing clothing. Restarting clears it.

Burns use the same native `LK_BURNS_A/B` particles, trace callback and 3x scale
as [Fire-Arrows-Standalone](https://github.com/baxlek/Fire-Arrows-Standalone).
A further arrow hit refreshes the duration without delaying the next damage tick.
Rank 1–3 burn for 3 seconds, rank 4–5 for 4 seconds; damage is the Fire Arrows rank
per second. Impact damage no longer receives the old Fire Arrows bonus. Damage
uses native enemy hit/death processing, with nonlethal stagger suppressed as for
normal sword hits. Burning particles and queued hits are cleared on death,
actor replacement, reset and mode exit.

Shields provide an automatic block roll against tracked enemy attacks. A successful
block turns Link toward the attacker and plays the native guard reaction, interrupting
automatic attacks. Idle lock-on does not grant free blocks. Shield and Dual Wield are mutually exclusive equipment choices. Taking the other
removes the current item and starts the replacement at rank 0, without storing old ranks.

| Shield ranks | Block chance | Reward model |
| --- | --- | --- |
| 1–3 | 5–15% | Wooden Shield |
| 4–6 | 20–30% | Ordon Shield |
| 7–10 | 35–50% | Hylian Shield |

Each shield rank adds 5 percentage points. At 50%, shields leave the chest pool.
After the first shield, Shield Attack can appear in level-up offers. These chest
shield ranks are separate from the 30 Shield Attack ranks.

Each unit of a potion reward adds a new filled bottle while fewer than four are
owned, even if an existing bottle is empty. Once all four slots are occupied,
it refills an empty bottle instead. Filled bottles are never overwritten. Red
Potion can be used normally from the item inventory; drinking it makes that
empty bottle eligible for a future refill. Four full bottles exclude potions
from the reward pool until a bottle is emptied. Equipment and potions are chest-only.

Obtained weapons, swords and the current shield also appear in the native item
wheel. Their explanation shows their current rank, automatic behavior, damage,
cooldown, range/area or block chance, including applicable collected upgrades.
Open the explanation again after a level-up to see the updated values.
Reward and inventory text retain native item message IDs, so reward icons match
all weapons, sword/shield tiers and potions. Wheel descriptions use the normal
text path, including Twilight HD HUD's item explanation panes. Get Item upgrade
text is rebound before sequence processing if a HUD rewrites it from the native
archive; the native reward ID/icon and normal dialog completion stay intact.
Only bottles can be assigned to item buttons; automatic equipment cannot be
assigned or combined. This also applies to extra bindings supplied by other mods.
These inventory entries belong only to the isolated Survivors save and reset
with the run. Red Potion occupies one equipment slot shared by all bottles, including empty ones.
It starts at rank 0 with one heart of healing. The unlocked Potion Healing power-up
adds a quarter heart per rank, up to 8.5 hearts at rank 30. Refills retain this rank;
only the red-potion drinking call changes healing, preserving other pickups and native bottle consumption.

Maxed weapons, a rank-5 Master Sword, an owned offhand sword and a 50% shield are excluded. New chests stop
spawning only while no useful reward is available. Each run resets the separate
Survivors inventory, sword/shield ranks, Dual Wield unlock, chest prices and weapon ranks, with
a giant wallet (at least 1000 capacity) and zero rupees.

All four weapons attack automatically alongside the sword without assigning
item slots or consuming ammunition. All four use Link's native upper-body animations while retaining movement: bow
draw/shoot, boomerang throw, bomb throw and ball throw/pull/catch. Link faces the target
and releases each projectile at its animation cue; higher fire rates accelerate
the animation. During ground locomotion and automatic sword, spin, shield and item actions, the stick
controls horizontal movement independently of animation lunges and slowdown;
releasing the stick stops that travel. The correction also spans the return from
a sword swing to locomotion and the remaining animation blend, avoiding a
speed dip after normal attacks or Sword Wave. Native floor/wall collision, gravity,
jumps, damage and item-get events retain their normal handling.
Sword, shield bash and item animations take turns; due item weapons rotate fairly.
The automatic iron-ball animation bypasses native manual iron-ball action and
aim transitions, so lock-on cannot cancel the throw before its release frame.
Arrows start at the native left-hand item matrix, including spread shots,
so the release follows the animated bow pose rather than a fixed body offset.
They aim at the nearest
live tracked enemy within range and line of sight. Arrows stop on impact;
boomerangs use the native Gale wind model/animation and particle trails. They
chain to one additional enemy at rank 1, then one more every three ranks
(4, 7, 10…), up to ten hops / eleven distinct targets at rank 30, before returning to Link. Each hop needs a live
target within range and line of sight, and cannot revisit an earlier target.
They deal Survivors damage on their outward/return paths. Bombs follow an
arc and explode at the target or a wall; the ball pierces outward to its full range and retracts toward Link.
The ball stays at its launch height on both passes, even when aiming at a flying
enemy or returning to a jumping Link; it may miss elevated targets.
Its chain uses the native iron-ball chain geometry/material. Bomb explosions use
all nine native normal-bomb particle effects, including splash/debris/smoke. Their
scale follows the upgraded blast radius, while the damage window stays bounded. Projectiles cannot pass through walls. Power, Haste and Reach also
apply. Custom bomb/ball hits use the linear Survivors damage path to avoid
vanilla Bokoblin instant-death/health-correction special cases. Native item/chest/chain models and bomb particles are reused; the chest lid
animates during the native reward presentation. These are run-local purchase
props. The demo item uses the native presentation-only flag; successful
completion explicitly grants Master Sword/bottles in the isolated Survivors
save. Story treasure flags remain unaffected.

## Power-up overview

In Survivors, the Hidden Skills scroll is always visible at its normal Collection
slot. Activating it with the controller or touch opens a read-only list of the
run's collected power-ups. Select a name to see its current rank, followed on
a separate line by its description and
effective values, including weapon ranks and dependent upgrades. No uncollected
power-ups are listed; an empty run shows a hint. Rupee rewards have no lasting rank.
Closing the overview returns to Collection. Actual Hidden Skill event flags and
story-mode menus are unchanged. Resetting/changing modes or destroying Collection
closes the overview and releases its window state.

## Records and saving

**Save records** on the result screen opens the game's native save prompt.
Complete that prompt before quitting to persist the records. The mode tracks
completed runs, wins, longest survival, most kills and highest level per save
slot. A new run keeps unsaved records in memory, but closing the application
without saving loses them. Failed runs still count as completed attempts.

Saving during combat does not suspend a run. Loading a Survivors save always
returns to a fresh start screen. Story and Boss Rush slots are not converted.

Dawnlight's combat presets, progression locks, HP scaling, Bullet Time, Flurry
Rush, manual Dark Link, stamina, sprint, glide and Gale are overridden for
this mode without rewriting their saved configuration. Manual Jump uses normal
height. Other mods can still change gameplay; cross-mod balance is not enforced.

## Initial scope and device testing

This is the initial arena implementation. It retains the native camera and uses
code-drawn crystals plus native chest/item/chain models,
sword animations and particle effects. No additional textures, models or music
are required. Larger arenas, camera changes, weapon evolutions, persistent
unlock trees and mid-run suspend are not part of this version.

The budgets are 372 tracked enemies (360 ordinary slots plus four scheduled
elites and up to eight guards, including pending/deleting actors), 40 attack
objects (eight base-weapon slots, twenty arrow slots and four per other chest weapon), six replenishing chest props, up to 18 one-use map chest props,
up to 60 free chest props, 64 experience crystals and up to 72 native pot actors. The free pool covers seven
lives per guard plus the four scheduled elites; opened slots are reused. Model allocation is bounded by a 3 MiB heap,
trimmed after loading; native pot actors use their own engine-managed heaps. Excess crystals merge their value and mark the recipient red; freeing/reusing a
slot resets that color. The gold-yellow global-magnet drop has four separate slots.
Native enemy arrows have a separate cap of 1116, including pending/deleting actors.
These limits are starting budgets, not a measured Android performance guarantee.
Spawns require a valid floor and unobstructed line from Link, and are staggered.
A failed elite creation/despawn can be retried without granting loot or counting as a kill.
Only confirmed deaths of initialized, tracked actors award experience.

Regression checks include three-card UI construction and activation, cached offers,
cancellation and deferred purchase, native arrow release positions, native item-icon selection, global-archive message
IDs, localized replacement text, ball flight at different target elevations,
and native movement/root-motion equations.

Before merging/releasing, test on Android and desktop:

1. Create and reload a separate Survivors slot; verify story and Boss Rush saves.
   Break/carry/throw pots on all maps; check rupee colors, rare native heart-piece
   pickup, menu/event pause and cleanup on End Run/restart.
2. Kill map guards and check their three-minute respawn, including pauses; check
   new Field/Desert positions and fixed Palace positions. Darknut Arena and Throne Room
   must have no additional guards. Leave earlier free
   chests unopened and confirm later drops remain separate. Verify native
   Leuchtkaefer/Tear-of-Light minimap markers follow living elites and disappear
   on death in every map and with Twilight HD HUD.
   Complete a run using all weapons and both dependent upgrades; check native enemy death, elite drops and automatic 20-minute victory
   and no HP scaling leakage. Verify jump, dodge, controller and touch controls.
3. Level up while surrounded; check three scroll cards, names/descriptions and
   red upgrade values with controller and touch. Close/reopen the choice window, and earn several
   levels at once. Verify Link/enemies and the timer stay paused during selection.
4. Die, save the result, restart, reload the slot, and decline the save prompt.
5. Reset, change modes and disable/reload Dawnlight while enemies or projectiles
   are present. Confirm no stale actors, collisions or windows remain.
6. Stress the population and crystal caps and check frame time/memory on Android.
7. Start without Sword Wave, hit nearby enemies with automatic normal strikes,
   verify the full four-hit combo while moving, then unlock Wave and verify a
   swing releases the small spin projectile. Verify the combo restarts after a
   damage reaction or a gap between targets, and permits movement during finishers.
   Circle an enemy at full speed; verify facing tracks it through the hit window,
   a passing enemy does not steal the target, and walls/death release the lock.
8. Steer in every direction during Whirling Blade, including into walls; verify
   native spin hits and that damage/dodges safely interrupt attacks.
9. Collect heart and rupee drops; verify leveling up (including Heart Container)
   does not heal and that restarting clears pickups and all weapon unlocks.
10. Approach chests with 19/20 rupees: only the affordable chest offers Open.
    Check three side-by-side cards with distinct icons/text and red upgrade deltas.
    Select each position with controller and direct touch; cancel/reopen and check
    unchanged offers/no charge. Check one/two remaining rewards near the caps.
    Check range/walls, native Get Item animation and lid motion, then
    prices 20/25/32/41 in the HUD with no purchase dialog or duplicate charges.
    Verify enemies/timer pause but Link and the item/camera finish normally.
    Press Open during sword/spin attacks, bow draw, boomerang throw and damage
    reactions; the chest must interrupt immediately without a delayed projectile.
11. Obtain all four chest weapons, test nearest-target selection, arrow impact,
    bomb blasts, returning boomerangs/balls and walls. Verify rank upgrades and
    simultaneous movement/automatic sword combos. Check native bow draw/shoot,
    facing and boomerang throw poses while moving and locking on. Verify faster
    animation with Haste, arrows leaving the bow at all facings/heights (including
    spread shots), Gale wind effects and 2/6 distinct targets at ranks 1/5.
12. Restart/change modes with closed/open chests and active item projectiles;
    verify prices, ranks, models, particles and collisions reset. Verify story
    treasure flags are unaffected and the next run restores Ordon Sword/no bottles.
13. Obtain five Master Sword ranks: verify native Light Sword at rank five,
    its glow and linear damage, and exclusion after rank 5. Collect four
    red-potion bottles, drink one, then obtain a refill. Check new-bottle priority
    over empty bottles, exclusion with four full bottles, and instant chest removal.
14. Obtain Ordon Sword and confirm Dual Wield. Obtain all ten shield ranks, checking
    5–50% block chance and models at 20%/35%. Check attack-facing native block/bash
    animation, return to Dual Wield, and Shield Attack unlock only after a shield.
15. Confirm ordinary nonlethal hits preserve enemy movement/attacks and hit particles;
    shield bashes and balls still stagger. Test a ball through a group, including
    outbound/return hits at the turnaround. Verify native chain and throw/pull/catch.
16. Compare bomb rank/Reach blast visuals and radius. Unlock Fire Arrows after Bow,
    then check 1 damage/second for 3 seconds, refresh while firing rapidly, higher periodic damage and the rank-4 duration increase. Check cleanup on death/restart/mod unload.
17. Fire Ball and Chain while standing, moving and locking on. Check release,
    outbound travel, pull/catch, and interruption by damage or opening a chest.
18. Check each chest reward's Get Item text, including any additional pages.
    Verify only changed values are red, shield 5% -> 10%, spread-shot unlocks,
    fifth-sword evolution, new bottles/refills, and no grant after cancellation.
19. Open Collection at the start of a run: the native Hidden Skills scroll should
    be visible and open an empty power-up list. Reopen after several upgrades and
    select each row with controller/touch; verify names, levels and effective
    descriptions, return to Collection and cleanup on reset. Test Twilight HD HUD
    enabled/disabled and verify the story Hidden Skills screen remains unchanged.
20. Open the item wheel after each reward and after level-up upgrades. Check
    weapon/sword/shield icons and current descriptions. Try X/Y, touch and
    external-mod extra bindings: automatic equipment must remain unassigned;
    full/empty bottles must remain assignable and usable. Check a story save
    afterward to confirm its normal assignments and descriptions are unchanged.

21. Compare all rarity colors on names and icon borders, quantities and full deltas.
    Test multi-rank grants near a cap, shield/sword evolution, potion batches with
    mixed empty/full/missing bottles, overflow currency and cancel/reopen stability.
    Check Luck's +10% increments through +200%, cap/overflow, reset and appearance in the power-up overview.

22. Collect Dark Link, verify the Poe Soul icon/Get Item text and automatic-only
    wheel slot, first activation at 20 seconds for 5 seconds, x2 damage for every
    weapon/burn/multihit, pause behavior and permanent rank 11. Check that leaving
    Survivors restores normal story Dark Link settings and gauge behavior.
23. Compare Dual Wield ranks 1–10 against a close group (2–5 targets). Check walls,
    high/far enemies, duplicate capsule hits and ordinary native death rewards.

Automated tests cover burn timing/refresh/final ticks and emitter cleanup,
Dark Link timing/pause/caps/visual isolation, nearest-target multihit selection,
duplicate suppression and rarity distributions through +200% Luck and the separate +50% drop cap, colored card
previews, multi-rank grants and overflow, potion transactions and cancellation,
continuous melee facing while orbiting, target retention/replacement, rules, caps, upgrade choice integrity, record encoding,
true-death accounting, deferred spawn cleanup, startup/reset routing and shared
Boss Rush arena admission, sword scheduling with the native combo selector,
movement during all three sword procedures, wave timing and
particle cleanup, drop gating and level-up rewards. Loot tests additionally cover
purchase pricing/caps, guarded transactions, placement, all four trajectories,
target selection, boomerang chains, native ranged-animation release timing,
interruption/priority, fairness, model-load failures and shared-animation/heap
cleanup. Defense tests cover block probabilities/direction, nonlethal reaction
restoration, stagger exceptions, lethal hits and Light Sword material isolation.
Equipment tests cover all sword/shield tiers and run-only Dual Wield overrides.
They do not replace the in-game checks above.

For these presentation/movement fixes, verify chest icons for every reward and
shield/sword tier, and reopen upgraded item descriptions with Twilight HD HUD
both enabled and disabled. Move, reverse direction and release the stick during
all automatic attacks (including block, bash, and ball pull/catch), then verify
that damage, jumping and chest item-get still finish normally.


For the additional maps, in-game verification remains required on an actual game
installation: load each arena from both a new and existing Survivors save, switch
previews repeatedly, start/replay, inspect every doorway and outdoor exit, roll
along the barriers, and check chest/enemy placement near slopes and platforms.
Verify the Palace exterior and intact throne room load without story events.
The automated checks exercise stage-routing gates, map-selection/start callbacks,
room-aware chest placement, mesh seam welding, exits, continuous barrier collision,
sliding, corners and fast movement; they do not render the actual room archives.

## Run options

The preparation window has Maps, Difficulty and Settings tabs. Session-local
options survive map changes/replays and are copied into the run at Start. HP and
damage range from x1–x10; movement and Horde from x1–x5. Native enemy HP is scaled
once at initialization. Movement is scaled before native ground/wall correction,
excluding dormant, dying and knocked-back actors. Enemy damage scales at Link's
native damage entry, restricted to owned enemy contacts.

Random horde events roll independently once per playable second (1/150 chance),
for about two events per five minutes, with no quota. Each adds 450 ticks to
remaining duration; active events multiply spawn batches by three, never by nine.
The normal population cap remains enforced. The HUD shows a warning/countdown;
menus freeze both trials and duration, and 20:00 clears/stops random hordes.

Horde difficulty multiplies population and per-wave spawns (up to 345 normal enemies);
every Nth normal kill generates XP and rolls loot. Elites and pots are exempt.
Camera distance ranges from 100–500% (default 200%), scales the rendered view
with a wall check and restores native input before the next camera tick to avoid
feedback. Event/aim cameras stay native. Timed-elite EXP magnets are drawn at 5x ordinary crystal size.
