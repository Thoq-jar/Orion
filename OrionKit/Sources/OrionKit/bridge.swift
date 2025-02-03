import COrionKit
import Foundation

#if canImport(COrionKit)
    import COrionKit
#else
    public struct CSearchResult {
        public let path: UnsafePointer<CChar>
    }

    public struct CSearchResults {
        public let results: UnsafeMutablePointer<CSearchResult>
        public let count: Int32
    }
#endif

class SearchHandle {
    let searcher: FileSearcher
    let task: Task<[SearchResult], Error>
    let semaphore: DispatchSemaphore
    var results: [SearchResult]?

    init(searcher: FileSearcher, task: Task<[SearchResult], Error>, semaphore: DispatchSemaphore) {
        self.searcher = searcher
        self.task = task
        self.semaphore = semaphore
    }
}

@_cdecl("orion_search_files")
public func orion_search_files(
    _ query: UnsafePointer<CChar>,
    _ directory: UnsafePointer<CChar>,
    _ progress_cb: @escaping @convention(c) (Double, UnsafeMutableRawPointer?) -> Void,
    _ user_data: UnsafeMutableRawPointer?
) -> UnsafeMutablePointer<orion_search_results_t>? {
    let searcher = FileSearcher()
    let queryStr = String(cString: query)
    let dirStr = String(cString: directory)

    let semaphore = DispatchSemaphore(value: 0)
    var searchResults: [SearchResult]?

    Task {
        do {
            searchResults = try await searcher.search(query: queryStr, in: dirStr) { progress in
                progress_cb(progress.progress, user_data)
            }
            semaphore.signal()
        } catch {
            semaphore.signal()
        }
    }

    semaphore.wait()

    guard let results = searchResults else {
        let results = UnsafeMutablePointer<orion_search_results_t>.allocate(capacity: 1)
        results.pointee.results = nil
        results.pointee.count = 0
        return results
    }

    let cResults = UnsafeMutablePointer<orion_search_result_t>.allocate(capacity: results.count)
    for (i, result) in results.enumerated() {
        let cPath = strdup(result.path)
        cResults[i] = orion_search_result_t(path: cPath)
    }

    let finalResults = UnsafeMutablePointer<orion_search_results_t>.allocate(capacity: 1)
    finalResults.pointee.results = cResults
    finalResults.pointee.count = Int32(results.count)
    return finalResults
}

@_cdecl("orion_free_search_results")
public func orion_free_search_results(_ results: UnsafeMutablePointer<orion_search_results_t>) {
    for i in 0..<Int(results.pointee.count) {
        free(UnsafeMutablePointer(mutating: results.pointee.results[i].path))
    }
    results.pointee.results.deallocate()
    results.deallocate()
}

@_cdecl("orion_open_in_finder")
public func orion_open_in_finder(_ path: UnsafePointer<CChar>) {
    let fileSearcher = FileSearcher()
    let pathStr = String(cString: path)
    fileSearcher.openInFinder(path: pathStr)
}
