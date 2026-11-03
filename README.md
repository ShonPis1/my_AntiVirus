# my_AntiVirus

A signature-based file scanner written in C. Given a folder and a virus signature file, it scans every file in the folder, reports which ones contain the signature and where it was found, and writes the results to a log file.

Built as a course assignment (Magshimim) and then extended: binary-safe signature search, a portable build, tests, and fixes for several bugs.

## How it works

- The signature is read as raw bytes, so it may contain zero bytes.
- **Normal scan:** searches the whole file.
- **Quick scan:** searches only the first 20% and the last 20% of each file, and reports which part matched.
- Files are scanned in alphabetical order. Sub-folders are not scanned.
- The log is written to `myAntiVirusLog.txt` inside the scanned folder (that file is skipped by the scan).

Example output (quick scan):

```
Folder to scan: folderToCheck
Virus signature: KittenVirusSign

Scanning:
folderToCheck/kitten_frog.jpg Infected! (last 20%)
folderToCheck/kitten_run.jpg Clean!
```

## Build and run

Linux / macOS:

```bash
make
./antivirus folderToCheck KittenVirusSign
```

Windows: open `AntiVirus_Magshimim.sln` in Visual Studio and run with the arguments `<folder> <signature file>` (a bundled `dirent.h` is used there).

The program asks for the scan type: `0` for a normal scan, any other key for a quick scan.

## Tests

```bash
make test
```

The script builds the scanner with AddressSanitizer and UBSan and runs it on generated files: signature at the start, middle and end of a file, a clean file, an empty file, and a file containing only the beginning of a signature that has a zero byte inside.

## Project structure

```
AntiVirus_Magshimim/prog.c   the scanner
AntiVirus_Magshimim/dirent.h directory API for Visual Studio (Windows only)
folderToCheck/               sample files to scan
KittenVirusSign              sample virus signature
tests/run_tests.sh           tests
Makefile                     build and test targets
```

