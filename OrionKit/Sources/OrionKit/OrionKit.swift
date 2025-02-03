import Foundation

public struct SearchResult: Identifiable, Hashable {
    public let id = UUID()
    public let path: String

    public init(path: String) {
        self.path = path
    }

    public static func == (lhs: SearchResult, rhs: SearchResult) -> Bool {
        lhs.id == rhs.id
    }
}
