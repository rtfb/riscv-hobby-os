const assert = @import("std").debug.assert;
const c = @cImport({
    @cInclude("programs.h");
    @cInclude("runflags.h");
    @cInclude("proc.h");
    @cInclude("string.h");
});
const mem = @import("std").mem;
const zstr = @import("zstr.zig");

const userland_programs = [_]c.user_program_t{
    c.user_program_t{
        .entry_point = c.u_main_shell,
        .name = "sh",
    },
    c.user_program_t{
        .entry_point = c.u_main_hello1,
        .name = "hello1",
    },
    c.user_program_t{
        .entry_point = c.u_main_hello2,
        .name = "hello2",
    },
    c.user_program_t{
        .entry_point = c.u_main_sysinfo,
        .name = "sysinfo",
    },
    c.user_program_t{
        .entry_point = c.u_main_fmt,
        .name = "fmt",
    },
    c.user_program_t{
        .entry_point = c.u_main_smoke_test,
        .name = "smoke-test",
    },
    c.user_program_t{
        .entry_point = c.u_main_hanger,
        .name = "hang",
    },
    c.user_program_t{
        .entry_point = c.u_main_ps,
        .name = "ps",
    },
    c.user_program_t{
        .entry_point = c.u_main_cat,
        .name = "cat",
    },
    c.user_program_t{
        .entry_point = c.u_main_coma,
        .name = "coma",
    },
    c.user_program_t{
        .entry_point = c.u_main_pipe,
        .name = "pp",
    },
    c.user_program_t{
        .entry_point = c.u_main_pipe2,
        .name = "pp2",
    },
    c.user_program_t{
        .entry_point = c.u_main_wc,
        .name = "wc",
    },
    c.user_program_t{
        .entry_point = c.u_main_gpio,
        .name = "gpio",
    },
    c.user_program_t{
        .entry_point = c.u_main_iter,
        .name = "iter",
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

pub export fn init_test_processes(runflags: u32) void {
    if (runflags == c.RUNFLAGS_DRY_RUN) {
        return;
    }
    if (runflags == c.RUNFLAGS_SMOKE_TEST) {
        assign_init_program("smoke-test");
    } else {
        assign_init_program("sh");
    }
}

pub fn assign_init_program(name: [*]const u8) void {
    var program: *const c.user_program_t = find_user_program(name) orelse {
        // TODO: panic
        return;
    };
    var p0: *c.process_t = c.alloc_process() orelse {
        // TODO: panic
        return;
    };
    // XXX: reinstate USR_VIRT
    // var status = c.init_proc(p0, c.USR_VIRT(program.entry_point), program.name);
    var fp: *usize = @alignCast(@ptrCast(@constCast(program.entry_point)));
    var ep: c_ulong = @intFromPtr(fp);
    var status = c.init_proc(p0, ep, program.name);
    c.release(&p0.lock);
    if (status != 0) {
        // TODO: panic
    }
    c.cpu.proc = p0;
}
