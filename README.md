# godot-format
Formats .h, .cpp etc files to pass Godot Engine static checks
Version 0.01

### WARNING

__This program is DESTRUCTIVE, be sure to commit your code as a backup BEFORE running godot format. BE SURE to test on a test folder the first time you use it, to make sure you understand how to use. Running it in the wrong folder could be *very bad*, although it creates backups they would need to be restored manually.__

This simple program (linux only so far, feel free to #define windows file functions) automatically edits files to pass Godot static checks. It does the changes that clang format cannot.

It operates RECURSIVELY through folders on .h, .cpp, .glsl files (but not .gen.h)

* Adds licences
* Removes extra blank lines
* Removes extra tabs

## Installation
g++ godot_format.cpp -o godot_format

## Running
```
./godot_format --verbose --dryrun /home/juan/godot/drivers
```

* `--verbose` gives extra log info
* `--dryrun` is a good idea especially first time, it does everything except write the modified files

Original files will be renamed to .gf_backup, modified files will replace the originals. You will have to delete the backup files manually (as a safety measure)

Note that Godot static checks are not run on some folders (e.g. thirdparty).
So you will generally only need to run this program in areas you are modifying.
