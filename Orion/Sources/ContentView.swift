//
//  ContentView.swift
//  Orion
//
//  Created by Tristan Shaw on 2/2/25.
//

import SwiftUI
import OrionKit

struct ContentView: View {
    @State private var searchQuery = ""
    @State private var searchPath = FileManager.default.homeDirectoryForCurrentUser.path
    @State private var searchResults: [SearchResult] = []
    @State private var searchProgress: Double = 0
    @State private var isSearching = false
    @State private var errorMessage: String?
    @State private var searchTask: Task<Void, Never>?
    @State private var selectedResult: SearchResult?
    
    private let searcher = FileSearcher()
    
    var body: some View {
        VStack(spacing: 20) {
            VStack(spacing: 16) {
                HStack {
                    TextField("Search query", text: $searchQuery)
                        .textFieldStyle(.roundedBorder)
                    
                    Button("Browse") {
                        let panel = NSOpenPanel()
                        panel.canChooseFiles = false
                        panel.canChooseDirectories = true
                        panel.allowsMultipleSelection = false
                        
                        if panel.runModal() == .OK {
                            searchPath = panel.url?.path ?? searchPath
                        }
                    }
                }
                
                TextField("Search path", text: $searchPath)
                    .textFieldStyle(.roundedBorder)
            }
            .padding(.horizontal)
            .padding(.top, 20)
            
            if isSearching {
                ProgressView(value: searchProgress) {
                    Text("Searching... \(Int(searchProgress * 100))%")
                }
                .progressViewStyle(.linear)
                .padding(.horizontal)
            }
            
            if let error = errorMessage {
                Text(error)
                    .foregroundColor(.red)
                    .padding()
            }
            
            List(searchResults, selection: $selectedResult) { result in
                Text(result.path)
                    .foregroundColor(isSelected(result: result) ? Color.white : Color.primary)
                    .padding(4)
                    .background(isSelected(result: result) ? Color.accentColor : Color.clear)
                    .cornerRadius(4)
                    .onTapGesture(count: 2) {
                        searcher.openInFinder(path: result.path)
                    }
                    .onTapGesture(count: 1) {
                        if selectedResult?.id == result.id {
                            selectedResult = nil
                        } else {
                            selectedResult = result
                        }
                    }
            }
            .listStyle(.inset)
            
            HStack {
                Button("Search") {
                    startNewSearch()
                }
                .disabled(isSearching || searchQuery.isEmpty)
                
                if isSearching {
                    Button("Cancel") {
                        cancelSearch()
                    }
                }
                
                Spacer()
                
                if let selected = selectedResult {
                    Button("Open in Finder") {
                        searcher.openInFinder(path: selected.path)
                    }
                }
            }
            .padding(.horizontal)
            .padding(.bottom)
        }
        .frame(minWidth: 400, minHeight: 300)
    }
    
    private func isSelected(result: SearchResult) -> Bool {
        selectedResult?.id == result.id
    }
    
    private func startNewSearch() {
        searchTask?.cancel()
        selectedResult = nil
        searchTask = Task {
            isSearching = true
            searchProgress = 0
            errorMessage = nil
            searchResults = []
            
            do {
                searchResults = try await searcher.search(
                    query: searchQuery,
                    in: searchPath
                ) { progress in
                    searchProgress = progress
                }
            } catch {
                if !Task.isCancelled {
                    errorMessage = error.localizedDescription
                }
            }
            
            if !Task.isCancelled {
                isSearching = false
            }
        }
    }
    
    private func cancelSearch() {
        searcher.cancelSearch()
        searchTask?.cancel()
        searchTask = nil
        isSearching = false
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
