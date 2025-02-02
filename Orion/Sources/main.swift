// The Swift Programming Language
// https://docs.swift.org/swift-book

import SwiftUI
import AppKit

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate

_ = NSApplicationMain(CommandLine.argc, CommandLine.unsafeArgv)

class AppDelegate: NSObject, NSApplicationDelegate {
    var window: NSWindow?
    
    func applicationDidFinishLaunching(_ notification: Notification) {
        setupMenu()
        NSApp.setActivationPolicy(.regular)
        
        let contentView = ContentView()
        
        window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 600, height: 400),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window?.title = "Orion"
        window?.center()
        window?.contentView = NSHostingView(rootView: contentView)
        window?.makeKeyAndOrderFront(nil)
        
        NSApp.activate(ignoringOtherApps: true)
    }
    
    func setupMenu() {
        let mainMenu = NSMenu()
        
        let appMenu = NSMenu()
        let appMenuItem = NSMenuItem()
        appMenuItem.submenu = appMenu
        
        let aboutMenuItem = NSMenuItem(
            title: "About Orion",
            action: #selector(showAbout),
            keyEquivalent: ""
        )
        
        let quitMenuItem = NSMenuItem(
            title: "Quit Orion",
            action: #selector(NSApplication.terminate),
            keyEquivalent: "q"
        )
        
        appMenu.addItem(aboutMenuItem)
        appMenu.addItem(NSMenuItem.separator())
        appMenu.addItem(quitMenuItem)
        
        mainMenu.addItem(appMenuItem)
        NSApp.mainMenu = mainMenu
    }
    
    @objc func showAbout() {
        let year = Calendar.current.component(.year, from: Date())
        let alert = NSAlert()
        alert.messageText = "Orion"
        alert.informativeText = "© \(year) Thoq\nA fast file search utility."
        alert.alertStyle = .informational
        alert.addButton(withTitle: "OK")
        alert.runModal()
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
}
