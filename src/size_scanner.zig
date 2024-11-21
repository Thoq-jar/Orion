const std = @import("std");
const fs = std.fs;
const mem = std.mem;
const Mutex = std.Thread.Mutex;
const ArrayList = std.ArrayList;
const stdout = std.io.getStdOut().writer();
const ThreadPool = @import("thread_pool.zig").ThreadPool;

const FileInfo = struct {
    path: []const u8,
    size: u64,
};

pub const SizeScanner = struct {
    results: ArrayList(FileInfo),
    root_dir: []const u8,
    mutex: Mutex,
    allocator: mem.Allocator,
    thread_pool: ?*ThreadPool,
    thread_count: u32,

    pub fn init(allocator: mem.Allocator, root_dir: []const u8, thread_count: u32) !SizeScanner {
        const scanner = SizeScanner{
            .results = ArrayList(FileInfo).init(allocator),
            .root_dir = root_dir,
            .mutex = .{},
            .allocator = allocator,
            .thread_pool = if (thread_count > 1)
                try allocator.create(ThreadPool)
            else
                null,
            .thread_count = thread_count,
        };

        if (scanner.thread_pool) |pool| {
            pool.* = try ThreadPool.init(allocator, thread_count);
        }

        return scanner;
    }

    pub fn deinit(self: *SizeScanner) void {
        if (self.thread_pool) |pool| {
            pool.deinit();
            self.allocator.destroy(pool);
        }
        for (self.results.items) |result| {
            self.allocator.free(result.path);
        }
        self.results.deinit();
    }

    pub fn scan(self: *SizeScanner, verbose: bool) !void {
        try stdout.print("\n[Orion] Scanning directory for large files...\n", .{});
        try self.scanDir(self.root_dir, verbose);
        try self.sortResults();
    }

    fn scanDir(self: *SizeScanner, dir_path: []const u8, verbose: bool) !void {
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
                try stdout.print("[VM] Orion Currently scanning: {s}\n", .{full_path});
            }

            switch (entry.kind) {
                .file => {
                    const file = fs.openFileAbsolute(full_path, .{}) catch |err| {
                        if (err == error.AccessDenied) {
                            if (verbose) {
                                try stdout.print("[Orion] Access denied: {s}\n", .{full_path});
                            }
                            continue;
                        }
                        return err;
                    };
                    defer file.close();

                    const stat = file.stat() catch |err| {
                        if (err == error.AccessDenied) {
                            if (verbose) {
                                try stdout.print("[Orion] Access denied: {s}\n", .{full_path});
                            }
                            continue;
                        }
                        return err;
                    };

                    self.mutex.lock();
                    defer self.mutex.unlock();
                    try self.results.append(FileInfo{
                        .path = try self.allocator.dupe(u8, full_path),
                        .size = stat.size,
                    });
                },
                .directory => self.scanDir(full_path, verbose) catch |err| {
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

    fn sortResults(self: *SizeScanner) !void {
        self.mutex.lock();
        defer self.mutex.unlock();

        const Context = struct {
            pub fn lessThan(_: @This(), a: FileInfo, b: FileInfo) bool {
                return a.size > b.size;
            }
        };

        std.mem.sort(FileInfo, self.results.items, Context{}, Context.lessThan);
    }

    fn formatSize(size: u64) struct { value: f64, unit: []const u8 } {
        const units = [_][]const u8{ "B", "KB", "MB", "GB", "TB" };
        var value: f64 = @floatFromInt(size);
        var unit_index: usize = 0;

        while (value >= 1024 and unit_index < units.len - 1) {
            value /= 1024;
            unit_index += 1;
        }

        return .{ .value = value, .unit = units[unit_index] };
    }

    pub fn displayResults(self: *SizeScanner) !void {
        self.mutex.lock();
        defer self.mutex.unlock();

        if (self.results.items.len == 0) {
            try stdout.print("[Orion] No files found.\n", .{});
            return;
        }

        try stdout.print("[Orion] Found {} files. Largest files:\n", .{self.results.items.len});

        const display_count = @min(100, self.results.items.len);
        for (self.results.items[0..display_count]) |result| {
            const formatted = formatSize(result.size);
            try stdout.print("[Orion] {d:.2}{s}: {s}\n", .{ formatted.value, formatted.unit, result.path });
        }
    }
};
