# Aetherburn

Aetherburn is an Unreal Engine game project.

## Repository setup

Git LFS is required because Unreal maps, assets, and other large binary source files are stored outside normal Git history.

1. Install [Git LFS](https://git-lfs.com/).
2. Run `git lfs install` once on your machine.
3. Clone the repository normally. Git LFS downloads the project assets automatically.
4. Right-click `Aetherburn.uproject` and generate project files when IDE files are needed.

Generated Unreal Engine folders and IDE solution files are intentionally not committed. Do not add `Binaries`, `DerivedDataCache`, `Intermediate`, `Saved`, or `.vs` to source control.

## Meshy editor plugin

Meshy Bridge 0.2.0 is installed in `Plugins/meshy` and enabled for the Editor in `Aetherburn.uproject`. It is third-party Meshy plugin code, not Aetherburn gameplay code.

The source came from the local `meshy-for-unreal-ue5.7-0.2.0` download. The project copy declares Unreal Engine 5.8 and is rebuilt from source; the downloaded 5.7 binaries were not copied. The original download remains unchanged.

The bridge service starts automatically with the Editor and listens on local port 5327. Use **Window > Meshy Bridge** to stop or restart it. A successful compile alone does not verify an actual Meshy asset transfer.

Validation (22 September 2026): AetherburnEditor compiled successfully for Unreal Engine 5.8 on Windows. The plugin emits include-path and deprecated font-constructor warnings. Editor menu loading and a Meshy asset transfer still need checking after restarting the Editor.

## Thunderlord playable character

Thunderlord of Olympus is configured as the full-body player character using `ZeusMaxiamoRig.fbx` from Downloads. The final skeletal mesh, skeleton, generated physics asset, baked material, and two embedded textures are imported under `Content/Thunderlord/ZeusImport`. The old Meshy skeletal mesh, its imported textures, and the animations retargeted to that skeleton have been removed. The Meshy editor plugin remains installed as a separate editor tool.

The imported Zeus skeleton is the target for a dedicated set of Mixamo animations. Nineteen animation-only FBXs were downloaded from Mixamo with **Without Skin** selected and imported onto the Zeus skeleton. The 12 standing and crouched locomotion cycles were exported with **In Place** enabled, so movement comes from the character controller rather than translation baked into the animation. The downloaded source FBXs are kept in `Downloads/Aetherburn_Mixamo_Animations`; the Unreal animation sequences live in `Content/Thunderlord/Zeus/Animations/Mixamo`.

The editable animation map is `Content/Thunderlord/Zeus/AnimationMap/DA_ThunderlordAnimations`; its runtime graph is `ABP_Thunderlord`. The state machine has Locomotion, Crouch, Slide, Jump, and Fall states with explicit transition-rule graphs and a separate animation player for each action. Jump plays once, then switches to looping Fall; touching down returns directly to crouch, slide, or locomotion with a short blend and no landing clip. Walk-to-jump transitions are immediate, while transitions into Fall blend for 0.24 seconds to soften the arms-out pose. Slide plays its in-place clip once and ends gameplay sliding when that 1.53-second clip completes; it then transitions to crouch or locomotion. There are no slide entry or exit clips in the map. Ordinary ground transitions use 0.18-second standard blends. Generated in-place copies lock root translation for slide, jump, and fall so animation playback cannot drag the mesh away from the capsule. The Thunderlord Blueprint applies a -60-unit vertical mesh adjustment.

The character loads `ABP_Thunderlord_C` as its runtime Anim Class. The native `UThunderlordAnimInstance` supplies state and transition data as the Blueprint parent; assigning that native parent directly would produce only the imported A-pose because it has no animation graph.

The original `-96` vertical mesh offset put the Zeus mesh too low. Thunderlord uses a character-specific mesh adjustment; it was lowered another 12 units to `-60` in the latest animation pass, leaving other character variants unchanged.

Unreal 5.8's experimental Interchange FBX importer did not extract this FBX's embedded textures. The FBX was imported through Unreal's legacy FBX importer with `Interchange.FeatureFlags.Import.FBX=False`; the project configuration was restored after importing. The checked import contains two `Texture2D` assets.

The animation set is built from 19 animation-only Mixamo FBXs and four generated root-locked action sequences. The character uses a third-person spring-arm camera and turns toward movement input. Crouching blends the full body into a lower posture while keeping directional locomotion. Sliding uses a shorter committed posture with limited steering and speed decay. The movement implementation remains in `AAetherburnCharacter`.

The slide follows a classic pre-omnimovement Call of Duty rhythm: it requires forward sprint speed, receives an entry boost, lasts at most 1.25 seconds or 950 Unreal units, decelerates continuously, exits below 260 units per second, and has a 1.1-second cooldown. Releasing crouch does not cancel an active slide. Movement acceleration is disabled during the committed slide so held input cannot sustain it indefinitely.

The playable map is `/Game/Volcanic_temple/Levels/L_Showcase`. It uses the native `AThunderlordGameMode`, which selects `BP_Thunderlord` as its default pawn, and contains `PlayerStart_Aetherburn` on the central ground. This keeps the map-level override aligned with the Thunderlord player setup.

Controls:

- WASD: move relative to the camera
- Mouse: orbit and aim the camera
- Space: jump
- Left Shift: run
- Left Control or C: crouch; press while running to slide

Validation (23 September 2026): the C++ Editor target compiled successfully and the animation map was rebuilt through Unreal's editor commandlet. The map has five states, immediate jump entry, a 0.24-second transition into Fall, and no landing state. The Showcase uses `BP_Thunderlord`, which applies the -60-unit offset. A Play In Editor visual check is still needed for arm blending, takeoff response, foot contact, slide completion, and camera framing.
