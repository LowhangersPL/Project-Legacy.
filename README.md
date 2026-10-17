# Project Legacy

Project Legacy was a community-run Vanilla 1.12 Classic+ World of Warcraft server, built around familiar Azeroth with new progression systems, class changes, quality-of-life improvements, and custom content.

This repository preserves the project's server source code and world database so others can explore the work, learn from it, and continue developing their own projects under the applicable licenses.

## Features

Development included:

- **Prestige:** Additional progression after reaching level 60, with cosmetic rewards.
- **Lorewalker:** An optional leveling challenge with reduced creature XP and milestone rewards.
- **War Mode:** Optional world PvP participation with increased quest XP.
- **Treasure Trails:** A clue-scroll system with randomized steps, reward caskets, and multiple difficulty tiers.
- **Cross-faction play:** Grouping, guilds, raids, and inspection.
- **Battleground improvements:** Cross-faction battlegrounds, remote queueing, and battleground XP.
- **Guild banks** and **talent inspection**.
- **Class and talent changes**, including Arms Warrior, Assassination Rogue, and Affliction Warlock reworks.
- **Karazhan Crypts:** Custom 10-player content.
- **Raid changes:** No world buffs in raids, removal of the boss debuff limit, and a 25-player Onyxia encounter.
- Additional custom quests, creatures, items, scripts, and world changes.

Feature availability depends on the source and database snapshot included here. Some features also require matching client modifications that are not included in this repository.

## Repository Contents

| Path | Contents |
| --- | --- |
| `src/` | Server source code and configuration templates |
| `sql/` | SQL scripts, base schemas, and updates included with the source |
| `database/` | Project Legacy world database snapshot and empty database schemas |
| `cmake/` | CMake build support |
| `contrib/` | Supporting tools and utilities |
| `dep/` | Bundled dependencies |
| `docs/` | Existing technical documentation |

## Database Files

- **`database/Project_Legacy_World.zip`** contains the exported world database with Legacy's world content.
- **`database/characters_structure.sql`** contains the character database schema without player records.
- **`database/realmd_structure.sql`** contains the authentication database schema without account records.
- **`database/logs_structure.sql`** contains the logging database schema without log records.

The world SQL file must be extracted from its ZIP before importing.

These files are intended for a fresh installation. Review database names, `USE` statements, and any `DROP` statements before importing. Do not import them into an existing server without a backup.

Structure-only databases may require initial configuration records, including a realm entry, before the server can run.

## Building and Setup

This is a source and database archive, not a ready-to-run repack.

A general setup involves:

1. Installing the compiler, CMake, and database dependencies required by this version of the core.
2. Building the source with CMake.
3. Creating and importing the world, character, authentication, and logging databases.
4. Creating runtime configuration files from the supplied templates and setting your own database credentials and paths.
5. Configuring a realm entry and creating your own administrator account.
6. Obtaining the required client-derived runtime data using the appropriate tools.
7. Supplying any matching client modifications required by the custom content.

Consult the source, CMake files, and documentation for version-specific requirements. A clean installation from this public archive has not yet been independently verified.

## Release Status

This repository is provided as an archival release.

It may contain unfinished work, experimental changes, bugs, and documentation inherited from the original core. Inclusion of a feature or script does not guarantee that it is complete or production-ready.

No ongoing development, hosting, updates, or technical support are promised.

## Client Files

The World of Warcraft client and extracted Blizzard game assets are not included. This repository alone is not a complete playable installation.

Project Legacy is an independent fan project and is not affiliated with or endorsed by Blizzard Entertainment.

## License and Credits

See `LICENSE` for the server source license. Existing copyright notices and third-party licenses remain applicable to their respective components. Database content and other components must be used in accordance with their applicable licenses and permissions.

Credit belongs to the original MaNGOS core developers, the developers and maintainers of the underlying fork and repack, dependency authors, database contributors, and everyone whose work is included here.

## Thank You

Thank you to everyone who played Project Legacy, reported bugs, shared ideas, helped other players, or supported the server.

You turned a development project into a community. This release preserves the work that went into it and gives others the opportunity to build on what we started.

**Lowhangers**
