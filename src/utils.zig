const std = @import("std");
const mem = std.mem;
const ArrayList = std.ArrayList;

pub fn contains(slice: []const []const u8, item: []const u8) bool {
    for (slice) |element| {
        if (mem.eql(u8, element, item)) {
            return true;
        }
    }
    return false;
}

pub fn readLine(allocator: mem.Allocator) ![]const u8 {
    var line = ArrayList(u8).init(allocator);
    defer line.deinit();
    try std.io.getStdIn().reader().streamUntilDelimiter(line.writer(), '\n', null);
    const trimmed = std.mem.trim(u8, line.items, &[_]u8{ ' ', '\r', '\n' });
    return allocator.dupe(u8, trimmed);
}

pub fn print(comptime format: []const u8, args: anytype) void {
    if (@import("builtin").os.tag == .windows) {
        std.debug.print(format, args);
    } else {
        const stdout = std.io.getStdOut().writer();
        stdout.print(format, args) catch return;
    }
}
