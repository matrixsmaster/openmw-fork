OpenMW-Fork
===========

Disclaimer
----------
 * I don't recommend using this branch to anyone! Proceed at your own risk.
 * This branch is __NOT__ designed to be pulled into the main project, it's just a __personal__ "having fun" branch
 * This branch is deliberately breaks compatibility with mac and droid (probably win32 as well, as I'm a Linux-only user)
 * This branch _could_ possibly be incompatible with upstream (maybe I will try to avoid that, though)
 * This branch _should_ be somewhat incompatible with vanilla Morrowind

Changes
-------

This is a fork of an OpenMW created for a personal tweaking of the engine.

So far it does:

1) Adds spellcasting VFX removal option to the config (very useful to run OpenMW in a LLVM-pipe software rendering)

2) Adds "bring" script command to be able to easily bring companions after PC teleportation

3) Removes version information read from "resources/version" file. Switched to hard-coded implementation (I have too many versions of ELF, and only one environment, so they all reported the same version, which is definitely not what I want)

4) Removes CI

5) Most important: fixed system memory leak (a lot of infinitely hanging dead openmw processes still in RAM) by a kludge: crashcatcher is not installed anymore

6) Skpped mandatory log file generation (part of the reasoning: see i.3)

7) Adds "inventory transfer" magic effect

8) Enchantment is a serious business now (added item's gold value modification based on enchantment cost)

9) Fixed inconsistent docstring formatting in a couple of core headers (more to do)

10) CommandHumanoid spell with a maximum magnitude will now create a temporary companion for a duration of the spell (adds ability to re-equip NPCs)

11) Joystick/gamepad support totally removed (I'm not using them)

12) Removed dependencies:

    1) OICS
    2) TinyXML
