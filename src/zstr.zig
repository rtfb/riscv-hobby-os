pub export fn kstrlen(s: [*c]const u8) callconv(.C) usize {
    var i: usize = 0;
    while (s[i] != 0) {
        i += 1;
    }
    return i;
}
