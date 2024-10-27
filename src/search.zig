const std = @import("std");
const fs = std.fs;
const os = std.os;
const mem = std.mem;
const time = std.time;
const Mutex = std.Thread.Mutex;
const ArrayList = std.ArrayList;
const path = std.fs.path;
const stdout = std.io.getStdOut().writer();

const SearchResult = struct {
    path: []const u8,
};

pub const FileSearcher = struct {
    query: []const u8,
    results: ArrayList(SearchResult),
    root_dir: []const u8,
    start_time: i64,
    mutex: Mutex,
    allocator: mem.Allocator,

    pub fn init(allocator: mem.Allocator, query: []const u8, root_dir: []const u8) FileSearcher {
        return FileSearcher{
            .query = query,
            .results = ArrayList(SearchResult).init(allocator),
            .root_dir = root_dir,
            .start_time = time.milliTimestamp(),
            .mutex = Mutex{},
            .allocator = allocator,
        };
    }

    pub fn deinit(self: *FileSearcher) void {
        for (self.results.items) |result| {
            self.allocator.free(result.path);
        }
        self.results.deinit();
    }

    pub fn search(self: *FileSearcher, verbose: bool) !void {
        try stdout.print("\n[Orion] Searching for files...\n", .{});
        try self.searchDir(self.root_dir, verbose);
    }

    fn searchDir(self: *FileSearcher, dir_path: []const u8, verbose: bool) !void {
        var dir = fs.openDirAbsolute(dir_path, .{ .iterate = true }) catch |err| {
            if (err == error.AccessDenied) {
                if (verbose) {
                    try stdout.print("[Orion] Access denied: {s}\n", .{dir_path});
                }
                return;
            }
            return err;
        };
        defer dir.close();

        var iter = dir.iterate();
        while (try iter.next()) |entry| {
            const full_path = try fs.path.join(self.allocator, &[_][]const u8{ dir_path, entry.name });
            defer self.allocator.free(full_path);

            if (verbose) {
                try stdout.print("[VM] Orion Currently processing: {s}\n", .{full_path});
            }

            switch (entry.kind) {
                .file => {
                    if (mem.indexOf(u8, entry.name, self.query) != null) {
                        self.mutex.lock();
                        defer self.mutex.unlock();
                        try self.results.append(SearchResult{ .path = try self.allocator.dupe(u8, full_path) });
                        if (verbose) {
                            try stdout.print("[Orion] Found match: {s}\n", .{full_path});
                        }
                    }
                },
                .directory => self.searchDir(full_path, verbose) catch |err| {
                    if (err != error.AccessDenied) {
                        return err;
                    }
                    if (verbose) {
                        try stdout.print("[Orion] Access denied: {s}\n", .{full_path});
                    }
                },
                else => {},
            }
        }
    }

    pub fn displayResults(self: *FileSearcher) !void {
        self.mutex.lock();
        defer self.mutex.unlock();

        if (self.results.items.len == 0) {
            try stdout.print("[Orion] No files found matching the query.\n", .{});
        } else {
            try stdout.print("[Orion] Done!\n", .{});
            try stdout.print("[Orion] Found {} file(s) matching the query in {s}:\n", .{ self.results.items.len, self.root_dir });
            for (self.results.items) |result| {
                try stdout.print("Orion found: {s}\n", .{result.path});
            }
            try stdout.print("[Orion] Search completed.\n", .{});

            const elapsed_time = time.milliTimestamp() - self.start_time;
            try stdout.print("[Orion] Elapsed time: {}ms\n", .{elapsed_time});
        }
    }
};
