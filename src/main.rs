use std::io::{self, Write};
use std::sync::{Arc, Mutex};
use std::time::Instant;

#[derive(Debug)]
struct SearchResult {
    path: String,
}

struct FileSearcher {
    query: String,
    results: Arc<Mutex<Vec<SearchResult>>>,
    root_dir: String,
    start_time: Instant,
}

impl FileSearcher {
    fn new(query: String, root_dir: String) -> Self {
        FileSearcher {
            query,
            results: Arc::new(Mutex::new(Vec::new())),
            root_dir,
            start_time: Instant::now(),
        }
    }

    fn search(&self, root_dir: &str) {
        println!("\n[Orion] Searching for files...");

        let results = Arc::clone(&self.results);
        let query = self.query.clone();

        let walker = walkdir::WalkDir::new(root_dir).into_iter();
        for entry in walker {
            match entry {
                Ok(entry) => {
                    if entry.file_type().is_file() {
                        let filename = entry.file_name().to_string_lossy().to_string();
                        if filename.contains(&query) {
                            let mut results = results.lock().unwrap();
                            results.push(SearchResult {
                                path: entry.path().display().to_string(),
                            });
                        }
                    }
                }
                Err(err) => {
                    println!("[Orion] Error: {} | Continuing...", err);
                }
            }
        }
    }

    fn display_results(&self) {
        let results = self.results.lock().unwrap();

        if results.is_empty() {
            println!("[Orion] No files found matching the query.");
        } else {
            println!("Done!");
            println!(
                "[Orion] Found {} file(s) matching the query in {}:",
                results.len(),
                self.root_dir
            );
            for result in results.iter() {
                println!("Orion found: {}", result.path);
            }
            println!("[Orion] Search completed.");

            let elapsed_time = self.start_time.elapsed();
            println!("[Orion] Elapsed time: {:?}", elapsed_time);
        }

        main();
    }
}

fn main() {
    let banner = r#"
 ██████╗ ██████╗ ██╗ ██████╗ ███╗   ██╗
██╔═══██╗██╔══██╗██║██╔═══██╗████╗  ██║
██║   ██║██████╔╝██║██║   ██║██╔██╗ ██║
██║   ██║██╔══██╗██║██║   ██║██║╚██╗██║
╚██████╔╝██║  ██║██║╚██████╔╝██║ ╚████║
 ╚═════╝ ╚═╝  ╚═╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝
"#;
    println!("{}", banner);
    println!("[Orion] Welcome to Orion, the file search engine.");

    print!("[Orion] Enter search scope: ");
    io::stdout().flush().unwrap();
    let mut root_dir = String::new();
    io::stdin().read_line(&mut root_dir).unwrap();
    let root_dir = root_dir.trim().to_string();

    print!("[Orion] Enter search query: ");
    io::stdout().flush().unwrap();
    let mut query = String::new();
    io::stdin().read_line(&mut query).unwrap();
    let query = query.trim().to_string();

    let file_searcher = FileSearcher::new(query.to_string(), root_dir.clone());
    file_searcher.search(&root_dir);
    file_searcher.display_results();
}