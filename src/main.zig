const std = @import("std");
const FileSearcher = @import("search.zig").FileSearcher;
const SizeScanner = @import("size_scanner.zig").SizeScanner;
const utils = @import("utils.zig");
const io = std.io;

const banner = @embedFile("assets/ascii_logo.txt");

pub fn main() !void {
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
                if (std.fmt.parseInt(u8, thread_str, 10)) |num| {
                    thread_count = num;
                } else |_| {
                    utils.print("[Orion] Error: Invalid thread count format\n", .{});
                    return;
                }
            } else {
                utils.print("[Orion] Error: Invalid thread count format\n", .{});
                return;
            }
        } else if (std.mem.eql(u8, arg, "--threads")) {
            i += 1;
            if (i >= args.len) {
                utils.print("[Orion] Error: --threads requires a number\n", .{});
                return;
            }
            thread_count = try std.fmt.parseInt(u32, args[i], 10);
        } else if (std.mem.eql(u8, arg, "--help")) {
            utils.print("[Orion] Usage: orion [--verbose] [--threads=N]\n", .{});
            utils.print("  --verbose: Enable verbose output\n", .{});
            utils.print("  --threads N: Number of threads to use (default: 1)\n", .{});
            utils.print("  --threads=N: Alternative syntax for thread count\n", .{});
            return;
        }
    }

    if (verbose) {
        utils.print("{s}\n", .{banner});
        utils.print("[VM] Orion running in Verbose mode!\n", .{});
        utils.print("[VM] Using {} thread(s)\n", .{thread_count});
    } else if (utils.contains(args[1..], "--help")) {
        utils.print("[Orion] Usage: orion [--verbose] [--threads=N]\n", .{});
        utils.print("  --verbose: Enable verbose output\n", .{});
        utils.print("  --threads N: Number of threads to use (default: 1)\n", .{});
        utils.print("  --threads=N: Alternative syntax for thread count\n", .{});
        return;
    } else {
        utils.print("{s}", .{banner});
    }

    utils.print("[Orion] Welcome to Orion, the file search engine.\n", .{});
    utils.print("\nSelect operation:\n", .{});
    utils.print("1. Search for files by name\n", .{});
    utils.print("2. Find large files\n", .{});
    utils.print("Choice (1/2): ", .{});

    const choice = try utils.readLine(allocator);
    defer allocator.free(choice);

    utils.print("[Orion] Enter search scope: ", .{});
    const root_dir = try utils.readLine(allocator);
    defer allocator.free(root_dir);

    if (std.mem.eql(u8, choice, "1")) {
        utils.print("[Orion] Enter search query: ", .{});
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
        utils.print("[Orion] Invalid choice. Please select 1 or 2.\n", .{});
    }
}
