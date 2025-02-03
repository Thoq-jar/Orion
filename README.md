# Orion
A fast file search engine written in Swift and C++ with a platform-native UI.

## Building
### Linux
Prerequisites:
- Swift
- CMake
- GnuMake
- GTK3

```bash
./scripts/build.sh
```

### macOS
Prerequisites:
- Swift

```bash
swift build --package-path OrionMac
```

### Windows
Follow the instructions in the [Windows guide](docs/WindowsDev.md)

## Running
### Linux
```bash
./scripts/run.sh
```

### macOS
```bash
swift run --package-path OrionMac
```

### Windows
Follow the instructions in the [Windows guide](docs/WindowsDev.md)

## Notes
The Linux build also runs on macOS. If you prefer GTK look and feel, it should work out of the box.
Windows support is coming soon but you could try running the GTK build on Windows but good luck with that.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE.md) file for details.