# Hyrule Survivors

[← Dawnlight](README.md)

A 20-minute survival mode included in `dawnlight_mod.dusk`. Select **Hyrule
Survivors** in Dusklight's game-mode menu, create/load its separate save, pick a
map and press **Start run**. Story and Boss Rush saves remain separate.

## Playing a run

- Move and dodge while Link attacks automatically. Collect blue XP crystals and
  choose one of three power-ups at each level-up. Selection supports controller,
  keyboard and touch; menus and Get Item animations pause the run.
- Start with three hearts and normal sword attacks. Unlock Sword Wave and
  Whirling Blade through level-ups. Power, Haste, Reach, Magnet and Heart
  Container compete for four passive slots; Luck has its own slot.
- The HUD shows time, level, kills, XP and the next chest price. Inspect equipment
  in the item wheel and power-ups via the Hidden Skills scroll in Collection.
  Only bottles can be assigned to item buttons.
- Call Midna for **Cancel** or **End Run**. Survive until 20:00 to win;
  **New run** resets upgrades and returns to map selection. Map exits are sealed.

## Maps, elites and pots

| Map | Additional dormant elites | Breakable pots |
| --- | --- | ---: |
| Darknut Arena | None | 6–10 |
| Hyrule Field | Four at random positions | 40–72 |
| Gerudo Desert | Four at random positions | 40–72 |
| Palace of Twilight | One per building doorway | 12–24 |
| Hyrule Castle Throne Room | None | 8–12 |

Every map also spawns an elite at **5:00, 10:00 and 15:00**. Elites are twice
normal size and marked with the native Tear-of-Light icon on the minimap. Dormant
elites wake only after taking damage; after a kill they respawn in three minutes,
with new positions in Field/Desert. Timed elites do not respawn.

Every elite drops a **free large chest**. Pots are scattered according to floor
area and safe placement, drop rupees of different values or rare heart pieces,
and do not refill during a run. Ordinary enemies can drop healing hearts and
rupees; level-ups do not heal.

## Chests and equipment

Paid chests cost **20, 25, 32, 41, 52… rupees**: increases of +5, +7, +9, +11…
They are interactable only when affordable. Opening takes priority over automatic
attacks and offers three items; choose one to receive it through the native Get
Item animation. Cancelling preserves offers. Opened chests disappear; free elite
chests never raise the paid-chest price.

| Chest reward | Main effect |
| --- | --- |
| Hero's Bow | Automatic aimed arrows; spread shots at ranks 3/5. Unlocks Fire Arrows level-ups: burning damage over time, refreshed on hit. |
| Boomerang | Gale throws that hop between enemies; upgrades add targets. |
| Bombs | Automatic throws and area explosions; upgrades improve damage, radius and frequency. |
| Ball and Chain | Pierces outward and back, damaging and staggering enemies on both legs. |
| Master Sword | More sword damage; rank 5 becomes the Light Sword. |
| Ordon Sword | Dual Wield: two nearby melee targets at rank 1, up to six at rank 5. |
| Shield | 5% block chance per rank, max 50%; evolves to Ordon at 20% and Hylian at 35%. Unlocks Shield Attack level-ups. |
| Dark Link | Double damage for 5 seconds every 20 seconds; upgrades alternate shorter intervals and longer duration, becoming permanent at rank 11. |
| Red Potion | Adds filled bottles up to four, then refills empty bottles. Use manually to heal. |

Weapons generally cap at five ranks. Unlocked chest weapons can also be upgraded
at level-up. Ordinary hits do not stagger enemies; Shield Attack and Ball and
Chain do.

## Rarities and Luck

Each offered card rolls independently, including **free elite chests**. Rarity
controls ranks/units of the chosen reward: **Common ×1, Green ×2, Blue ×3,
Purple ×4, Orange ×5**. Names and icon borders show the rarity. Units exceeding
a reward's cap become 20 rupees each, subject to wallet capacity.

Luck grants **+10% per rank**, up to **20 ranks / +200%**, and resets each run.
Higher-rarity Luck cards grant multiple ranks too. With `k = 1 + Luck / 100`,
rarity weights are **60 / 25k / 10k² / 4k³ / k⁴**. Divide each weight by their
sum to get its chance. There is no player-level or chest-source bonus.

| Luck | Common | Green | Blue | Purple | Orange |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0% | 60.00% | 25.00% | 10.00% | 4.00% | 1.00% |
| +10% | 56.40% | 25.85% | 11.37% | 5.00% | 1.38% |
| +30% | 49.57% | 26.85% | 13.96% | 7.26% | 2.36% |
| +50% | 43.30% | 27.06% | 16.24% | 9.74% | 3.65% |
| +100% | 30.30% | 25.25% | 20.20% | 16.16% | 8.08% |
| +150% | 20.94% | 21.81% | 21.81% | 21.81% | 13.63% |
| +200% | 14.49% | 18.12% | 21.74% | 26.09% | 19.57% |

Rounded chances **per card**, not per three-card selection. At +200% Luck,
a selection has about **47.96%** chance to contain at least one orange card.
Luck never rerolls existing offers.

Heart, heart-piece and rupee bonuses use a separate multiplier capped at **+50%
Luck**: enemy drops reach 7.5% heart / 67.5% rupee / 25% nothing; pot heart pieces
reach 3%. Better rupee-color chances also stop improving above +50%.

[Technical details and testing notes](docs/features/hyrule-survivors.md)
