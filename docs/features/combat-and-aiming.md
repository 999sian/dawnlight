# Combat and aiming

[← Dawnlight overview](../../README.md)

## Aiming

**Aim Movement** allows movement while aiming. **Aim Mode** offers Vanilla,
3rd Person and Cinema options, with touch and gyro support for the modded modes.
3rd Person keeps its shoulder view on magnetic walls and ceilings with Iron Boots;
Cinema keeps its native close aim view. Camera offsets and reticle aiming follow
Link's magnetic orientation.
The Bow and Slingshot reticles also appear when aiming from Epona in either custom mode,
including when starting to aim on horseback without first aiming on foot.
The Gale Boomerang can lock onto terrain in both custom camera modes, using its
native lock-on range and target limit. Terrain targets follow the camera reticle,
including the 3rd Person reticle offset, with Aim Movement either on or off.

See [Arrow Modes](arrow-modes.md) for Bow ammunition modes and ZR controls.

## Bullet Time

Bullet Time while aiming the Bow during a manual jump or Gale: **Off** disables
it, **Always** preserves the previous On behavior, and **BOTW** requires a ground
clearance of twice the original 100% jump height when activating. The threshold
is independent of Jump Height/Gale Height. A 100% jump from level ground cannot
trigger BOTW Bullet Time; jumping off a ledge or using Gale can. A 200% jump
reaches the threshold near its apex. Once activated, duration, stamina and
cancellation work as before. Existing On/Off configurations migrate to Always/Off.

## Flurry Rush and Great Spin

**Flurry Rush** rewards a perfectly timed evade. The optional **Great Spin
Projectile** adds a ranged attack to the Great Spin.

During Flurry Rush, each sword strike that connects shows the native impact
effect at the contact point. Damage and the enemy's hit reaction are still
deferred until the rush ends.

Bullet Time, Flurry Rush and the Great Spin projectile share the
[stamina meter](movement-and-abilities.md#shared-stamina) with movement abilities.

## Dark Link

**Dark Link** adds a charge meter, a temporary transformation and configurable sword
damage (double by default). Open **Gameplay -> Dark Link Settings** below its toggle.
Charge survives area changes, death/respawn and switching to wolf form; these
transitions end the transformation without emptying its remaining charge.
Saving the game stores the current charge for that save slot. Loading restores
the saved charge with the transformation inactive. New saves and older saves
without stored charge start empty; Boss Rush charge is kept separate.
**Dark Link Visual** selects the appearance:

| Visual | Appearance during the transformation |
| --- | --- |
| Magic Armor | The existing Magic Armor model swap. |
| Dark (default) | The currently worn outfit/model with dark shading and red eyes. |
| Dark Magic | The Magic Armor model with dark shading and red eyes. |
| White | Current outfit: white texels become black; black and colored texels become white. Amber iris/pupils and black sclera. |
| Gold | Current outfit with warm golden shading/specular highlights and fully white glowing eyes. |

While Dark Link is active, up to 88 black smoke strands rise from the animated torso,
neck, shoulders, arms and legs with any selected visual. Their roots follow
eleven current presented bone poses, including custom outfits. Each strand is
an independent curved ribbon: there is no enclosing surface or geometry between
neighbouring strands. Bends travel upward at 20 game units per second, with
independent phases, heights, widths and opacity across the body. Eight strands
per body part double the previous population; maximum width is four times the
previous limit while the thinnest wisps retain their original width. Width also
undulates along the rising curve. Roots sit inside the approximate limb volume
without a transparent base gap; scene depth hides the buried section. Soft
transparent sides and tapered tips keep the strands fine; opacity detail flows
upward with the curves.
Their narrow cross-sections face the camera to remain visible from the side.
Roots slide along and around each limb, following its presented position and
rotation while attached. Visible middle and upper sections trail running and
limb movement by at most 28 game units, with independent 0.12–0.24 second response
times, settling smoothly after stopping. Each strand has a staggered 3.2–6 second
lifecycle: fade in, wander along the body, detach, drift upward in world space
with a little inherited momentum, then fade away. A replacement appears at a
new body position with fresh width, height and opacity variation. Detached
strands no longer follow Link or his limb rotations. The bounded pool needs
no particle assets or unbounded emission history.
The veil fades in over 0.2 seconds and clears on deactivation, hidden/dead/wolf
states, events, scene changes and outfit reloads. Teleports and long frame gaps
reset its movement history. It draws after the world translucent lists, with
scene-depth occlusion and no depth, alpha or normal writes. It uses no new assets.

The previous FIRE02 body flames are **preserved but disabled**, including their
attachment, lifetime, velocity and black-blending hooks. To restore them, set
`kUseLegacyDarkFlames = true` in `src/fierce_deity_veil.hpp` and rebuild. This
switch disables the veil and re-enables the old particle hooks/emission; the two
effects do not run together. Eye bloom is independent of this switch.

Dark and Dark Magic render the eye surfaces themselves as self-lit red with a 2x
emission multiplier, independent of room lighting and preserving eyelid/cutout alpha.
RGB output still clamps to the renderer's normal color range. Selective bloom
captures the actual skinned eye pixels with their native alpha into an offscreen
mask. Scene-depth testing removes hidden pixels before a separable Gaussian blur
adds a soft red spill around the visible eye surface. Its brightness fluctuates
gently by at most 12% with a continuous, frame-rate-independent signal shared by
both eyes. The sharp eye surface remains steady. There are no halo sprites,
projection offsets or enlarged eye meshes. The composite preserves scene alpha,
depth and normal attachments; global bloom and lantern lighting are unchanged.
Captures are bounded to eight eye primitive groups per scene and discarded at
scene/frame boundaries. During warp transitions or without the graphics service,
only the masked self-lit eye surface is drawn.
For the disabled legacy effect, body-flame vertical movement is held at 0.50 units per simulation
step after native force/drag calculation. Particles live for 99 steps to preserve
the original nominal ascent distance (1.10 x 45 = 0.50 x 99); the emission rate
is unchanged, so more particles coexist. Native texture animation is unchanged.

**Dark Link Activation**, below the visual selector, offers **Spin Attack**
(charged), **R+Z**, **R+A** (default), **L3**, or **R3**. Activation requires a full meter unless **Free Transform** is enabled. With
either R shortcut, hold R and then press Z or A, just like native R+Y Quick
Transform. The initial R press can still trigger a manual jump; the shortcut also
works during that jump. Keep holding R and press Z or A again to end early. The
remaining charge stops draining immediately and damaging sword hits can refill
it; the next activation requires full power again. Holding the combination does
not toggle repeatedly. Holding Z/A first and then pressing R does not activate it.
Only Z/A is consumed until release; R and its manual jump remain available.
L3/R3 use a fresh left/right stick click without holding R. Click again to end
Dark Link early and preserve the remaining charge. Holding a stick click never
repeats; clicks held through menus, blocked states, or controller reconnection
must be released before they can activate. Physical and touch stick buttons work.
On Android, touch presses are captured directly so short Z taps and the HD HUD
Z mapping can also trigger the shortcut. A pending Gale charge is cancelled when the
shortcut fires, while the jump already in progress continues normally.

The same submenu contains these gauge/combat options. Gains and depletion use
points; both the built-in bar and HUD integrations display the resulting percentage.
Raising capacity preserves existing points; lowering it clamps charge to the new capacity.

| Setting | Range | Default |
| --- | --- | --- |
| Dark Link Gauge | 1–10,000 points | 100 |
| Sword Attack Gain | 0–1,000 points per damaging sword hit | 5 |
| Damage Received Gain | 0–1,000 points per health-damage event | 0 |
| Depletion Rate | 0–1,000 points/sec | 5 |
| Damage Multiplier | 0–1,000% sword damage | 200% |
| Gauge Auto Fade | Off / On | Off |
| Free Transform | Off / On | Off |

Both gain options apply while Dark Link is inactive. Received damage must actually
reduce pending health: blocked hits, armor-only costs, healing and carried-over
scene-entry damage do not grant points. Wolf-form damage does not charge the gauge.
Zero depletion keeps the form active until manually stopped or otherwise interrupted.
Fractional sword damage rounds up and is capped to the native damage field.

**Free Transform** hides the gauge, including compatible replacement HUDs, and
lets every activation binding toggle Dark Link with any charge. With Spin Attack
selected, a new charged spin toggles the mode on or off. Charge neither fills
nor drains while Free Transform is enabled, and transforming preserves stored
charge. Turning it off restores normal gauge rules and depletion; an active form
with no charge ends on the next gameplay update. The Dark Link feature and its
normal gameplay restrictions still apply.

Gauge Auto Fade hides an empty bar on every HUD preset. HUD preset import/reset
no longer changes this setting. Registered HUD replacements receive its fading alpha;
the KH2 drive API supports show/hide only, so its empty gauge is cleared without
an alpha animation and returns when charge is gained.

Entry and exit use the native flying warp particles at twice the normal speed:
the appearance effect and sound on entry, and the disappearance effect and sound
on exit. Each sound plays once when its visual transition starts. The material
continues to build from top to bottom and recede from bottom to top.

Both selectors are disabled while Dark Link is unavailable/off, including
its Progression lock. Its saved selection is retained. Changing it during a
transformation applies when gameplay resumes, waits for any pending model load,
and preserves the remaining meter. Dark, White and Gold retain the selected outfit;
the Magic variants restore it when the transformation ends. Each warp layer retains
its own appearance when changing visuals during a transformation.

Dark, Dark Magic, White and Gold affect Link's body, face, hair/hat, hands,
worn boots, equipped sword, shield and scabbard, including Dual Wield's second
sword/scabbard, held or stowed. Magic Armor alone retains native equipment colors.
These appearances use the loaded models, including compatible replacements,
and preserve native texture transparency. In White, swords, shields and scabbards
(including Dual Wield) are uniformly white; their texture colors are not inverted.

Dark/Dark Magic eyes glow fully red; Gold eyes glow fully white. White uses
orange-yellow emission for the iris and pupil and turns the sclera black.
Eye recognition uses `eyeballL`/`eyeballR` material names. Eyelids, blinking and
cutout alpha remain native; unknown eye material names use body shading.

On Link's body and eyes, White classifies the raw diffuse texture before scene lighting: near-white means
all three RGB components are at least 224/255 (192/255 on eyes to include shaded
sclera). Those texels become black; black, gray and colored texels become white,
or amber on eyes. This is a deliberate monochrome recolor, not RGB inversion.
Materials without an identifiable diffuse texture or three spare TEV stages use
uniform white/amber; those with no spare stage remain native. Gold and Dark use
specular lighting when the native batch can safely restore the borrowed light.
Other held items retain their normal appearance. Rendering changes are
scoped to the player's draw calls; shared model data and other actors are untouched.

With [Progression System](general-settings.md) enabled, Dark Link unlocks
after freeing Faron; it is available immediately in Boss Rush.

## Shielding and Dual Wield

Manual Shielding provides optional manual guard controls.
[Dual Wield](dual-wield.md) replaces the visible shield with a second sword and
uses a crossed-sword guard and Shield Attack.

For enemy and boss difficulty settings, see [Hard Mode](hard-mode.md).
