import Foundation
#if os(macOS)
import AppKit
#endif

public struct SearchResult: Identifiable, Hashable {
    public let id = UUID()
    public let path: String
    
    public init(path: String) {
        self.path = path
    }
    
    public func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
    
    public static func == (lhs: SearchResult, rhs: SearchResult) -> Bool {
        lhs.id == rhs.id
    }
}

public class FileSearcher {
    public typealias ProgressCallback = (Double) -> Void
    private var currentTask: Task<[SearchResult], Error>?
    
    public init() {}
    
    public func cancelSearch() {
        currentTask?.cancel()
        currentTask = nil
    }
    
    public func search(query: String, in directory: String, progress: @escaping ProgressCallback) async throws -> [SearchResult] {
        cancelSearch()
        
        let task = Task<[SearchResult], Error> {
            var results: [SearchResult] = []
            let fileManager = FileManager.default
            
            guard let enumerator = fileManager.enumerator(atPath: directory) else {
                throw NSError(domain: "FileSearcher", code: 1, userInfo: [NSLocalizedDescriptionKey: "Could not access directory"])
            }
            
            let allPaths = Array(sequence(state: enumerator as NSEnumerator) { enumerator in
                enumerator.nextObject() as? String
            })
            
            let totalFiles = Double(allPaths.count)
            var processed: Double = 0
            
            for path in allPaths {
                try Task.checkCancellation()
                
                let fullPath = (directory as NSString).appendingPathComponent(path)
                var isDirectory: ObjCBool = false
                
                if fileManager.fileExists(atPath: fullPath, isDirectory: &isDirectory) && !isDirectory.boolValue {
                    if path.localizedCaseInsensitiveContains(query) {
                        results.append(SearchResult(path: fullPath))
                    }
                }
                
                processed += 1
                progress(processed / totalFiles)
            }
            
            return results
        }
        
        currentTask = task
        return try await task.value
    }
    
    #if os(macOS)
    public func openInFinder(path: String) {
        NSWorkspace.shared.selectFile(path, inFileViewerRootedAtPath: "")
    }
    #elseif os(Linux)
    public func openInFinder(path: String) {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/xdg-open")
        process.arguments = [path]
        try? process.run()
    }
    #elseif os(Windows)
    public func openInFinder(path: String) {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "C:\\Windows\\explorer.exe")
        process.arguments = ["/select,", path]
        try? process.run()
    }
    #else
    public func openInFinder(path: String) {
        print("Unsupported platform")
    }
    #endif
}
