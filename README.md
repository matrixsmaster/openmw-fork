OpenMW-Fork
===========

This is a personal OpenMW fork created for experiments and fun engine-tweaking.


https://youtu.be/m3_be5weKW8

---


Changelist
----------


1) Most important: fixed system memory leak (a lot of infinitely hanging dead openmw processes still in RAM) by a kludge: crashcatcher is not installed anymore

1) Add spellcasting VFX removal option to the config (very useful to run OpenMW in a LLVM-pipe software rendering)

1) Add "bring" script command to be able to easily bring companions after player teleportation

1) Add "inventory transfer" magic effect (much like ref1.RemoveAllItems [ref2] command in Oblivion)

1) Enable collisions with user-placed objects (configurable via config file)

1) Enchantment is a serious business now (added item's gold value modification based on enchantment cost)

1) CommandHumanoid spell with a maximum magnitude will now creates a temporary companion for a duration of the spell (adds ability to re-equip NPCs)

1) Add a simple hack for RaceMenu UI, so it's possible now to make cosmetic changes without complete reset of the player character

1) Make it possible to use DosCard VM inside the game

1) Simplify OpenCS record filter feature (much easier and intuitive to use without worrying about the special syntax)

1) Simplify input subsystem, and remove joystick/gamepad support (I'm not using them)

1) Remove dependency on:

    1) OICS
    1) TinyXML

1) Remove company logo video playback (actually, I made it optional, but disabled by default) - I'm tired of Esc'ing it for almost 17 years!

1) Fix inconsistent docstring formatting in a couple of core headers

1) Remove version information read from "resources/version" file. Switched to hard-coded implementation (I have too many versions of ELF, and only one environment, so they all reported the same version, which is definitely not what I want)

1) Disable mandatory log file generation (reasoning: see above)

1) Remove CI support (I'm not using it, since I have only one platform and always personally use the latest master anyway)

---

