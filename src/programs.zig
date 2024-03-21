const assert = @import("std").debug.assert;
const c = @cImport({
    @cInclude("machine/qemu_u.h");
    @cInclude("kernel.h");
    @cInclude("programs.h");
    @cInclude("runflags.h");
    @cInclude("proc.h");
    @cInclude("string.h");
});
const mem = @import("std").mem;
const zstr = @import("zstr.zig");

fn fp(p: *const fn (...) callconv(.C) c_int) ?*anyopaque {
    return @ptrCast(@constCast(p));
}

const userland_programs = [_]c.user_program_t{
    c.user_program_t{
        .entry_point = fp(&c.u_main_shell),
        .name = "sh",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_hello),
        .name = "hello",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_sysinfo),
        .name = "sysinfo",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_fmt),
        .name = "fmt",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_hanger),
        .name = "hang",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_ps),
        .name = "ps",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_cat),
        .name = "cat",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_coma),
        .name = "coma",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_wc),
        .name = "wc",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_gpio),
        .name = "gpio",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_iter),
        .name = "iter",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_test_printf),
        .name = "testprintf",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_fibd),
        .name = "fibd",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_fib),
        .name = "fib",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_wait),
        .name = "wait",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_ls),
        .name = "ls",
    },
    c.user_program_t{
        .entry_point = fp(&c.u_main_clock),
        .name = "clock",
    },
};

comptime {
    assert(userland_programs.len < c.MAX_USERLAND_PROGS);
}

pub export fn find_user_program(name: [*]const u8) callconv(.C) ?*const c.user_program_t {
    var len: c_uint = @truncate(zstr.kstrlen(name));
    for (&userland_programs) |*prog| {
        if (c.strncmp(prog.name, name, len) == 0) {
            return prog;
        }
    }
    return null;
}

pub export fn get_user_program(index: usize) callconv(.C) ?*const c.user_program_t {
    _ = index;
    return null;
    // if (index >= userland_programs.len) {
    //     return null;
    // }
    // return &userland_programs[index];
}

pub export fn init_test_processes(runflags: u32) void {
    if (runflags == c.RUNFLAGS_DRY_RUN) {
        return;
    }
    if (runflags == c.RUNFLAGS_SMOKE_TEST or runflags == c.RUNFLAGS_SMOKE_TEST) {
        assign_init_program("sh", c.test_script);
    } else {
        assign_init_program("sh", null);
    }
}

pub fn assign_init_program(name: [*]const u8, opt_script: ?[*:0]const u8) void {
    var program: *const c.user_program_t = find_user_program(name) orelse {
        c.panic("no init program");
        return;
    };
    var p0: *c.process_t = c.alloc_process() orelse {
        c.panic("alloc p0 process");
        return;
    };
    var ep: c_ulong = @intFromPtr(program.entry_point);
    var status = c.init_proc(p0, c.ZUSR_VIRT(ep), program.name);
    if (opt_script) |script| {
        var args = [_][*:0]const u8{ program.name, "-f", script };
        c.inject_argv(p0, 3, @ptrCast(&args[0]));
    }
    c.release(&p0.lock);
    if (status != 0) {
        c.panic("init p0 process");
    }
    c.cpu.proc = p0;
}
