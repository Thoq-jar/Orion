const std = @import("std");
const FileSearcher = @import("search.zig").FileSearcher;
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

    if (utils.contains(args[1..], "--verbose")) {
        verbose = true;
        try stdout.print("{s}", .{banner});
        try stdout.print("[Debug] Orion running in Verbose mode!\n", .{});
    } else if (utils.contains(args[1..], "--help")) {
        try stdout.print("[Orion] Usage: orion [--verbose] (is the only option available)\n", .{});
        return;
    } else {
        try stdout.print("{s}", .{banner});
    }

    try stdout.print("[Orion] Welcome to Orion, the file search engine.\n", .{});
    try stdout.print("[Orion] Enter search scope: ", .{});
    const root_dir = try utils.readLine(allocator);
    defer allocator.free(root_dir);

    try stdout.print("[Orion] Enter search query: ", .{});
    const query = try utils.readLine(allocator);
    defer allocator.free(query);

    var file_searcher = FileSearcher.init(allocator, query, root_dir);
    defer file_searcher.deinit();

    try file_searcher.search(verbose);
    try file_searcher.displayResults();
}
