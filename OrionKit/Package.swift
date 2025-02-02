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
            targets: ["OrionKit"]),
    ],
    targets: [
        .target(
            name: "OrionKit"),
        .testTarget(
            name: "OrionKitTests",
            dependencies: ["OrionKit"]),
    ]
)
