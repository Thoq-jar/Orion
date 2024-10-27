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
    return allocator.dupe(u8, line.items);
}
