# List Directory

Is a simple ls utility for Windows.

## Usage

```txt
ls <options> wild-card

options:
    -x list directory entries line by line.
    -c list directory entries in columns (default).
    -a list hidden files.
    -d only list directories.
    -m add a comma after each entry.
    -R list recursively.
    -l list the file size, last write time and the file name.
```

## Building

Building with CMake

```txt
mkdir build
cd build
cmake ..
```
