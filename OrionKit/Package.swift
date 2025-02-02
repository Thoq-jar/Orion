// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "OrionKit",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .library(
            name: "OrionKit",
            type: .dynamic,
            targets: ["OrionKit", "COrionKit"]),
    ],
    targets: [
        .target(
            name: "COrionKit",
            publicHeadersPath: "include"),
        .target(
            name: "OrionKit",
            dependencies: ["COrionKit"])
    ]
)
