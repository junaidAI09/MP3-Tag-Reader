# MP3 Tag Reader
A command-line MP3 metadata viewer and editor developed using C.

## Overview
This project reads and edits ID3v2.3 metadata tags in MP3 files.
It allows users to view existing song information and modify selected metadata fields through command-line arguments.

## Features
- View MP3 metadata.
- Edit selected ID3 tags.
- Support for ID3v2.3 format.
- Command-line based interface.
- File handling and metadata processing.

## Technologies Used
- Programming Language: C
- Concepts: File Handling, Structures, Pointers
- Standard: C11
- Compiler: GCC

## Compilation
```bash
gcc -Wall -Wextra -std=c11 main.c mp3view.c mp3edit.c -o mp3reader
```
## Usage
View MP3 details:
```bash
./mp3reader -v sample.mp3
```
Edit Metadata:
```bash
./mp3reader -e -t "New Title" sample.mp3
```
Other supported options:
-t : Title
-a : Artist
-A : Album
-m : Year
-y : Genre
-c : Comment

## Author
Junaid AI09
