# Per-actor hook performance audit

Source baseline: PR #145, commit `d0592ae2af013e5a2fc2c018532096118acc0614`. The follow-up branch changes the Debug compilation switch only; this audit does not change combat or hook behavior. SDK dispatch semantics below were checked against the repository's pinned Dusklight v2.0.0 sources. Confirm the exact host version and platform when comparing a profiler capture.

## Assessment

The report identifies a credible optimization opportunity. There is unnecessary work in callbacks that run for unrelated actors, including when Survivors is inactive. However, the reported approximately 280 calls per frame and their timing have not been reproduced here. Inclusive time in `fpcMtd_Method` also includes the native actor method that it invokes; it is not all Dawnlight overhead. Per-callback self time, inclusive time and call counts would make the priority order much more reliable.

Disabling the Survivors Debug tab only hides/neutralizes debug controls. It does not unregister the Survivors process hooks or disable the standalone-spawner hook used by gameplay actors.

## Registered callbacks

The current source has nine hook aliases that can target `fpcMtd_Method`, but five do so only on Apple. Aliases, pre callbacks and post callbacks are different counts.

| Feature / alias | Pre | Post | Target and lifetime |
| --- | ---: | ---: | --- |
| Survivors / `SurvivorsProcess` | 6 | 4 | `fpcMtd_Method`, registered during mod initialization |
| Enemy slow motion / `ProcessMethodHook` | 1 | 1 | `fpcMtd_Method`, registered during mod initialization |
| Cave randomizer / `StalhoundProcessHook` | 1 | 1 | `fpcMtd_Method`, registered during mod initialization |
| Standalone spawner / `SpawnerProcessHook` | 1 | 1 | `fpcMtd_Method`, installed on first standalone spawn; used by gameplay as well as testing |
| Bullet Time / `ProcessMethodHook` | 1 | 0 | Apple: `fpcMtd_Method`; otherwise `fpcMtd_Execute` |
| Fierce Deity / `FiercePlayerExecuteHook` | 1 | 1 | Apple: `fpcMtd_Method`; otherwise `fpcMtd_Execute` |
| Fierce Deity visuals / `FiercePlayerDrawHook` | 1 | 1 | Apple: `fpcMtd_Method`; otherwise `fpcLf_DrawMethod` |
| Glide carrier / `GlideCarrierExecute` | 0 | 1 | Apple: `fpcMtd_Method`; otherwise `fpcMtd_Execute` |
| Glider reward / `GliderRewardExecute` | 1 | 0 | Apple: `fpcMtd_Method`; otherwise `fpcMtd_Execute` |

On non-Apple platforms this yields 8 pre + 6 post registrations at startup, or **9 pre + 7 post** after the standalone-spawner hook is installed. On Apple it yields 12 pre + 9 post, or 13 pre + 10 post after that installation. These are source registration counts, not measured invocations: a pre callback returning `HOOK_SKIP_ORIGINAL` stops later pre callbacks, while post callbacks still run.

The six Survivors pre callbacks are elite scale, Stalhound daytime, boss scope, contact-owner deletion, general process handling and enemy-attack observation. Four corresponding post callbacks restore scopes or finish enemy processing. Registration is unconditional in `initialize_survivors`, not tied to Debug or an active run.

## Highest-priority findings

### 1. Survivors scans all tracking containers before rejecting unrelated actors

[`before_process`](../../src/survivors.cpp) validates that the process is an actor, then calls `tracked`, `tracked_pot`, `tracked_pickup` and `tracked_enemy_arrow` before testing whether that actor or the current arena is relevant. There is no inactive-mode fast path ahead of these lookups.

The fixed capacities are 372 enemies, 72 pots and 1,488 enemy arrows/children: **1,932 slot visits for one complete miss**, plus the current pickup-vector length. Empty fixed arrays still get scanned. If all 280 reported dispatches were unrelated actor misses, this one callback alone would traverse 540,960 fixed slots per frame. That is an illustrative source-level count, not a measured CPU cost; non-actor dispatches return earlier and matches can terminate scans.

First optimization candidate: maintain an explicit cheap condition for no active Survivors work and no pending cleanup. Do not use only `!in_arena()` or `!s_active`: retirement, asynchronous creation, deletion, contact detachment and Get Item cleanup can outlive the active arena. Preserve those paths until drained. Within active runs, resolve actor ownership once and reuse it, or use an ID-to-slot index with correct creation/deletion/reuse handling. Check the native method before Execute-only work.

Other Survivors scopes repeat actor validation and ownership lookup. `before_roster_time` can filter Stalhound profiles before looking up a run enemy. `before_enemy_attack_process` evaluates `playable()` before establishing tracked ownership on Execute; that can query the UI service for unrelated actors during runs. Move cheap ownership/method tests ahead of expensive state queries where lifecycle semantics allow it.

### 2. Slow-motion profile lookup repeats linear searches

[`enemy_slow_motion.cpp`](../../src/enemy_slow_motion.cpp) searches a 64-entry profile array for each actor accepted by its global pre callback, before checking the actual native method. For profiles using `processExecute`, the Execute path searches again in `before_enemy_slow_execute`, then again in `enemy_uses_continuous_slow`.

Potential improvements: method filtering before profile lookup, a direct or indexed profile lookup, and passing the resolved profile into the shared Execute helper. The helper also initializes/copies an `EnemySlowStep` containing large arrays and clears clock entries on the inactive path. Measure that work separately before redesigning it.

Do not gate this solely on Bullet Time being off: the same path supports Hard Mode, clock cleanup, nested execution scopes and profile observers. Darknut's observer, for example, advances its attack cooldown even outside slow motion.

### 3. Spawner and Cave hooks have cheap opportunities

[`enemy_spawner.cpp`](../../src/enemy_spawner.cpp) pushes a process-stack entry and checks actor ownership on every dispatch once installed, even when no tracked actors remain. An empty-owned-set plus empty-scope fast path can avoid this work. Preserve the base-process creation path: actors may need appearance-switch handling before `fopAcM_IsActor` is valid. A nested unrelated process must still mask an outer owned process's scope.

[`cave_randomizer.cpp`](../../src/cave_randomizer.cpp) validates actors and checks the stage even when there are no owned Stalhounds. A fast path based on both an empty Stalhound set and an empty time-scope stack is a candidate. The stack condition is essential to restore normal daytime inside nested unrelated calls.

### 4. Several hooks already filter narrowly

Fierce Deity Execute checks Link's pointer and innermost method table. Glide carrier and reward hooks first reject inactive state. Contact-owner deletion returns immediately when its contact map is empty. These are not automatically the largest costs just because their registration is global. While active, carrier/reward helpers still perform actor-ID resolution before comparing the incoming process, which may be worth measuring.

Apple needs separate measurements: several broader dispatch targets are deliberate fallbacks for inlined host functions. Replacing them blindly with `fpcMtd_Execute` or a private actor symbol can silently miss calls on those builds.

## Why per-feature uninstall is not a local change

The SDK's `HookService::uninstall` unregisters **all callbacks for the calling mod at the target address**. In the pinned host implementation, `hook_uninstall` invokes `erase_callbacks(slot, context)`, even though the original-function slot identifies a particular trampoline candidate. Therefore uninstalling `SurvivorsProcess`, `SpawnerProcessHook` or `StalhoundProcessHook` independently can also remove the other Dawnlight callbacks on `fpcMtd_Method`.

A safe structural approach is one owner of the shared target, with one pre/post dispatcher and feature-specific gates, or a host API supporting individual callback removal. Consolidation must preserve callback order, `HOOK_SKIP_ORIGINAL`, post callbacks whose pre callback was never reached, nested actor/create requests and deletion safety. Do not mutate callback registration while the host is iterating the callback vectors.

The safe first pass is to reduce work inside existing callbacks. Consolidating dispatch or changing installation lifetime should follow profiling and lifecycle tests.

## Profiling and validation plan

Request the profiler's exact Dawnlight commit, Dusklight version, platform/device, scene/mode, actor count and active features, plus per-callback calls/frame, self time and inclusive time. Separate `fpcMtd_Method` native-original time from dispatch/callback time and distinguish simulation frames from render/interpolation frames. Use the same scene and capture duration for before/after comparisons; report frame-time median and p95/p99 as well as average callback time. Avoid per-actor logging in the measured build.

Capture at least: ordinary story play with optional features off; Survivors hub and dense combat; Bullet Time/Hard Mode separately and together; Cave randomizer; and return to story play after a run/spawn so pending cleanup and retained hooks are covered.

Implementation regressions should cover non-actor create requests, unrelated actors, nested owned/unowned execution, pre-skip/post pairing, deletion before free, ID/address reuse, pending creation/retirement, stage and mode changes, and Apple dispatch fallback. Existing starting points are `process_actor_test.py`, Survivors contact/lifecycle/hook tests, spawner/Cave tests, and enemy slow-motion/Hard Mode tests. Validate in-game on the reported weak device before claiming a frame-time improvement.

## Follow-up scope

The follow-up PR re-enables the development Debug tab with its session toggle still defaulting Off and records this audit. No performance speedup is claimed and no hook optimization is implemented by this audit. Before merging that development PR, disable Debug again and verify the disabled build.
