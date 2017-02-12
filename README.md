#The Chrd Chord Chart Editor
[www.chrd.co.uk](http://www.chrd.co.uk)

[![Build Status](https://travis-ci.org/oetherington/chrd.svg?branch=master)](https://travis-ci.org/oetherington/chrd)

Chrd is a markup language and text editor for typesetting chord charts.

## Dependencies
Building requires libz, libpng, gtk3, gtksourceview, evince, clang, make and xxd.

On Ubuntu and derivatives, these may be installed with
```
sudo apt-get install build-essential clang libgtk-3-dev libevince-dev libgtksourceview-3.0-dev libpng-dev libz-dev
```

On Arch and derivaties, these may be installed with
```
sudo pacman -Syu libpng gtk3 gtksourceview3 evince clang make
```

##BUILDING
With all dependencies installed, Chrd may then be built simply by running
`make`. It can then by installed with `sudo make install` and, if needed,
uninstalled with `sudo make uninstall`.

##COPYING
Chrd is free software under the GNU GPLv3. For full details, see "COPYING".
