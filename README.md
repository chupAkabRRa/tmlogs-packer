# Time Machine Logs Packer (TMLP)

A C++ console tool to pack and unpack directories of log files into "archive", optimized for deduplication. Includes a functional test suite.

## Build Instructions

### Prerequisites
- CMake >= 3.20
- C++20 compatible compiler

### Steps

1. Clone the repo
2. Generate the build system:
    - For single-config (e.g. Make):
        ```bash
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
        ```
    - For Visual Studio (multi-config):
        ```bash
        cmake -S . -B build -G "Visual Studio 17 2022"
        ```
3. Build the project:
    - Single-config:
        ```bash
        cmake --build build
        ```
    - Visual Studio:
        ```bash
        cmake --build build --config Release
        ```

## Usage Instructions

### Pack a Directory

```bash
packer pack <source_directory> <output_file>
```

### Unpack an Archive

```bash
packer unpack <input_file> <target_directory>
```

## Run Functional Tests

1. Build `packer_test` target (`--config` part is needed for multi-config generators):
```bash
cmake --build build --config Release
```

2. Run tests via CTest:
```bash
cd build
ctest -C Release
```

The test:
* creates a temp directory with nested subfolders and files with basic content;
* packs it using `packer`;
* unpacks into another directory;
* compares contents (including sizes and contents);
* cleans up after itself.

## Notes
* Identical files are stored once, referenced by multiple paths.
* The archive is binary format; unpacking restores the exact folder structure and contents.
* Hashing uses *xxHash64* for fast detection of identical files.
* Large files are streamed with minimal memory usage.
