# List Directory

Is a simple list directory utility for Windows.

## Usage

```txt
ls <options> wild-card

options:
    -x  list directory entries line by line.
    -c  list directory entries in columns (default).
    -a  list hidden files.
    -as list hidden files as well as system files.
    -d  only list directories.
    -f  only list files.
    -m  add a comma after each entry.
    -R  list recursively.
    -l  list the file size, last write time and the file name.
    -S  build a short path name. 
    -h  show this help message.
```

## Building

Building with CMake

```txt
mkdir build
cd build
cmake ..
```
