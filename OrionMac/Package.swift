// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "Orion",
    platforms: [
        .macOS(.v13)
    ],
    dependencies: [
        .package(path: "../OrionKit")
    ],
    targets: [
        .executableTarget(
            name: "Orion",
            dependencies: ["OrionKit"]),
    ]
)
