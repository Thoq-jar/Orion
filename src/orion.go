package orion

import (
	"bufio"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

type SearchResult struct {
	Path string
}

type FileSearcher struct {
	Query     string
	Results   []SearchResult
	RootDir   string
	StartTime time.Time
	mu        sync.Mutex
}

func NewFileSearcher(query, rootDir string) *FileSearcher {
	return &FileSearcher{
		Query:     query,
		Results:   []SearchResult{},
		RootDir:   rootDir,
		StartTime: time.Now(),
	}
}

func (fs *FileSearcher) Search(verbose bool) {
	fmt.Println("\n[Orion] Searching for files...")

	var wg sync.WaitGroup
	err := filepath.Walk(fs.RootDir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			fmt.Printf("[Orion] Error: %s | Continuing...\n", err)
			return nil
		}

		if verbose {
			fmt.Printf("[VM] Orion Currently processing: %s\n", path)
		}

		if info.IsDir() {
			return nil
		}

		if strings.Contains(info.Name(), fs.Query) {
			fs.mu.Lock()
			fs.Results = append(fs.Results, SearchResult{Path: path})
			fs.mu.Unlock()
			if verbose {
				fmt.Printf("[Orion] Found match: %s\n", path)
			}
		}
		return nil
	})

	if err != nil {
		fmt.Printf("[Orion] Error walking the path: %s\n", err)
	}

	wg.Wait()
}

func (fs *FileSearcher) DisplayResults() {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	if len(fs.Results) == 0 {
		fmt.Println("[Orion] No files found matching the query.")
	} else {
		fmt.Println("[Orion] Done!")
		fmt.Printf("[Orion] Found %d file(s) matching the query in %s:\n", len(fs.Results), fs.RootDir)
		for _, result := range fs.Results {
			fmt.Printf("Orion found: %s\n", result.Path)
		}
		fmt.Println("[Orion] Search completed.")

		elapsedTime := time.Since(fs.StartTime)
		fmt.Printf("[Orion] Elapsed time: %v\n", elapsedTime)
	}
}

func Run() {
	banner := `
 ██████╗ ██████╗ ██╗ ██████╗ ███╗   ██╗
██╔═══██╗██╔══██╗██║██╔═══██╗████╗  ██║
██║   ██║██████╔╝██║██║   ██║██╔██╗ ██║
██║   ██║██╔══██╗██║██║   ██║██║╚██╗██║
╚██████╔╝██║  ██║██║╚██████╔╝██║ ╚████║
 ╚═════╝ ╚═╝  ╚═╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝
`
	args := os.Args[1:]
	verbose := false

	if contains(args, "--verbose") {
		verbose = true
		fmt.Print(banner)
		fmt.Println("[Debug] Orion running in Verbose mode!")
	} else if contains(args, "--help") {
		fmt.Println("[Orion] Usage: orion [--verbose] (is the only option available)")
		return
	} else {
		fmt.Print(banner)
	}
	fmt.Println("[Orion] Welcome to Orion, the file search engine.")
	fmt.Print("[Orion] Enter search scope: ")
	reader := bufio.NewReader(os.Stdin)
	rootDir, _ := reader.ReadString('\n')
	rootDir = strings.TrimSpace(rootDir)

	fmt.Print("[Orion] Enter search query: ")
	query, _ := reader.ReadString('\n')
	query = strings.TrimSpace(query)

	fileSearcher := NewFileSearcher(query, rootDir)
	fileSearcher.Search(verbose)
	fileSearcher.DisplayResults()
}

func contains(slice []string, item string) bool {
	for _, a := range slice {
		if a == item {
			return true
		}
	}
	return false
}
