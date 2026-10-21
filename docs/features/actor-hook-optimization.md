# Actor hook lookup optimization

This pass starts from the PR #145 merge (`19fe132`), independently of PR #146.
The Survivors Debug tab remains disabled.

## Motivation

A supplied Switch profile of Dawnlight 5.0.0 reports 520–540 calls/frame to
`fpcMtd_Method`, approximately 6.4 ms/frame in its callbacks and 10.6–12 ms/frame
in Dawnlight during dense Survivors waves. The ten reported individual timings
sum to 6.70 ms and include callees; they must not be treated as disjoint exclusive
costs without clarifying capture windows and nesting. These measurements motivate
the patch, but do not establish which source function owns each offset or predict
this patch's savings.

## Changes

- Index Survivors enemies, pots, pickups and enemy projectiles/children by actor
  ID. Lookups no longer scan 372 enemy, 72 pot and 1,488 child slots plus the
  pickup vector. The hash index provides expected constant-time lookup and is
  shared by all callers of the existing tracking helpers, including active runs.
- Store slot numbers rather than actor pointers or vector-element pointers.
  Register IDs immediately after accepting a spawn, retain pending and retiring
  IDs, remove entries on native deletion or confirmed creation failure, and
  rebuild the pickup index only when compaction actually removes entries.
  Validate the slot's current ID before returning a record.
- Preserve treasure retention and arena-mesh discovery for unrelated actors, as
  well as cleanup after leaving Survivors. There is no blanket inactive-mode
  return that could bypass those responsibilities.
- Replace the 64-entry slow-motion profile scan with a dense native-profile table.
  Filter Execute before looking up the profile, pass the resolved profile into
  the shared Execute helper, and reuse the slow-motion scale. Dedicated profile
  hooks, Hard Mode, observers, clock cleanup and scope restoration still run.
- Reject unrelated profiles before the Survivors Stalhound lookup, and defer
  `playable()` until enemy-attack observation has established live ownership.
- Skip empty spawner/Cave process work only when the corresponding scope stack
  is also empty. A nested unrelated call still masks an outer owned scope even
  when its final owned ID has just been removed.
- Retain successfully loaded native object archives used by run-owned standalone
  actors across respawns on all five Survivors maps. This includes secondary boss
  archives and child actors, using existing spawner ownership rather than another
  roster or a map-specific archive whitelist. A resource-processing post callback
  on `dRes_info_c::setRes()` acquires one extra reference only after processing
  succeeds. Both native load overloads reach this function. It neither starts
  asynchronous loads nor changes the actor's phase state; stage-table resources
  are excluded by checking object-table record identity.
- Keep at most 32 encountered archives per run, with normal native behavior when
  full. Never preload other maps or retain archives in custom heaps. Release only
  our references on run end, restart, map/stage exit, mode reset and shutdown;
  living native actors retain their own references. Endless starts a fresh cache.

The supplied main-build log contains 20,952 resource-processing messages,
including repeated enemy models. This supports reducing repeated archive
processing, but the log has no frame timestamps and cannot establish the cause
of reported multi-second freezes. Retention avoids repeated processing while an
archive is held; the first load can still stall. The 32-entry limit bounds archive
count, not resident bytes, so peak memory must also be checked on Switch.

The first cache implementation hooked both `dComIfG_resLoad` overloads. The
supplied Windows v2.0.3 log (`dusklight-20261021-110156.log`) reports funchook
relocation failure followed by `failed to install Hyrule Survivors post-hook
SurvivorsArchiveLoad (1)`, aborting Dawnlight activation. Those hook targets have
been removed. The replacement resource-processing hook is optional: if a host
cannot install it, a single initialization warning is logged and normal native
resource loading remains active. All gameplay-critical hook failures still fail
initialization. The subsequent Windows log (`dusklight-20261021-111445.log`)
reaches `Dawnlight portable feature pack initialized` without the cache fallback
warning, supporting successful installation of the replacement hook. It records
one load each of `ba.bmd` and `ms.bmd`; this short run does not prove cache reuse
across repeated respawns or a reduction in freezes.

No callback is uninstalled or reordered. The shared-target SDK uninstall behavior,
Apple fallbacks, gameplay limits, attack timing and equipment remain unchanged.
The new indices consume memory proportional to tracked records and perform
allocation/maintenance on spawn/removal; device profiling must include that cost.

## Validation

- All 76 Survivors test programs pass locally with GCC, including the new process
  index regression and existing lifecycle, pickups, projectile ownership and
  equipment tests. The new regression exercises ID/slot reuse, deletion outside
  the arena, pickup reallocation/compaction, geometry collection and record-free
  lookup misses.
- Archive retention tests cover all five maps with representative primary,
  secondary and child archives, repeated respawns without another native load,
  nested unrelated actors, incomplete loads, custom heaps, reference overflow,
  cache capacity, failed acquisition, replaced resources, same-name stage records
  and balanced teardown. Registration tests inject a failed cache hook and verify
  successful mode registration, a disabled cache and one warning; failures of
  required hooks remain fatal.
- Additional regressions cover all native-profile lookup boundaries, dedicated
  versus process hooks, nested/overflow scope balance, empty ownership during an
  outer scope, base creation admission, and unmatched post callbacks.
- Native Linux Release module and `.dusk` packaging succeed. New tests are wired
  into the Linux CI job with GCC and Clang.
- The two existing ASan/UBSan contact/audio tests were run locally with
  `ASAN_OPTIONS=detect_leaks=0`: LeakSanitizer cannot inspect process threads in
  this runtime. Address/undefined-behavior instrumentation remained enabled;
  repository/CI sanitizer settings were not changed.

Switch frame-time gains are not measured yet. Compare the same build options,
scene, enemy/pickup density and feature state before/after, including spawn-heavy
moments and teardown. Report callback totals plus frame-time median and p95/p99.
The planned story/Hyrule Field feature-on/off comparison remains useful for idle
costs. This patch does not promise stable 30 FPS in late waves.

## Follow-up optimization TODOs

PR #147 closes this optimization pass. Any later optimization work must start
from current `main` on a **new branch with a new pull request**; do not reuse
`perf/actor-hook-lookups` or append further optimization work to PR #147.
The following items are investigations, not established bugs or promised gains:

- Symbolicate the original Switch callback offsets against the exact profiled
  `linux-aarch64/mod.so` and its matching symbols (reported SHA-256 prefix
  `f77ab0846f5b56ce`). Start with `+0x114840` (3.26 ms), `+0xca140` (0.95 ms)
  and `+0xcf9a0` (0.59 ms). Obtain per-callback call counts and distinguish
  inclusive from exclusive timings before choosing the next code changes.
- Capture comparable before/after Switch runs at stock handheld clocks,
  including dense late waves and all five maps. Record actor/pickup counts,
  Dawnlight game-thread time, callback costs, frame-time median/p95/p99 and
  multi-second stalls. Keep build options, enabled mods and feature settings
  matched; separately measure the planned Hyrule Field story-mode feature-on/off
  test for Bullet Time, Flurry Rush, Hard Mode and Cave Randomizer idle costs.
- Verify archive reuse over sustained respawns, boss secondary resources and
  child actors, plus restart, Endless, map changes and teardown. Check Windows,
  Android and Switch. Installation success alone does not establish callback
  coverage or cache hits. If needed, add temporary bounded counters for acquired
  references, capacity misses and releases; avoid per-frame/per-actor log spam.
- Measure peak resident/heap memory and cache occupancy on Switch. The current
  32-entry cap is not a byte budget. Consider a byte budget or different retention
  policy only if measurements show pressure or capacity misses.
- If freezes remain, correlate timestamped stalls with first archive loads,
  model/texture processing, allocations and other mods. Consider staged warm-up
  only if first-load stalls are confirmed and memory/loading-time costs are
  acceptable; the current implementation intentionally does not preload maps.
- Reprofile remaining actor-hook work after these changes, including index
  maintenance/allocation at spawn and cleanup. Optimize measured hotspots while
  preserving pending/retiring actor tracking, nested ownership masking and
  cleanup outside an active run. Do not introduce blanket inactive returns or
  shared-target hook uninstalls that bypass other features.
