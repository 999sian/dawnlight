# Hyrule Survivors

[← Dawnlight overview](../../README.md) · [Player guide and Luck chart](../../SURVIVORS.md)

Hyrule Survivors is a separate game mode included in `dawnlight_mod.dusk`.
Select it in Dusklight's game-mode menu and create or load a Survivors save.
It uses `gczelda2-survivors`, separate from the story and Boss Rush save files.
The title screen, save selection, Link/Epona names and optional TV settings finish
before the arena loads. Selecting the mode alone cannot start a run. Returning
to the title screen disarms the run until another save is selected and started.

## Hub and arena selection

Loading a Survivors save enters the Darknut Arena hub with normal movement and
no automatically opened menu. The Master Sword in its stone pedestal stands in
the room's center. The floating **Talents** label uses the native boss-name font.
Face the sword and press A (including touch A) to open **Talents / Settings**.
Closing the menu automatically saves talent progress to the current Survivors
slot. **Save progress** remains available for an explicit retry after a failed save.

The exit door carries a floating **Start Run** label. Interacting with it opens
**Maps / Difficulty / Settings / Debug** (Debug only in debug-enabled builds).
Selecting maps updates the description and high score while Link stays in the hub.
**Start run** closes the menu, waits for background saving and the UI to finish,
then orders the original native door event. The door opens and Link walks through;
only the event's final scene change is redirected to the chosen map. Combat and
the timer start after arrival, grounding and collision preparation. Cancelling the
menu leaves Link in the hub. **New run** returns to the hub with the last arena selected.

The **Survivors** tab beside **Boss Rush** in Dawnlight Settings mirrors the same
Survivors Settings controls. Camera distance, Reticle Y, attack/lock-on swap,
gyro and all ten manual-ability switches share persistent config values; difficulty
is persisted too. They participate in Dawnlight settings export/import. Edit outside
a run; active runs keep their snapshot. Talents remain specific to each save slot.

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
native scene exits are disabled during runs. The hub retains its usable exit door.
Enemy and chest placement reject other rooms and paths across these barriers.
If collision is not ready, combat does not begin. Story mode and Boss Rush retain their own rules.

## Run

Call Midna during a run on any arena, including the throne room, using the usual
controller binding or Midna touch button. Her native dialog offers **Cancel**,
**End Run** and **Restart Run**. Cancel (also the back button) resumes play; End Run closes the
dialog and opens the existing results screen without awarding a victory.
Restart Run waits for the same native dialog/event closure, credits the run once,
then reloads the hub with the previous map selected for another run.
The results screen is skipped. Transition/reset discards pending actions. Run time,
automatic attacks and enemies pause while talking. Calling Midna interrupts
automatic attacks; chest interaction keeps priority if both are pressed together.

Random horde and scheduled-elite positions require dry native ground, safe
footprints and a sampled walkable path to Link. Native water-volume checks reject
riverbeds while allowing bridges above water; gaps, steep steps, walls and exit
barriers reject a candidate instead of falling back to an unsafe position.
Random Field/Desert guards use local path checks in eight directions over 600
units, with 3,000 units of clearance from the original start and 1,800 from
Link. They sample the whole room without a straight-line path to Link, so an
entrance tunnel cannot attract the entire guard population. Guard/pot footprints also
reject submerged or hazardous ground.

Every arena hosts a twenty-minute run.
Start with three hearts and automatic normal sword strikes. Sword Wave starts
locked, like Spin Attack. Move, dodge and collect blue experience
crystals while weapons attack automatically. Manual Jump remains available with
your configured binding; arena exits are disabled. Pressing or holding the mapped
sword button aims all unlocked attack families at the reticle by default.
The Settings **Manual aiming abilities** list independently controls Sword Attack,
Spin Attack, Jump Strike, Sword Wave, Bow, Boomerang, Bombs, Ball and Chain,
Shield Attack and Shockwave. Each switch defaults to on. Off keeps that family's
automatic target gates and priorities even during a manual hold; on only overrides
targeting during a tap/hold. Release resumes automatic attacks. These are next-run
settings and never unlock skills or bypass cooldowns. Jump Strike retains its
nearby-enemy requirement and proc chance, jumps in place and stays steerable.
Spin remains centered on Link; close-range attacks retain their reach.
Fire Arrows, Bomb Arrow, Fire Bomb, Fire Edge and Hurricane inherit their base
weapon's targeting; auras and passive effects remain automatic. Native
charged spin attacks remain suppressed. Holding uses the shared Dawnlight
third-person shoulder camera and reticle without replacing Link's movement or
attack procedure. Releasing, pausing, events, run resets and mode exit restore
the normal camera and clear the owned sight.
Settings adds a separate **3rd Person Reticle Y** (-40..40, default 0), copied
into the next run. It only adjusts the Survivors sword-aim view; the global
Dawnlight item-aim setting is unchanged. **Switch Attack/Lock-on Keybinds**
(default off) swaps the accepted B/L gameplay inputs, including analog trigger
edges, while menus keep their normal bindings. Touch moves the native sword
image to L and makes L momentary for attack holds; B becomes lock-on.
**Gyro Aim** (default on) adds motion control to the held shoulder view with
native gyro sensitivity/inversion. This run-local toggle is independent of the
global item gyro enable setting. Stick orbit and normal movement remain available;
sensor demand is shared with bullet time and released on leaving the view.
Directional projectiles converge from their actual launch origins onto the rendered
reticle before their first movement. Bombs follow their ballistic arc to the aimed
point within weapon range. Boomerangs keep their aimed first leg, then use earned
automatic hops without revisiting crossed enemies. Manual Ball and Chain can aim
vertically and returns to Link. Shield and Shockwave retain their shared cadence
while honoring their separate target-selection switches. Manual Bow/Sword Wave
multishot keeps one central projectile and its extra projectiles retain their fan;
other manual multishots share the aimed point. In-flight projectiles never retarget.
The Palace exterior uses the standard DungeonS gameplay camera base before
Survivors distance scaling, avoiding the authored distant-camera preset stacking
with the setting. Event, tag and native aiming cameras retain their own types.
Normal sword acquisition/retention distances and its blade collision length start
at 130% of their original base values, before Reach scaling.
Normal strikes aim at nearby enemies and use Link's native sword animation and
collision. They wait for the sword to be drawn and do not interrupt jumps, dodges,
damage reactions or item actions.
Automatic strikes advance through the native four-hit combo, including its
directional/lock-on variants and finisher. Follow-up strikes respect the native
hit and cancel windows; interruptions or two seconds without a new strike reset
the sequence. Normal swings, finishers and spin attacks all retain stick-controlled
movement through the native ground and wall collision handling. During automatic normal
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
side-by-side card layout as chests. Cards use the native Hidden Skills scroll, full Heart Container, or rarity-colored
rupee icon, its power-up name/description and red before/after changes. Controller,
keyboard and direct touch/click navigation use Dusklight's native mod window controls. Closing
the choice window does not resume combat or reroll its offers. Excess experience
is retained and multiple earned levels are offered consecutively.

| Upgrade | Effect |
| --- | --- |
| Sword Wave | One aimed, piercing projectile per cast plus Multishot; becomes a larger Great Spin projectile at rank 15 |
| Spin Attack | Automatic Spin Attack while moving; becomes Great Spin Attack at rank 15 with a larger radius |
| Shield Attack | Requires a chest shield; automatic native shield bash, with damage, range and cooldown upgrades; fixed small knockback |
| Fire Arrows | Requires Hero's Bow; ignite enemies for 1 damage/second over 3 seconds; +1 periodic damage per rank and +1 second after every three further upgrades (rank 4) |
| Hero's Bow | Chest unlock; one arrow at ranks 1–9, three at 10–19, five at 20–30; symmetric fan up to 45°; same-volley extra hits per target deal 25% damage |
| Boomerang | Chest unlock; Gale wind effect and one extra target at rank 1; each upgrade adds another hop and improves damage/size/cooldown |
| Bombs | Chest unlock; aimed ballistic throws with larger, stronger blasts and shorter cooldowns |
| Ball and Chain | Native throw/pull/catch animation and chain links; pierces to full range, damages each enemy once per leg and fixed small knockback |
| Power | Increase automatic weapon damage |
| Haste | Reduce automatic weapon cooldowns |
| Reach | +7.5% linear reach/range per rank; Spin/Bomb/Shield radii use the square root of that factor |
| Magnet | Attract XP, rupees, hearts and heart pieces from farther away |
| Heart Container | Add one maximum heart; level-up healing fills the new capacity |
| Luck | 20 ranks; rarity weights +5% and crit multiplier +2.5% per rank; drop bonuses +10% per rank capped at +50% |
| Knockback | 10 ranks; +4 units horizontal push/rank on direct attacks; fixed 24-unit Ball/Shield base; elites take half |
| Piercing | Requires Bow; 10 ranks, +1 distinct damaged target per arrow/rank, including Multishot arrows |
| Multishot | Epic +1 rank, Legendary +2, cap 3; +1 projectile/rank for all five ranged abilities; arrows retain the existing fan angle |
| Life Regen | Rare or better; cap 5; -1 second/rank from passive regeneration; stacks with the talent |
| Life Steal | Legendary only, cap 1; +¼ heart per credited enemy death |
| Rupees | Receive 20 rupees per reward unit |

All passive upgrades are independently available, including Heart Container
until the native maximum of 20 hearts. Weapons, Fire Arrows, Shield Attack and
ordinary passives have up to 30 levels; Luck retains its 20-rank cap and the new power-ups use the caps above. Rupees remain available after the
other upgrades reach their caps. Native enemy defense and death reactions still apply. Ordinary nonlethal hits
keep their hit effects and damage but do not interrupt enemy movement or attacks.
Shield Attack and Ball and Chain use a bounded horizontal push instead of native launches. Its base never grows with weapon ranks, damage or Reach; only Knockback adds distance. Six short steps, a twelve-tick repeat delay, non-additive simultaneous hits and terrain checks limit movement. Living knocked-down
Bokoblins keep their target spheres enabled before collision registration, so
normal attacks and abilities can still hit during knockback, prone and get-up
animations. Rats keep their real hit cooldown instead of resetting it each frame
of the damage animation. Native brief hit cooldowns, death states and the ball's
once-per-leg hit rule remain; ordinary hits do not restart the prone animation. The ball can hit an enemy
once on each leg, including a return hit during recovery from its outbound hit; the mod replaces the native nonlinear damage table only
for its own weapons and automatic sword strikes. Sword Wave keeps its own cooldown but releases at the attack-start frame of
a visible normal sword cut. An existing normal cut can carry the wave; at ranged
distance it starts a new first cut, drawing the sword if needed. Interrupted
wind-ups cancel, finishers are left intact, and items/spin/shield keep their own
independent schedulers.
It emits one piercing projectile plus one per Multishot rank, up to four in a symmetric 36-degree fan. At rank 15, both Sword Wave and
Spin Attack switch from the two normal-spin particles to the six Great Spin
particles. The wave base radius increases from 65 to 100; Spin Attack uses the
native normal radius (260) before rank 15 and Great Spin radius (400) thereafter,
independently of Master Sword rank. Reach still scales the radii, and particle
scale follows the actual hitbox. In-flight waves retain their launch tier.
The ability name changes from Spin Attack to Great Spin Attack at rank 15.

Each confirmed ordinary enemy kill has a base 2.5% chance to drop a native heart,
45% to drop a rupee and 52.5% to drop nothing. Luck multiplies the heart and rupee
chances; at +50% Luck these become 3.75%, 67.5% and 28.75%. A second roll chooses the
rupee's color. Experience crystals are awarded independently. Native random drops
are suppressed to prevent duplicate rewards. Every level-up fully heals Link to the native maximum life gauge. Heart Container choices also fill the new capacity.

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
rounded probabilities. With `k = 1 + 0.05 × Luck rank`, weights for Common through
Legendary are `60 / 25k / 10k² / 4k³ / k⁴`, normalized by their sum. Integer-scaled
weights and rejection sampling avoid modulo bias. All three cards roll separately
for level-ups, paid chests and free elite chests; no level/source bonus applies.
Luck has 20 ranks (+5% rarity weighting each, max +100%) and resets each run. Heart, heart-piece,
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

`survivors_roster.hpp` defines each map's basic pool, timed specialist unlocks,
ordinary caps and four elite species. See [the player guide](../../SURVIVORS.md#maps-elites-and-pots)
for the exact tables. Staltroops are deliberately excluded, including their native
hooks. Basic/specialist weights are 75/25 normally and 90/10 during hordes;
capped or still-locked specialists fall back to basic species. Pending actors count
toward species caps, while timed elites and dormant guards do not.

Victory requires 20 minutes and all four confirmed timed elite kills. New horde
spawns and guard respawns stop at 20:00; combat continues for surviving elites.
Menus, Get Item and Midna dialogue pause the timer. Elites have 120 base HP before
difficulty. Giant variants use double models and target radii; Helmasaurus,
Dynalfos, Stalfos, Aeralfos, Darknut, Stallord’s head and Zant’s Hand retain native size.

The extended roster adapters retain native models, movement and attack states,
but replace puzzle-specific damage handlers for run-owned actors. Registered body
and armor targets accept Survivors attacks; one shared damage pass applies power,
crit and status hooks, cooldowns, and Ball/Shield stagger. Defeat credits the enemy
before requesting deletion and uses the native disappearance effect. This prevents
armor HP resets, resurrection and Chu splitting from losing kills or duplicating
loot. Chu merging and Gibdo paralysis are suppressed only for run-owned actors.
Helmasaur shell contacts route to parent HP regardless of parent/child execution
order. Shells retire with their owner; Bomskit eggs and Bulblin arrows retain
independent ownership for pause, difficulty, defense and deferred teardown.
Native hooks pass through for actors outside Survivors ownership.

Helmasaur armor is a structural child: it may finish creation during upgrade/treasure
pauses, with one child slot per enemy reserved against projectile saturation.
Failed/missing armor is retried once per playable second, pending creates are
retained, and the attached state is maintained for live run-owned Helmasaurs.
The body remains a damage target while armor loads; death/teardown still retires
its shell with the parent.

Gerudo's basic/specialist roster still includes Stalhounds and Dodongos. The four
bosses on each map are listed in the player guide; ordinary pools are unchanged.
Native-size bosses include King Bulblin, Death Sword, Twilit Bloat, Phantom Zant,
Darkhammer, Puppet Zelda and Ganondorf. Shadow guards retain their dormant behavior.
Stalhound Execute temporarily sees midnight, with nested-process restoration.

`survivors_bosses.inc` adapts native boss entry states and arena anchors. Native
attack dispatch stays active for King Bulblin, Death Sword, Twilit Bloat, Phantom
Zant, Darkhammer, Puppet Zelda and Ganondorf. Death Sword's opaque chase/strike
and two-shot attack avoid world-origin orbit states. Bloat's electric charge uses
Link's local floor instead of lake elevation and stays visible without wolf senses.
Phantom receives reachable teleport points and retains native volleys and portals,
using the map's Shadow Keese/Beast minions as owned ordinary enemies. Its projectiles
and portals retire with the boss. Darkhammer's ball and Ganondorf's cloak are
structural children, with pending-create guards/retries before native dereferences.
Native puzzle damage and off-map body spheres cannot reintroduce per-enemy i-frames.
Darknut's armor loss follows run HP, then uses the native non-cinematic sword throw
and light-armor combat phase. Boss intros, camera/QTE transitions, dungeon switch
writes and arena-specific weather changes are isolated to owned actor scopes.

`survivors_encounters.inc` keeps Stallord's native phase-two action/shot/recovery,
replacing tower movement and hover anchors and excluding rail/trap spawning. The
Carrier's native rider-dependent catch sequence retains approach, braking,
telegraph and catch poses, using a talon collider in place of the capture cinematic.
The elite Carrier adds one owned mounted archer with native shooting animations.
The unused Zant Hand adapter remains available but is no longer a scheduled boss.
Run pause, teardown, HP scaling and final HP budgets apply to all new bosses.
Desktop/Android in-game validation of the new encounters remains required.


Boss balance: active elites/bosses move 50% faster. Promoted ordinary elites have
half attack-readiness/recovery timers and double damage; native bosses/minibosses
do not inherit those two generic factors. Explicit damage overrides are Darknut
×2; Death Sword, Stallord, elite Carrier and Puppet Zelda ×4; Bloat and Darkhammer
×3; Ganondorf and Phantom Zant ×5. Overrides do not stack with the generic ×2.
King Bulblin can spin every six seconds at a safe chase boundary. Darknut repeats
his sword throw every 1.5 active seconds outside melee range in either phase.
Death Sword can enter ranged combat after four seconds of distant pursuit;
Stallord can start breath out to 6,000 units without Link's damage-protection gate.
Bloat can attack immediately while retaining its telegraph. Ganondorf charges
from distant WAIT with a shared five-second cooldown and summons five native
ghost riders every eight active seconds. Summons/riders retire with their boss;
all owned children pause and clean up with the run. Full native baselines and
current overrides: [boss attack reference](hyrule-survivors-boss-attacks.md).

Dodongo armor flags are also normalized after Execute, because its native code
sets them after collision registration. Only owned target objects are changed;
attack colliders retain their native role. The fixture exercises the actual new
action hooks over complete cycles, ownership boundaries, pause/retirement,
phase-two spawn restoration, fireball production, Carrier dives/mounted archery, orb-free
slams, nested clock/flag scopes and slot reuse. Runtime rendering, attack feel,
terrain interaction and device performance still require in-game checks.

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

Guards (Bokoblins, or Shadow club Bulblins in the Palace) use native idle animation, physics and collision, but
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

Each scheduled elite and first kill of a guard slot leaves **one free large chest** at its death position (or its
last safe floor position). It uses native Dalways large-chest model 13 and lid
animation 8, not a scaled small chest. Opening shows the same three-card
controller/touch selection as paid chests, labelled as free. Choose one item;
only then does its native Get Item sequence start. Cancel keeps the offers.
Free chests cost zero rupees, never advance the paid-chest price, and disappear
once the reward completes. Guard respawns do not repeat the chest reward. A separate pool retains unopened first-kill drops, independent
of the six replenishing paid chests and the one-use map chests. Failed creation is retried; deletion, cleanup and repeated
death callbacks never generate extra loot. Guards and elites do not age-despawn.

The four scheduled elites additionally leave a gold-yellow EXP crystal, five times
ordinary crystal size, about 160 units from the corpse on connected dry floor.
Failed safe placement retries. Walking within 100 units consumes it and marks all
existing XP orbs, hearts and rupees for global attraction; later drops use normal
Magnet behavior.
The drop uses the same code drawing as EXP, without a native item actor, story
flag or Get Item hook. Map guards do not drop it. Ordinary rewards can also roll this drop
(2%, up to 3% with Luck). All slots reset with the run.

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
Spin Attack and Sword Wave, without the vanilla light-material 100-damage override.
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
Jump Strike also retains normal stick movement throughout launch, flight and
landing. Its nearby enemy requirement and proc chance remain, but the native
target-directed launch is skipped: without stick input, Link jumps in place.
Manual Sword Wave uses the shared reticle ray, including vertical offset/FOV,
and converges from Link's actual launch position onto the nearest ray hit on
terrain or registered enemy collision shapes. Fresh waves receive their final
direction after the camera update, before their first movement/collision frame.
Existing projectiles retain their trajectories. Multishot keeps a centered wave
and adds spread shots; other weapons keep their ordinary targeting.
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
are required. Larger arenas, camera changes, further weapon evolutions, persistent
unlock trees and mid-run suspend are not part of this version.

The budgets are 372 tracked enemies (360 ordinary slots plus four scheduled
elites and up to eight guards, including pending/deleting actors), six replenishing chest props, up to 18 one-use map chest props,
up to 60 free chest props, 64 experience crystals and up to 72 native pot actors. The free pool covers seven
lives per guard plus the four scheduled elites; opened slots are reused. Treasure and initial projectile models use a 3 MiB heap,
trimmed after loading; native pot actors use their own engine-managed heaps. Excess crystals merge their value and mark the recipient red; freeing/reusing a
slot resets that color. The gold-yellow global-magnet drop reserves four boss slots and grows reusable
ordinary-drop storage as needed.
Player projectiles have no fixed pool ceiling. The initial 42 collision slots
(eight waves, twenty arrows, four each for boomerangs/bombs/balls, two skill pulses)
are reusable reservations. Additional waves and item projectiles append stable
collision objects and reuse inactive entries of the same weapon. Each extra
item model owns a trimmed heap; all model heaps are released on run teardown.
Collision storage and chain packets retain stable addresses across reuse.
Allocation failure can still prevent a release; engine renderer/collision/particle
budgets and available memory remain finite. Native enemy arrows have a separate
cap of 1116, including pending/deleting actors.
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
   Check free movement on load, the central sword and both floating boss-font labels.
   Open Talents with A/touch A, change/refund ranks and settings, close, then reload
   the slot/app and verify persistence. Compare the mirrored Dawnlight Survivors tab.
   Cancel the door menu and verify no departure; confirm each of the five maps,
   checking menu fade/save completion before native door animation and the selected
   destination after the scene transition. Verify starting-item choice/timer only
   begin after arrival, and failed saving keeps Link in the hub with a retry message.
   Confirm Hero's Shade's original pedestal still starts its encounter normally.
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
3. Level up while surrounded; check three upgrade cards with the correct native icons, names/descriptions and
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
   Check 130% base sword reach before buying Reach. Tap/hold the sword button
   and verify all enabled attack families aim manually. Toggle each family off
   individually and verify its automatic target gate, priorities and direction
   while the others still follow the reticle. Test all-off and mixed selections,
   shield/shockwave independently, aimed bomb arcs, vertical ball return and
   boomerang/Hurricane first legs followed by earned hops. Check no native charge/spin
   appears on hold/release. Set Survivors Reticle Y to -40/0/+40 and verify the
   held shoulder view, centered wave/arrow with every multishot rank, nearby enemies,
   walls and camera movement. Confirm the global item-aim offset is unchanged.
   Enable the B/L swap and verify controller/keyboard/analog and touch hold/release,
   sword icon on L, lock-on on B, and normal menu bindings. Check gyro on/off,
   inversion and sensitivity, combined stick input, bullet-time handoff and cleanup.
   Compare Palace distance at 100/200/500% to the standard base and check camera
   wall clipping and native event views. Acquire Fire Edge and inspect flames on
   drawn/sheathed Ordon and Master Sword through upgrades, pauses and restart.
   Release, pause, open a chest, call Midna, restart and change modes while aiming;
   check restoration of the normal camera and sight cleanup. Trigger Jump Strike
   near an enemy with no stick input, then steer through takeoff/flight/landing;
   verify there is no enemy-directed lunge and wall/floor collision remains.
8. Steer in every direction during Spin Attack, including into walls; verify
   native spin hits and that damage/dodges safely interrupt attacks.
9. Collect heart and rupee drops; verify leveling up (including Heart Container)
   fully heals, including queued levels, and that restarting clears pickups and all weapon unlocks.
   Engage a chest guard and each map boss: the red native oil/oxygen-style bar
   and RubyFont boss name appear below XP. Damage changes the fill; defeat
   switches to another active elite or hides the bar. Sleeping guards stay hidden.
   Check native oil, oxygen, stamina and Fierce Deity bars remain unchanged, and
   check boss name/bar spacing at 4:3, widescreen and with HUD customization.
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
    Check Luck's +5% rarity weighting increments through +100%, cap/overflow, reset and appearance in the power-up overview.

22. Collect Dark Link, verify the Poe Soul icon/Get Item text and automatic-only
    wheel slot, first activation at 20 seconds for 5 seconds, x2 damage for every
    weapon/burn/multihit, pause behavior and permanent rank 11. Check that leaving
    Survivors restores normal story Dark Link settings and gauge behavior.
23. Compare Dual Wield ranks 1–10 against a close group (2–5 targets). Check walls,
    high/far enemies, duplicate capsule hits and ordinary native death rewards.

Automated tests cover burn timing/refresh/final ticks and emitter cleanup,
Dark Link timing/pause/caps/visual isolation, nearest-target multihit selection,
duplicate suppression and rarity distributions through Luck rank 20 and the separate +50% drop cap, colored card
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
installation: load new and existing Survivors saves into the hub, switch map
selections repeatedly, start/replay, inspect every doorway and outdoor exit, roll
along the barriers, and check chest/enemy placement near slopes and platforms.
Verify the Palace exterior and intact throne room load without story events.
The automated checks exercise stage-routing gates, map-selection/start callbacks,
room-aware chest placement, mesh seam welding, exits, continuous barrier collision,
sliding, corners and fast movement; they do not render the actual room archives.

## Combat follow-up

Close-range enemy facing is corrected before native execution and after ground
correction, before models and attack colliders are built. This also refreshes
the rat leap heading and Keese attack target without changing dormant, hurt or
death states. Successful ordinary spawns, a full population, and the final elite
fight reset the consecutive placement-failure warning.

The appended Dodge talent uses basis-point chances 500/625/750/875/1000 and the
same unbiased chance helper as critical hits. It precedes shield block, works
without a shield, and adds no invulnerability timer or attack interruption.
Records v3 append its rank; v1/v2 saves retain existing progress and start Dodge
at zero. Block/Dodge/Crit use a bounded pool of 64 rising, fading labels. Crit
requires actual health loss and carries the original roll through Bulblin's
power cache and queued Dual Wield hits; burn ticks cannot generate it alone.

During native player-hit processing, positive damage temporarily enables the
native no-reaction branch. Health damage still runs, the temporary mode bit is
restored afterward, and new normal-hit immunity is cleared independently of
hook order. Survivors forces No Normal-Hit Invulnerability without changing the
saved setting. Interrupted sword draws retry using the actual equip animation;
a newly drawn sword gets a ready melee opportunity before the next item handoff.

Regression coverage includes enemy facing, text lifecycle/projection, applied
crit feedback, Dodge probabilities, v2 migration, scoped hit reactions, setting
restoration and interrupted equip/item scheduling. In-game checks remain needed:
circle every enemy type during its attack; interrupt sword draws with hits and
item actions; verify melee/waves/spins resume with all ranged weapons unlocked;
check Block/Dodge/Crit positioning and fading; and compare health loss, flinch
and repeat contacts with Survivors enabled and after returning to story mode.

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

### Starting Item talent

Starting Item costs one Unlock Point and has one rank. Each new run opens a
free three-card chest-reward selection before timer/combat advance. It reuses
normal eligibility/rarity and the run's reroll/banish budgets. An inactive free
chest slot holds the offers only while paused; no model or native item event is
created. Closing the window retains offers and reopens the choice. If all cards
are banished, Continue consumes this run's starting choice without a reward.

Selection is committed once after the UI closes, through the same inventory
application helper as completed chests. Equipment, bottles and run unlocks stay
in sync; rupees are not charged and regular chest prices do not advance. Reset,
replay and mode changes discard transient offers; the talent grants a new choice
on the next started run. Records v4 append the rank (108 bytes); v1/v2/v3 saves
preserve XP, scores and talents, with Starting Item initially unspent.

Regression tests cover all nine reward kinds, paused timing, stable reopening,
shared choice budgets, all-banned continuation, duplicate callbacks, restart,
native inventory effects, talent spending/refunds and legacy save migration.
In-game controller/touch selection and run-start presentation remain to be checked.

## Concurrent-combat balance and 45-degree bow fan

The current formulas and full map HP table are in [the player guide](../../SURVIVORS.md#enemy-health-curves).
This revision applies the offline balance proposal: flatter Haste/item cooldowns,
+2.5% Dual Wield speed per rank, reduced AoE growth, separate Luck rarity/crit/drop
multipliers, first-kill-only guard chests, and fixed map/time HP curves with
absolute final-boss budgets. Power, Master Sword damage, Dark Link's permanent
2× at rank 30, shield block and rupee drop rates are retained. The optional rare
Legendary global multiplier is not implemented.

Bow volleys use a symmetric center-out fan with 11.25 degrees between arrows:
one arrow at ranks 1–9, three at ranks 10–19 (−11.25°/0°/+11.25°), and five
at ranks 20–30 (−22.5°…+22.5°). Full salvos
are emitted regardless of occupied projectile slots. Volley groups and each
enemy's tracking vector grow as needed; an occupied group cannot be reused. Each enemy tracks the last
volley ID in each slot. The first arrow of a volley deals full damage to that
enemy, later arrows 25% (rounded to at least one HP before Crit/Dark Link).
Other targets and later/overlapping volleys retain independent first hits.
No damage iFrame or shared time gate is introduced. Per-projectile deduplication
still prevents a persistent arrow collider from hitting twice. Arrows stop at
their first enemy impact. The hit collector sets both the base hit object and
native target actor ID; projectile movement also retains the recorded impact
when the target dies or native hit metadata is cleared before the next update.

Ball lifetime includes the Reach-scaled outbound and return travel plus a short
margin. Spin particle scale and collider radius use the same area-radius rule.
Menus/previews display the new radii, intervals, crit chance, rarity weighting
and bow fan angle. Guard reward state resets with the run but survives respawn
and outdoor relocation; every confirmed respawn kill still awards XP.

Validation: native Linux build, rules/distribution tests, production ranged/hit
collector fixtures, map/elite health and reward lifecycle, and UI regression
fixtures. Device validation should compare five-arrow 45° volleys, simultaneous
volleys at different distances, large and small targets, high-Reach ball returns,
first versus repeated guard rewards, and every map's final boss on desktop/Android.
The previous balancing simulation predates the requested wider fan; its DPS
figures are not a new measurement of this build.

Hyrule Field navigation: owned Bokoblins use the native Link-through polygon
flag for their collision correction and generic navigation probes. Solid walls
remain solid. The probe hook is restricted to their tracked Execute scope in
this arena; story actors and camera/projectile-specific probes are unchanged.

Moving auto-attacks use directional leg animation. Straight forward movement and
a 22.5-degree steering cone retain the original native calculator and gait exactly;
the directional overlay blends in smoothly toward side/back movement. Native forward, left, back
and right attention walk/run clips blend by movement relative to Link's final
facing, including diagonal travel. The gait advances with horizontal travel and
uses the normal movement HIO speeds, so Sprint increases cadence. Native foot
contact phases are aligned before blending. Only the pelvis subtree borrows a
separate matrix calculator during body evaluation; attack/upper animation packs,
frame controllers, root movement, weapon matrices and Dual Wield remain native.
The pelvis compensates attack-root rotation without turning the upper body.
Standstill, airborne actions, Spin/Jump attacks, damage, riding, events and pause
retain their native presentation. Animation storage is released with run resources;
nonstandard rigs and unavailable resources fall back to native animation.
Desktop/Android visual validation of diagonal blends and attack transitions is
still required.


### Persistent pickups and ordinary magnet drops

Run-owned hearts and all seven rupee colours use the native no-timer flag and a
nonzero wait timer to prevent expiry and flashing. Collection, healing, currency,
pause and teardown remain native/scoped; heart-piece Get Item events are preserved.
Each ordinary reward opportunity rolls an independent 2% bonus magnet chance,
scaled by the existing Luck drop multiplier to 3%. Horde difficulty normalizes
reward opportunities as before. Four slots remain reserved for timed bosses;
additional magnet slots grow as needed and reuse collected ordinary drops.
Collecting either type summons the current XP, heart and rupee snapshot globally,
including pending owned item creates. Later drops and retiring items are excluded.


### Expanded combat perks, Legendary Chance and regeneration

Crit Chance and Bonus Damage have ten ranks (+10 base percentage points and +10
weapon damage at maximum). Append Legendary Chance and Life Regen without moving
existing talent IDs. Records v6 uses 132 bytes and migrates v1–v5, preserving
scores, XP and old ranks; appended perks start unspent.

Legendary Chance transfers 0.6 base percentage points per rank from Common to
Legendary, up to +3 points (1% to 4%), before the existing rarity Luck weights.
The shared rarity roll covers chest items, Starting Item choices and level-up
power-ups. It does not change the frequency of enemy heart/rupee/magnet drops.
Use 64-bit intermediate arithmetic; total weights fit the 32-bit rejection sampler
at every supported perk/Luck combination.

After each active player tick, regenerate one native life unit (one quarter heart)
at intervals of (20 - Life Regen rank) seconds. Rank zero already regenerates;
rank five shortens the interval to 15 seconds. Run pause/menu/death guards and the
native maximum-life gauge apply. Full health does not accumulate healing, restart
resets the interval, and ticks continue through the final boss beyond 20 minutes.


### Single-rank Legendary weapon powers

Append Hurricane, Fire Bomb, Bomb Arrow, Fire Edge, Dark Aura and Shockwave to
run upgrades. Each is Legendary-only, grants one rank and requires its family
(Fire Edge uses starting melee). Generic UI cards and collected-power previews
show their effects and cap; talent save IDs are unchanged.

Hurricane snapshots doubled boomerang collision/model/wind size and ignores
incidental contacts when choosing when to hop. Existing per-projectile victim
tracking damages every crossed enemy once. Bomb Arrow loads the native Alink
0x1d model into a private per-projectile heap, preserves in-flight piercing,
and creates one separate, deduplicated bomb explosion on first contact. Each
Multishot arrow can explode; the burst is half bow power with rank-5 bomb area
and Reach scaling. Models/cached matrices are released before archive teardown.

Fire Bomb, Fire Edge and Dark Aura use separate run-local burn clocks. Rehits
refresh their five-second lifetime without resetting pulse or stacking damage.
Fixed-point tenths preserve low-damage ratios. Fire Edge covers native sword
and jump colliders, parallel Spin/Shield and secondary Dual Wield hits; blade
particles continuously follow three points on the current sword model, including
its sheathed pose and equipment changes. They use visual blade length, independent
of Reach collision extension. Fire Arrow status
remains independent. Dark Aura checks form, range and line of sight, uses stored
maximum run HP and preserves all five trailing pulses after exiting. Fixed burn
damage bypasses later Dark/crit scaling in both native and extended damage paths.
Black flame rendering wraps native emitter callbacks and restores particle blend
state after drawing, without editing shared resources. Death, slot reuse,
restart and unload clear the clocks, blade emitters and callback bindings.

Shockwave allocates a stable independent projectile on each shield pulse, with
current melee power and rank-5 Ball radius scaled by Reach. Normal spin particles
match its collider size. It travels straight for 30 ticks at 45 units/tick, stops
at walls and damages each enemy once. It does not occupy an item or body action.

Regression coverage includes rarity floors/dependencies/caps, five exact burn
pulses and refresh timing, aura form/range/LOS/max-HP isolation, melee families,
private model allocation/reuse/release, eight separate bomb arrows, Hurricane
flight/wind sizing, shockwave geometry and multi-target deduplication, and scoped
black-particle rendering. Device verification: retest dense Multishot + Piercing
explosions, black flame appearance with other visual mods, blade placement,
Hurricane wind and Shockwave visuals, boss burns, restart and map transitions.


### Development debug switch (must be off before merge)

`src/survivors_debug.hpp` owns `DAWNLIGHT_SURVIVORS_DEBUG` (currently 1).
**Set it to 0 before merging/releasing PR #140.** No separate switches or save
migration are required. `DebugOptions::clamp` resets all debug fields when off;
effective getters also neutralize stale values, including an existing run.
Menu/reset/start boundaries sanitize stored options. Debug is the rightmost,
Ready-only tab, and values are copied into a new run, never into records/config.

The shared rarity sampler applies the chosen floor after its normal roll.
Outgoing damage scales once in the final owned damage hook, after cache reuse
checks and ordinary/Dark/fixed damage composition; it saturates at 32767 to
avoid signed native HP overflow. Infinite rerolls are zero-cost and retain
charges/counters; infinite banishes retain charges but keep exclusion flags.
Both chest and upgrade action buttons use effective availability and display
infinity. Reserved chest currency is still required.

Per-run duration and quarter-duration boss intervals drive encounter scheduling,
spawn/horde cutoff, victory gating, time-score cap, result time and HUD target.
Static 20-minute capacity bounds remain sufficient. HP/XP/cooldown progression
is not accelerated. Regression tests exercise enabled and disabled builds,
stale-value reset, UI ordering/callbacks, all five controls, rarity floor,
resource consumption, damage application and real 5/10/20-minute boss scheduling.


### Final boss victory and native defeat presentation

Defeating the final scheduled boss settles victory immediately. That boss still
awards its normal kill, XP, chest and magnet. A native Fyrus fire wave expands
from its death position and removes every surviving enemy, including guards and
pending spawns. The sweep uses no attack collider or HP damage and awards no
kills, score, XP, drops or damage-stat entries. Existing loot remains available.

The HUD shows **Victory** in the boss-name Ruby font with a red ten-second
countdown. Link remains controllable while combat and run time stop. The timer
advances only during playable simulation ticks; pause, upgrade selection and
native chest/Get Item events hold it. No new enemies, attacks or chests spawn.
Results and records are finalized once the grace period and any in-progress
reward selection finish, preserving rewards collected from the final boss.

On lethal damage, Link retains his normal death procedure and camera. The native
Game Over screen runs through its animation/fades, remains visible for two
seconds including its fade-in, and is deleted before its save/retry menu opens.
After its shared UI heap is released, Survivors opens the defeat results. A
voluntary Midna End Run still opens results immediately.

`tests/survivors_ending_test.py` exercises production callbacks alongside the
SDK's native Game Over transition methods: final-boss reward ordering, unscored
cleanup, live pickups, frozen combat time, countdown pauses, expiry during a
level-up, deferred UI deletion and exactly-once records. The HUD test also checks
Ruby-font victory text, red countdown rounding and restored GX vertex state.
