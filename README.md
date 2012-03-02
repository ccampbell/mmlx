## About

MMLX is a music programming language used to make NES Chiptunes.  It extends from Music Macro Language:
http://en.wikipedia.org/wiki/Music_Macro_Language

It is short for MML eXtended. Everything written in MML is valid in MMLX, but there are additional features.

If you are familiar with web programming, MMLX is to MML what SASS is to CSS

For a getting started tutorial check out:
https://github.com/ccampbell/mmlx/wiki/Getting-Started

For complete documentation:
https://github.com/ccampbell/mmlx/wiki/Documentation

You can check out some samples in the files/mmlx directory of this repository.  Also an MML beginner's guide is available at:
http://nullsleep.com/treasure/mck_guide/

## Dependencies

**setuptools** or **distribute**:

    curl http://python-distribute.org/distribute_setup.py | python

**pip**:

    curl https://raw.github.com/pypa/pip/master/contrib/get-pip.py | python

this may have to be run as root

## Quick start (stable release)
    pip install mmlx
    mmlx --watch path/to/mmlx

for additional options run

    mmlx --help

### Dev version

If you want to try the latest greatest you can install the dev version

    pip install https://github.com/ccampbell/mmlx/zipball/master

*NOTE: MMLX has only been tested on Python 2.6.1 using Mac OS X at this time*

## Features
* define and use instrument patches
* use ADSR envelopes for creating instruments
* import other MMLX files or instruments into your current file
* use portamento to slide smoothly from one note to the next
* store data such as chords or patterns in variables
* transpose to any key
* target notes directly by octave without having to manually move up and down octaves
* auto generate NSF files on save and open them
* generate separate NSF files for each voice

## To-Do
* Throw proper warnings/errors/syntax checks when code is not valid mmlx
* Unit tests
* Automatic DPCM sample conversions from wav files
* Ability to create random instruments
* Ability to use absolute paths to directories
* Add improved support for expansion packs (VRC7, FME7, etc)
* Skip over existing macros defined in your MMLX file when generating new macros from instruments
