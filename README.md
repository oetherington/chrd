# The Chrd Chord Chart Editor
[chrd.etherington.xyz](http://chrd.etherington.xyz)

Chrd is a markup language and text editor for typesetting chord charts.

## Dependencies

Building the C++ version requires libz, libpng, gtk3, gtksourceview, evince, clang, make and xxd.

On Ubuntu and derivatives, these may be installed with

```
sudo apt-get install build-essential clang libgtk-3-dev libevince-dev libgtksourceview-3.0-dev libpng-dev libz-dev
```

On Arch and derivaties, these may be installed with

```
sudo pacman -Syu libpng gtk3 gtksourceview3 evince clang make
```

## BUILDING

With all dependencies installed, the C++ version may then be built simply by
running `make`. It can then by installed with `sudo make install` and, if needed,
uninstalled with `sudo make uninstall`.

## COPYING
Chrd is free software under the GNU GPLv3. For full details, see "COPYING".
