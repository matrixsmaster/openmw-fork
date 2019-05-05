OpenMW
======

This is a fork of an OpenMW created for a personal tweaking of the engine.

So far it does:
1) Adds spellcasting VFX removal option to the config (very useful to run OpenMW in a LLVM-pipe software rendering)
2) Adds "bring" script command to be able to easily bring companions after teleportation
3) Removes version information read from "resources/version" file. Switched to hard-coded implementation (I have too many versions of ELF, and only one environment, so they all reported the same version, which is definitely not what I want)
4) Removes CI
