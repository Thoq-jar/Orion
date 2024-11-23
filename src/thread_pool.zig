const std = @import("std");
const Thread = std.Thread;
const Mutex = Thread.Mutex;
const Allocator = std.mem.Allocator;

pub const ThreadPool = struct {
    threads: []Thread,
    allocator: Allocator,
    task_mutex: Mutex,
    tasks_available: Thread.Condition,
    tasks_complete: Thread.Condition,
    tasks: std.ArrayList(Task),
    shutdown: bool,

    const Task = struct {
        function: *const fn (args: *anyopaque) void,
        args: *anyopaque,
    };

    pub fn init(allocator: Allocator, thread_count: u32) !ThreadPool {
        var self = ThreadPool{
            .threads = try allocator.alloc(Thread, thread_count),
            .allocator = allocator,
            .task_mutex = .{},
            .tasks_available = .{},
            .tasks_complete = .{},
            .tasks = std.ArrayList(Task).init(allocator),
            .shutdown = false,
        };

        const Context = struct {
            pool: *ThreadPool,
            pub fn run(ctx: @This()) void {
                ctx.pool.workerFn();
            }
        };

        for (self.threads) |*thread| {
            thread.* = try Thread.spawn(.{}, Context.run, .{Context{ .pool = &self }});
        }

        return self;
    }

    pub fn deinit(self: *ThreadPool) void {
        {
            self.task_mutex.lock();
            defer self.task_mutex.unlock();
            self.shutdown = true;
            self.tasks_available.broadcast();
        }

        for (self.threads) |thread| {
            thread.join();
        }

        self.allocator.free(self.threads);
        self.tasks.deinit();
    }

    pub fn submit(self: *ThreadPool, comptime function: anytype, args: anytype) !void {
        const Args = @TypeOf(args);
        const allocator = self.allocator;

        const args_ptr = try allocator.create(Args);
        args_ptr.* = args;

        const task = Task{
            .function = struct {
                fn wrapper(args_void: *anyopaque) void {
                    const args_typed = @as(*Args, @ptrCast(@alignCast(args_void)));
                    @call(.auto, function, .{args_typed.*});
                    allocator.destroy(args_typed);
                }
            }.wrapper,
            .args = args_ptr,
        };

        self.task_mutex.lock();
        defer self.task_mutex.unlock();

        try self.tasks.append(task);
        self.tasks_available.signal();
    }

    fn workerFn(self: *ThreadPool) void {
        while (true) {
            self.task_mutex.lock();
            while (self.tasks.items.len == 0 and !self.shutdown) {
                self.tasks_available.wait(&self.task_mutex);
            }

            if (self.shutdown and self.tasks.items.len == 0) {
                self.task_mutex.unlock();
                break;
            }

            const task = self.tasks.orderedRemove(0);
            self.task_mutex.unlock();

            task.function(task.args);

            self.task_mutex.lock();
            if (self.tasks.items.len == 0) {
                self.tasks_complete.signal();
            }
            self.task_mutex.unlock();
        }
    }

    pub fn wait(self: *ThreadPool) void {
        self.task_mutex.lock();
        defer self.task_mutex.unlock();

        while (self.tasks.items.len > 0) {
            self.tasks_complete.wait(&self.task_mutex);
        }
    }
};
