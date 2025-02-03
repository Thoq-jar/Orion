import Foundation

#if os(macOS)
    import AppKit
#endif

public struct SearchProgress {
    public let progress: Double
    public let status: String
}

public class FileSearcher {
    public typealias ProgressCallback = (SearchProgress) -> Void
    private var currentTask: Task<[SearchResult], Error>?

    private let concurrentTasks = max(1, ProcessInfo.processInfo.processorCount - 1)

    public init() {}

    public func cancelSearch() {
        currentTask?.cancel()
        currentTask = nil
    }

    public func search(query: String, in directory: String, progress: @escaping ProgressCallback)
        async throws -> [SearchResult]
    {
        cancelSearch()

        let task = Task<[SearchResult], Error> {
            var results: [SearchResult] = []
            let fileManager = FileManager.default

            guard let countEnumerator = fileManager.enumerator(atPath: directory) else {
                throw NSError(
                    domain: "FileSearcher", code: 1,
                    userInfo: [NSLocalizedDescriptionKey: "Could not access directory"])
            }

            var paths: [String] = []
            var filesCounted: Double = 0
            let chunkSize: Double = 1000
            
            while let path = countEnumerator.nextObject() as? String {
                paths.append(path)
                filesCounted += 1
                if filesCounted.truncatingRemainder(dividingBy: chunkSize) == 0 {
                    progress(SearchProgress(
                        progress: 0.5 * (filesCounted / (filesCounted + chunkSize)),
                        status: "Parsing files..."
                    ))
                }
            }

            let totalFiles = Double(paths.count)
            let components = query.components(separatedBy: " extension:")
            let searchQuery = components[0]
            let fileExtension =
                components.count > 1 ? components[1].trimmingCharacters(in: .whitespaces) : nil

            let chunkCount = concurrentTasks
            let pathChunks = stride(from: 0, to: paths.count, by: max(1, paths.count / chunkCount))
                .map { start in
                    let end = min(start + (paths.count / chunkCount), paths.count)
                    return Array(paths[start..<end])
                }

            var processed = 0

            try await withThrowingTaskGroup(of: [SearchResult].self) { group in
                for chunk in pathChunks {
                    group.addTask {
                        var chunkResults: [SearchResult] = []
                        
                        for path in chunk {
                            try Task.checkCancellation()
                            
                            let fullPath = (directory as NSString).appendingPathComponent(path)
                            var isDirectory: ObjCBool = false
                            
                            if fileManager.fileExists(atPath: fullPath, isDirectory: &isDirectory)
                                && !isDirectory.boolValue
                            {
                                if let ext = fileExtension {
                                    let pathExt = (path as NSString).pathExtension.lowercased()
                                    let searchExt =
                                        ext.hasPrefix(".")
                                        ? String(ext.dropFirst()).lowercased() : ext.lowercased()
                                    
                                    if pathExt != searchExt {
                                        continue
                                    }
                                }
                                
                                if path.localizedCaseInsensitiveContains(searchQuery) {
                                    chunkResults.append(SearchResult(path: fullPath))
                                }
                            }
                        }
                        
                        return chunkResults
                    }
                }
                
                for try await chunkResults in group {
                    results.append(contentsOf: chunkResults)
                    processed += chunkResults.count
                    progress(SearchProgress(
                        progress: 0.5 + 0.5 * (Double(processed) / totalFiles),
                        status: "Searching files..."
                    ))
                }
            }

            progress(SearchProgress(progress: 1.0, status: "Search complete"))
            return results
        }

        currentTask = task
        return try await task.value
    }

    public func openInFinder(path: String) {
        #if os(macOS)
            NSWorkspace.shared.selectFile(path, inFileViewerRootedAtPath: "")
        #elseif os(Linux)
            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/usr/bin/xdg-open")
            process.arguments = [path]
            try? process.run()
        #elseif os(Windows)
            let process = Process()
            process.executableURL = URL(fileURLWithPath: "C:\\Windows\\explorer.exe")
            process.arguments = ["/select,", path]
            try? process.run()
        #else
            print("Unsupported platform")
        #endif
    }
}
