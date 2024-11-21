const std = @import("std");
const FileSearcher = @import("search.zig").FileSearcher;
const SizeScanner = @import("size_scanner.zig").SizeScanner;
const utils = @import("utils.zig");
const io = std.io;
const stdout = io.getStdOut().writer();

pub fn main() !void {
    const banner =
        \\  ██████╗ ██████╗ ██╗ ██████╗ ███╗   ██╗
        \\ ██╔═══██╗██╔══██╗██║██╔═══██╗████╗  ██║
        \\ ██║   ██║██████╔╝██║██║   ██║██╔██╗ ██║
        \\ ██║   ██║██╔══██╗██║██║   ██║██║╚██╗██║
        \\ ╚██████╔╝██║  ██║██║╚██████╔╝██║ ╚████║
        \\  ╚═════╝ ╚═╝  ╚═╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝
        \\
    ;

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    var verbose = false;
    var thread_count: u32 = 1;

    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.eql(u8, arg, "--verbose")) {
            verbose = true;
        } else if (std.mem.startsWith(u8, arg, "--threads=")) {
            if (std.mem.indexOf(u8, arg, "=")) |index| {
                const thread_str = arg[index + 1 ..];
                thread_count = try std.fmt.parseInt(u32, thread_str, 10);
            } else {
                try stdout.print("[Orion] Error: Invalid thread count format\n", .{});
                return;
            }
        } else if (std.mem.eql(u8, arg, "--threads")) {
            i += 1;
            if (i >= args.len) {
                try stdout.print("[Orion] Error: --threads requires a number\n", .{});
                return;
            }
            thread_count = try std.fmt.parseInt(u32, args[i], 10);
        } else if (std.mem.eql(u8, arg, "--help")) {
            try stdout.print("[Orion] Usage: orion [--verbose] [--threads=N]\n", .{});
            try stdout.print("  --verbose: Enable verbose output\n", .{});
            try stdout.print("  --threads N: Number of threads to use (default: 1)\n", .{});
            try stdout.print("  --threads=N: Alternative syntax for thread count\n", .{});
            return;
        }
    }

    if (verbose) {
        try stdout.print("{s}", .{banner});
        try stdout.print("[VM] Orion running in Verbose mode!\n", .{});
        try stdout.print("[VM] Using {} thread(s)\n", .{thread_count});
    } else if (utils.contains(args[1..], "--help")) {
        try stdout.print("[Orion] Usage: orion [--verbose] [--threads=N]\n", .{});
        try stdout.print("  --verbose: Enable verbose output\n", .{});
        try stdout.print("  --threads N: Number of threads to use (default: 1)\n", .{});
        try stdout.print("  --threads=N: Alternative syntax for thread count\n", .{});
        return;
    } else {
        try stdout.print("{s}", .{banner});
    }

    try stdout.print("[Orion] Welcome to Orion, the file search engine.\n", .{});
    try stdout.print("\nSelect operation:\n", .{});
    try stdout.print("1. Search for files by name\n", .{});
    try stdout.print("2. Find large files\n", .{});
    try stdout.print("Choice (1/2): ", .{});

    const choice = try utils.readLine(allocator);
    defer allocator.free(choice);

    try stdout.print("[Orion] Enter search scope: ", .{});
    const root_dir = try utils.readLine(allocator);
    defer allocator.free(root_dir);

    if (std.mem.eql(u8, choice, "1")) {
        try stdout.print("[Orion] Enter search query: ", .{});
        const query = try utils.readLine(allocator);
        defer allocator.free(query);

        var file_searcher = FileSearcher.init(allocator, query, root_dir);
        defer file_searcher.deinit();

        try file_searcher.search(verbose);
        try file_searcher.displayResults();
    } else if (std.mem.eql(u8, choice, "2")) {
        var size_scanner = try SizeScanner.init(allocator, root_dir, thread_count);
        defer size_scanner.deinit();

        try size_scanner.scan(verbose);
        try size_scanner.displayResults();
    } else {
        try stdout.print("[Orion] Invalid choice. Please select 1 or 2.\n", .{});
        return;
    }
}
