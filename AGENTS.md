# AGENTS.md — Unreal Engine Development and Fontys Portfolio Instructions

## Project context

This is an Unreal Engine game project developed for a Fontys ICT portfolio. Preserve working maps, Blueprints, C++ code, assets, plugins, configuration, and source-control settings.

The portfolio is maintained outside the project repository under `C:/Users/cvoog/Documents/GameDevelopment/documentation`. Determine the project name from the `.uproject` filename and use it in every portfolio document.

## Main workflow

For every task:

1. Inspect the relevant code, assets, maps, settings, plugins, and current Git changes.
2. Make the smallest useful change that solves the request.
3. Build and test the strongest relevant part that can be tested.
4. Update meaningful documentation during the same task.
5. Report honestly what works, what was tested, and what still needs checking.

Preserve unrelated user or teammate work. Never discard, overwrite, move, or reorganize it carelessly.

## Prefer C++

Use native Unreal C++ as much as reasonably possible for gameplay and project systems. Prefer C++ for:

- gameplay rules, state, and reusable components;
- characters, pawns, controllers, game modes, game states, and player states;
- interactions, items, combat, abilities, AI, saving, networking, and replication;
- data processing, performance-sensitive work, reusable libraries, and tests;
- interfaces, delegates, events, subsystem logic, and stable project architecture.

Use Blueprints mainly for:

- assigning assets and tuning exposed values;
- animation graphs, sequences, effects, sounds, and presentation;
- UI layout and simple widget connections;
- small level-specific events and designer-authored content;
- rapid experiments that will be moved to C++ if they become a real system.

Do not place important reusable gameplay rules only in a Level Blueprint. Do not create a large Blueprint system when clear C++ components would be easier to maintain, test, and reuse. When Blueprint access is useful, expose a focused C++ API with appropriate `UCLASS`, `USTRUCT`, `UENUM`, `UPROPERTY`, and `UFUNCTION` specifiers. Avoid exposing internal mutable state without a reason.

Blueprint subclasses of C++ classes are encouraged for asset assignment and tuning. Keep the rule in C++ and the content choice in the Blueprint where practical.

## Unreal project safety

Before changing the project, inspect the `.uproject`, relevant `.Build.cs` and `.Target.cs` files, `Config/`, source modules, maps, Blueprints, Data Assets, input setup, and enabled plugins.

Do not edit generated or temporary folders such as `Binaries/`, `DerivedDataCache/`, `Intermediate/`, or `Saved/`. Do not commit those folders unless the repository deliberately tracks a specific file.

Treat `.uasset` and `.umap` files as binary assets. Do not attempt risky text replacements inside them. Avoid moving or renaming Unreal assets outside the Editor because redirects and references may break. When assets are moved in the Editor, fix redirectors and validate references.

Keep changes to plugins, engine version, target files, module dependencies, input mappings, collision channels, packaging, and project configuration focused and explained.

## C++ style and architecture

Follow Unreal naming, reflection, ownership, and garbage-collection rules. Match the project’s existing module and folder structure.

Prefer:

- `UActorComponent` for reusable actor behaviour;
- `UObject` for managed non-actor objects that need reflection;
- plain C++ types for logic that does not need Unreal reflection;
- `UDataAsset` or `UPrimaryDataAsset` for designer-editable shared data;
- Unreal interfaces for genuinely shared capabilities;
- delegates or explicit references for communication;
- `TObjectPtr` and supported Unreal containers where appropriate;
- forward declarations in headers when safe;
- implementation details in `.cpp` files to limit compile dependencies.

Avoid unnecessary Tick functions, global searches, hard-coded asset paths, giant base classes, catch-all managers, and circular module dependencies. Disable ticking when it is not needed. Use timers, events, delegates, or state changes for occasional work.

Do not add a framework, subsystem, singleton, plugin, or third-party dependency unless it clearly benefits the task or the project already uses it.

## Networking, input, UI, and assets

Respect server authority and ownership. Replicate only the state that clients need. Use RPCs deliberately, validate who may call them, and do not claim multiplayer behaviour was tested unless it actually ran with the relevant server/client setup.

Follow the existing input system, preferably Enhanced Input when already configured. Preserve Input Actions, Mapping Contexts, player ownership, and local multiplayer behaviour.

Keep gameplay rules outside UMG widgets where practical. Widgets display state and forward player intent; gameplay classes own the rules and state.

Use soft references when assets do not need to load immediately. Preserve Data Assets, Data Tables, materials, Niagara systems, animations, collision settings, and Blueprint defaults. Never assume an asset is unused only because no C++ reference was found.

## Build, testing, and debugging

Use the strongest validation available:

- compile the affected Unreal target and configuration;
- run Unreal Automation Tests when relevant;
- test the affected map or gameplay loop in PIE;
- use standalone, packaged, multiplayer, or target-device testing when the feature requires it;
- inspect logs, Blueprint compiler errors, asset references, and packaging output.

Do not claim that a map, Blueprint, packaged build, interaction, network flow, or visual result works unless it was actually tested. Explain remaining Editor tests in simple language. Remove noisy temporary logging before finishing.

## Documentation location and format

Never create portfolio documentation inside the Unreal repository. Store it under:

`C:/Users/cvoog/Documents/GameDevelopment/documentation`

Every portfolio document has:

- editable Markdown under `documentation/_sources/<project-name>/<learning-outcome>/`;
- a matching polished PDF under `documentation/<learning-outcome>/`.

Visible learning-outcome folders contain PDFs only. Use these folders:

- `analysis/`
- `advice/`
- `design/`
- `realisation/`
- `manage-and-control/`
- `professional-standard/`
- `personal-leadership/`

Prefix PDF filenames with the project name and do not number filenames. Use the shared PDF renderer and stylesheet in the documentation root. Re-render a PDF whenever its Markdown changes and verify that it exists, is non-empty, and has the correct title.

## Keep documentation synchronized

Keep documentation up to date throughout development. When a project change affects the design, planning, scope, controls, map, interface, assets, architecture, testing, or documented behaviour, update the relevant Markdown and PDF during the same task.

Create or update a focused Realisation devlog when a substantial feature or larger function is completed. Examples include a gameplay system, AI behaviour, multiplayer feature, save system, complete UI flow, map area, tool, or reusable C++ component. Explain:

- the starting goal;
- important design choices;
- how it was made in understandable language;
- the role of C++ and any supporting Blueprints;
- problems and changes;
- real testing and evidence;
- the resulting behaviour;
- the student’s own contribution.

Do not create a new devlog for every small method, minor bug fix, rename, tuning change, or cleanup. Add those to the nearest existing devlog or main document.

## Historical accuracy

Write initial Analysis, Initial Advice, the first Design Document, and the Project Plan as if they were written at the beginning. Only include ideas, questions, choices, and plans known at that moment. Do not add later results or completed-project knowledge. Put later work in dated devlogs, playtest reports, feedback, later advice, or evaluations. Do not call documents retrospective unless the user asks.

Clearly distinguish engine or school starter content, Marketplace assets, plugins, teammate work, AI-generated material, and the student’s own work. Never claim ownership of Unreal Engine systems or third-party content.

## Writing style

Write naturally from the student’s point of view. Use familiar language and explain technical terms briefly when they matter. Do not fill portfolio documents with command flags, process codes, raw logs, build-tool details, or audit language. For example, write that the project compiled successfully rather than listing every build command option.

Do not invent evidence, tests, feedback, performance figures, commits, sources, contributions, or reflection. Use honest TODOs for missing material.

## Design images

Every important design question or choice needs a relevant image close to its explanation. Use an original sketch, mood-board reference, generated concept, diagram, asset example, or real screenshot. A map design must show the full map layout; a UI design must show the full interface. Cropped details may follow the complete overview.

Give every image a caption and identify it as the student’s work, AI-generated concept art, project content, Marketplace content, or an outside reference. Do not use a finished screenshot in a starting document as though it existed at the start. A later result image does not replace the original design reference.

## Sources and evidence

Full Analysis and Design documents include APA-style citations and a References section when research or established methods influenced the work. Only cite sources actually consulted and prefer official Unreal documentation, original research, books, and trustworthy sources.

Useful evidence includes real C++ files, focused commits, Blueprint or Editor screenshots, maps, Data Assets, Automation Test results, PIE captures, packaged builds, and before-and-after behaviour. Do not paste entire classes when a focused excerpt or file reference is clearer.

## Final rule

Build the requested Unreal feature primarily in C++ where practical, use Blueprints as a focused supporting layer, validate it honestly, preserve project assets and user work, and keep the matching portfolio documents and PDFs current.
