# Survivors animation audit — PR #140

Compared the current scheduling, collision and presentation code with local
history commit `beaa6ab452f43cc96281d4009919d567128e556b`
(`Run Survivors abilities concurrently and preserve rapid independent hits`)
and its parent. Findings below are source-level conclusions, not an in-game
visual certification.

| Ability / interaction | Confirmed finding | State after this patch |
| --- | --- | --- |
| Spin Attack / Great Spin | The concurrency patch removed `procCutTurnInit` and replaced the native action with a six-frame collider/particle pulse. Link never performed the spin. | Fixed: native action, effect expansion and hit window restored; no extra pulse or duplicate damage. |
| Normal sword versus item poses | `update_sword` could start an ordinary body action while a cosmetic upper-body item pose was active; `update_ranged` then cancelled the pose because the body was no longer idle. Sustained sword use could suppress presentation entirely. | Fixed: ordinary cuts/facing yield; an item pose may overlay an already-running ordinary cut and is retained until completion or a higher-priority interruption. Sword Wave wind-up is treated as an ability. |
| Shield Attack | The patch deleted `update_shield_attack` and its `procGuardAttackInit` call. The independent frontal hit still damages/staggers, but has no native shield-bash action. | Open: native shield-bash presentation is absent. Shockwave still launches independently from its shield cadence. |
| Hero's Bow | The draw/reload phase `ARELORD` was removed. Arrows launch before optional `ASHOOT`; there is no draw-to-release sequence. | Open: shoot-only cosmetic pose, possibly skipped when another ability owns the body. |
| Boomerang and Bombs | Native frame-triggered releases (5.5 and 4 respectively) were removed; projectiles launch before the throw pose. `released` starts true, so `draw_ranged` omits their held models. | Open: throw/release timing and pre-release hand presentation are missing. |
| Ball and Chain | The throw-frame release and projectile-linked `IBPULL`/`IBCATCH` stages were removed. The `pulling`/`catching` fields remain but are never set true. | Open: only an optional throw pose remains; the chain projectile still flies and returns independently. |
| Simultaneous ranged weapons | Only one upper-body pose exists. `fire_items` tries Bow, Boomerang, Bombs, Ball in fixed order. `s_lastRanged` is written but no longer selects the next pose, so simultaneous ready weapons favor the earlier item. A shot that misses the pose slot is not queued for later presentation. | Open: visual starvation is possible; projectile firing itself is independent and is not starved by the pose slot. |
| Sword Wave | The concurrency patch initially detached the wave from a sword swing. Later commit `9b4b744` restored the casting swing and release frame. | Already restored; this patch additionally protects the wind-up from Spin/item pose replacement. |
| Jump Strike | The concurrency patch retained `procCutLargeJumpInit`, native launch/landing actions and their animation rate, adding only per-activation hit tracking. Later steering changes retain those actions. | No corresponding animation removal found. Native jump/landing actions remain protected while independent shots can continue without their own pose. |

The shield/ranged presentation findings are recorded for a follow-up; this patch
does not reserialize projectile gameplay to restore those stages. A future pose
scheduler needs to arbitrate the single body/upper-body animation channels while
keeping shot cooldowns, manual reticle aim, hit attribution and already-released
projectiles independent.

The Stallord/Death Sword visibility issue is separate from this player-animation
patch: their native effects used dungeon-bank particle IDs unavailable in other
arenas. Those resources are now imported and mapped in owned actor scope.

## Verification and remaining runtime checks

Production-code regression fixtures cover native Spin entry, ordinary-cut/item
priority, protected wave wind-up, simultaneous projectile release and per-swing
hit deduplication. Grounding tests distinguish the former 240-unit placement
limit from airborne deaths at 300, 1,500 and 12,000 units, and preserve boss spawn
altitude separately. Phantom Zant tests cover direct and queued heavy hits,
existing stagger and lethal damage. Particle tests cover all imported effects,
texture remapping, actor scoping and unload; they use archive fixtures rather
than copyrighted game assets.

An in-game pass is still needed for Spin/Great Spin blending, the exact Fire Edge
flame appearance, Stallord/Death Sword trails outside their dungeons, and the feel
of tenfold knockback. Compilation and isolated regressions cannot verify those
rendered/gameplay details.
