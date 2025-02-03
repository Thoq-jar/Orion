// swift-tools-version: 5.9
import PackageDescription

#if os(Windows)
let linkerSettings: [LinkerSetting] = [
    .linkedLibrary("ucrt"),
    .linkedLibrary("vcruntime"),
    .linkedLibrary("msvcrt")
]
#else
let linkerSettings: [LinkerSetting] = []
#endif

let package = Package(
    name: "OrionKit",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .library(
            name: "OrionKit",
            type: .dynamic,
            targets: ["OrionKit"]),
    ],
    targets: [
        .target(
            name: "OrionKit",
            dependencies: ["COrionKit"],
            linkerSettings: linkerSettings
        ),
        .target(
            name: "COrionKit",
            dependencies: []
        )
    ]
)
