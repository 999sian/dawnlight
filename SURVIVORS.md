# Hyrule Survivors

[← Dawnlight](README.md)

A 20-minute survival mode included in `dawnlight_mod.dusk`. Select **Hyrule
Survivors** in Dusklight's game-mode menu, create/load its separate save, pick a
map and press **Start run**. Story and Boss Rush saves remain separate.
The Dusklight menu shortcut (keyboard, controller or three-finger tap) closes
this setup menu and opens Dusklight. Closing Dusklight returns to setup;
your map, difficulty, settings and allocated talents remain unchanged.

## Playing a run

- Move and dodge while Link attacks automatically. Cliff jumps follow movement,
  including sideways/backward combat movement. Automatic attacks and their cooldowns
  pause during jumps, Gale charging and gliding; released projectiles finish normally.
  Collect blue XP crystals and choose one of three power-ups at each level-up. Selection supports controller,
  keyboard and touch; menus and Get Item animations pause the run.
- Start with three hearts and normal sword attacks. Unlock Sword Wave and
  Whirling Blade through level-ups. Passive upgrades, including Heart Container,
  remain available independently of other choices (maximum 20 hearts).
- The HUD shows time, run level, score, XP and the next chest price. Inspect equipment
  in the item wheel and power-ups via the Hidden Skills scroll in Collection.
  Only bottles can be assigned to item buttons. Magnet also attracts rupees,
  hearts and heart pieces; nearby pots trigger normal automatic sword swings.
- Call Midna for **Cancel** or **End Run**. Survive until 20:00 to win;
  **New run** resets upgrades and returns to map selection. Map exits are sealed.

## Run setup

Map selection also has **Difficulty**, **Talents** and **Settings** tabs. Choices carry over
between maps/replays in the current session and are fixed when the run starts.

| Setting | Range | Default |
| --- | --- | --- |
| Enemy HP | ×1–×10, including elites | ×1 |
| Enemy Damage | ×1–×10 | ×1 |
| Enemy Speed | ×1–×3 normal locomotion; attacks keep native movement | ×1 |
| Horde | ×1–×5 enemy population and spawn amount | ×1 |
| Camera distance | 100–500%, walls may limit distance | 200% |

At Horde ×N, every Nth normal kill gives XP and rolls a drop; pots and elite
rewards are unchanged. Camera 100% is native distance; event cameras stay native.

## Score and permanent talents

Each map has its own high score, shown in the right live preview and the result screen. Final score:

| Source | Points before difficulty bonus |
| --- | ---: |
| Survival | 2 per second, up to 20 minutes |
| Normal kills | 10 each, divided by Horde |
| Timed elite / additional elite | 500 / 250 |
| Run levels gained | 50 each after level 1 |
| Complete the run | 3,000 |

Difficulty multiplier: `1 + 0.10 × (HP−1) + 0.15 × (Damage−1) +
0.25 × (Speed−1) + 0.10 × (Horde−1)` (×1–×4.15). Camera settings do not
change score. Final totals round down; normal kill points are normalized before
applying difficulty. Example: a win at default difficulty with 400 normal kills,
three timed elites and run level 20 earns **11,850 points**.

On victory, defeat or **Midna → End Run**, the final score also grants permanent
XP, shared across maps/runs in this Survivors save. Meta level 1 starts at 0 XP;
the next level costs 2,000 XP, then 2,500, 3,000… Each grants **one Unlock Point**.
Abruptly quitting an unfinished run does not award its score. Old saves retain
previous statistics and start this new progression at level 1.

Open **Talents** between Difficulty and Settings. Each rank costs one point;
use **+ / −** to spend or refund points in the scrollable list. Click a perk name
for its description. Removing a prerequisite also refunds its dependent perks;
**Reset all perks** refunds every rank without changing XP, level or high scores.
Changes apply next run; run power-up ranks remain separate. **Save progress**
writes the native save in the background and keeps the menu open; wait for
**Progress saved** before quitting. The results screen also offers **Save records**.

| Talent | Ranks | Effect / prerequisite |
| --- | ---: | --- |
| Max Health | 5 | +1 starting heart/rank, up to 8; overall health cap stays 20. |
| Crit Chance | 5 | +1 percentage point base chance/rank before Luck. Combined final chance caps at 100%. |
| Sprint | 1 | Unlock 125% running speed, including automatic combat. |
| Sprint Speed | 2 | Requires Sprint; 137.5%, then 150%. |
| Magnet | 5 | +10% pickup range/rank after run upgrades; includes all magnet pickups. |
| Manual Jump | 1 | Unlock the configured Jump input/touch button. |
| Glider | 1 | Requires Manual Jump; press Jump while airborne. |
| Revali's Gale | 1 | Requires Glider; one charge, 120-second recovery. |
| Gale Charges | 2 | Requires Gale; +1 charge/rank, up to three. Refilled each run. |
| Rerolls | 2 | +1 free reroll/rank/run, up to three; shared by both choice menus. |
| Banishes | 2 | +1 banishment/rank/run, up to three; shared by both choice menus. |

## Maps, elites and pots

| Map | Additional dormant elites | Breakable pots |
| --- | --- | ---: |
| Darknut Arena | None | 6–10 |
| Hyrule Field | Four at random positions | 40–72 |
| Gerudo Desert | Four at random positions | 40–72 |
| Palace of Twilight | One per building doorway | 12–24 |
| Hyrule Castle Throne Room | None | 8–12 |

Random enemies require dry ground and a walkable approach to Link, avoiding
water, isolated ledges and gaps.

Every map also spawns an elite at **5:00, 10:00 and 15:00**. Elites are twice
normal size and marked with the native Tear-of-Light icon on the minimap. Dormant
elites wake only after taking damage; after a kill they respawn in three minutes,
with new positions in Field/Desert. Timed elites do not respawn.

Every elite drops a **free large chest**. The three timed elites also drop an
**Epona flute** at five times normal size nearby: walk over it to pull all currently lying XP crystals
to Link from anywhere on the map. It is a one-time pickup, not an inventory item. Pots are scattered according to floor
area and safe placement, drop rupees of different values or rare heart pieces,
and do not refill during a run. Ordinary enemies can drop healing hearts and
rupees; level-ups do not heal.

## Chests and equipment

Paid chests cost **20, 25, 32, 41, 52… rupees**: increases of +5, +7, +9, +11…
Their current price floats above each small chest.
They are interactable only when affordable. Opening takes priority over automatic
attacks and offers three items; choose one to receive it through the native Get
Item animation. Cancelling preserves offers. Opened chests disappear; free elite
chests never raise the paid-chest price.

Equip up to **four chest items**; their icons (or empty slots) appear above the
choices. Red Potion uses one shared slot for all bottles, retained even when
they are empty. Passive power-ups have no
slot limit. **Dual Wield and Shield replace each other** in the same slot, with
an explicit warning below the item name. A replacement starts at **rank 0**;
the discarded ranks are lost, including when switching back. Replacement-card
rank units become rupees; subsequent cards upgrade the new item normally.

Each run shares **one free Reroll and one Banish** across both selection menus.
Reroll refreshes all offers; further uses cost **50, 100, 150… rupees**. Paid
chests reserve their purchase price. Banish removes the named choice from future
offers without removing its current ranks or equipment; weapon bans apply to
both menus. The top-left **Reroll** button shows **Free** or its rupee cost; the
icons beside your equipment show remaining free rerolls and banishes. Use the red
banish symbol at the top right of a card to remove that choice. Talents can add two more free uses of each. All counters and bans reset on a new run.

| Chest reward | Main effect |
| --- | --- |
| Hero's Bow | Automatic aimed arrows; one extra arrow every three ranks (4, 7, 10…), max 10 arrows. Unlocks Fire Arrows level-ups: burning damage over time, refreshed on hit. |
| Boomerang | Gale throws that hop between enemies; one extra hop every three ranks (4, 7, 10…), max 10 hops / 11 targets. |
| Bombs | Automatic throws and area explosions; upgrades improve damage, radius and frequency. |
| Ball and Chain | Pierces outward and back, damaging and staggering enemies on both legs. |
| Master Sword | More sword damage; rank 5 becomes the Light Sword. Unlocks Jump Strike level-ups. |
| Ordon Sword | Dual Wield: two nearby melee targets at rank 1, up to five total targets at rank 9; capped at rank 10. |
| Shield | 5% block chance per rank, max 50%; evolves to Ordon at 20% and Hylian at 35%. Unlocks Shield Attack level-ups. |
| Dark Link | Double damage for 5 seconds every 20 seconds; upgrades alternate shorter intervals and longer duration, becoming permanent at rank 11. |
| Red Potion | One equipment slot; adds filled bottles up to four, then refills empty ones. Rank 0 heals 1 heart. Potion Healing level-ups add ¼ heart per rank, up to 8½ hearts at rank 30. Use manually. |

**Jump Strike:** 20% chance at an eligible normal melee attack, with a 6-second
cooldown reduced by Haste. Uses the native quick jump attack and the original
bomb shockwave on landing. Landing splash deals 30% of melee damage at rank 1,
+5 percentage points per rank (175% at rank 30), rounded to native whole damage.
Direct sword contact deals normal melee damage; critical hits and Dark Link apply.
**Reach** scales both Jump Strike and Spin collision radii by +15% per rank.
Other automatic attacks pause throughout Jump Strike, as they do during traversal.

Weapons, passives, Fire Arrows, Shield Attack and Jump Strike reach rank 30.
Luck caps at 20, Master Sword at 5, Dual Wield and shield block at 10, and Dark Link at 11.
Haste retains its first five ranks, then improves with diminishing returns;
weapon cooldowns never fall below six ticks (0.2 seconds). Unlocked chest weapons can also be upgraded
at level-up. Ordinary hits do not stagger enemies; Shield Attack and Ball and
Chain do. Knocked-down enemies remain damageable while falling, lying down
and getting back up.

## Critical hits

Two passive level-up rewards, both capped at **rank 30**:

| Power-up | Start | Per rank | At rank 30 |
| --- | --- | --- | --- |
| Crit Chance | 0% | +1 percentage point base chance | 30% base; 90% at +200% Luck |
| Crit Damage | ×2 damage | +0.05 multiplier | ×3.5 damage |

Talents add up to five base percentage points; combined final chance caps at 100%.
Final chance is `base chance × (1 + Luck / 100)`, inspired by
[Vampire Survivors](https://vampire-survivors.fandom.com/wiki/Flash_Arrow).
Each direct sword, spin, projectile, shield or Dual Wield hit rolls independently.
Burn ticks cannot crit. Critical damage rounds to native integer health before
Dark Link doubles it. Both power-ups support rarities, rerolls and banishment;
the menu shows the effective chance including current Luck.

## Rarities and Luck

Each offered card rolls independently, including **free elite chests**. Rarity
controls ranks/units of the chosen reward: **Common ×1, Uncommon ×2, Rare ×3,
Epic ×4, Legendary ×5**. Names and icon borders show the rarity. Units exceeding
a reward's cap become 20 rupees each, subject to wallet capacity.

Luck grants **+10% per rank**, up to **20 ranks / +200%**, and resets each run.
Higher-rarity Luck cards grant multiple ranks too. With `k = 1 + Luck / 100`,
rarity weights are **60 / 25k / 10k² / 4k³ / k⁴**. Divide each weight by their
sum to get its chance. There is no player-level or chest-source bonus.

| Luck | Common | Uncommon | Rare | Epic | Legendary |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0% | 60.00% | 25.00% | 10.00% | 4.00% | 1.00% |
| +10% | 56.40% | 25.85% | 11.37% | 5.00% | 1.38% |
| +30% | 49.57% | 26.85% | 13.96% | 7.26% | 2.36% |
| +50% | 43.30% | 27.06% | 16.24% | 9.74% | 3.65% |
| +100% | 30.30% | 25.25% | 20.20% | 16.16% | 8.08% |
| +150% | 20.94% | 21.81% | 21.81% | 21.81% | 13.63% |
| +200% | 14.49% | 18.12% | 21.74% | 26.09% | 19.57% |

Rounded chances **per card**, not per three-card selection. At +200% Luck,
a selection has about **47.96%** chance to contain at least one Legendary card.
Luck never rerolls existing offers.

Heart, heart-piece and rupee bonuses use a separate multiplier capped at **+50%
Luck**: enemy drops reach 7.5% heart / 67.5% rupee / 25% nothing; pot heart pieces
reach 3%. Better rupee-color chances also stop improving above +50%.

[Technical details and testing notes](docs/features/hyrule-survivors.md)
