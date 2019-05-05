OpenMW
======

This is a fork of an OpenMW created for a personal tweaking of the engine.

So far it does:
1) Adds spellcasting VFX removal option to the config (very useful to run OpenMW in a LLVM-pipe software rendering)
2) Adds "bring" script command to be able to easily bring companions after teleportation
3) Removes version information read from "resources/version" file. Switched to hard-coded implementation (I have too many versions of ELF, and only one environment, so they all reported the same version, which is definitely not what I want)
4) Removes CI
5) Most important: fixed system memory leak (a lot of infinitely hanging dead openmw processes still in RAM) by a kludge: crashcatcher is not installed anymore
6) Skpped mandatory log file generation (part of the reasoning: see i.3)

Notes:
 * I don't recommend it to use this branch by anyone except me :)
 * This branch is designed to break compatibility with mac and droid (probably win as well, as I'm a Linux-only user)
