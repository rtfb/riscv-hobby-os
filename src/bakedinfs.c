#include "bakedinfs.h"
#include "errno.h"
#include "mem.h"
#include "programs.h"
#include "string.h"

bifs_directory_t *bifs_root;
bifs_directory_t bifs_all_directories[BIFS_MAX_FILES];
bifs_file_t      bifs_all_files[BIFS_MAX_FILES];

void bifs_init() {
    memset(bifs_all_directories, sizeof(bifs_all_directories), 0);
    memset(bifs_all_files, sizeof(bifs_all_files), 0);
    bifs_root = bifs_allocate_dir();
    bifs_root->name = "/";

    bifs_directory_t *procfs = bifs_allocate_dir();
    procfs->name = "proc";
    procfs->parent = bifs_root;

    bifs_directory_t *home = bifs_allocate_dir();
    home->name = "home";
    home->parent = bifs_root;

    bifs_directory_t *bin = bifs_allocate_dir();
    bin->name = "bin";
    bin->parent = bifs_root;
    bin->lsdir = bin_lsdir;

    bifs_file_t *f0 = &bifs_all_files[0];
    f0->flags = BIFS_READABLE | BIFS_RAW;
    f0->parent = bifs_root;
    f0->name = "readme.txt";
    f0->data = "Hello, I am File Zero.\n";

    bifs_file_t *f1 = &bifs_all_files[1];
    f1->flags = BIFS_READABLE | BIFS_RAW;
    f1->parent = bifs_root;
    f1->name = "file";
    f1->data = "A very short text.\n";

    bifs_file_t *f2 = &bifs_all_files[2];
    f2->flags = BIFS_READABLE | BIFS_RAW;
    f2->parent = home;
    f2->name = "read.me";
    f2->data = "This Is File One. File Zero is at root.\n";

    bifs_file_t *st = &bifs_all_files[3];
    st->flags = BIFS_READABLE | BIFS_RAW;
    st->parent = home;
    st->name = "smoke-test.sh";
    st->data = "sysinfo\n\
fmt\n\
hang\n\
sysinfo\n\
ps\n\
cat /readme.txt | wc\n\
iter 300 | wc\n\
testprintf\n";

    bifs_file_t *dt = &bifs_all_files[4];
    dt->flags = BIFS_READABLE | BIFS_RAW;
    dt->parent = home;
    dt->name = "daemon-test.sh";
    dt->data = "fibd\n\
ps\n\
wait 1 3\n\
fib 1\n\
wait 1 1\n\
fib 1\n\
cat /proc/1/stats\n\
sysinfo\n";

    bifs_file_t *lt = &bifs_all_files[5];
    lt->flags = BIFS_READABLE | BIFS_RAW;
    lt->parent = home;
    lt->name = "ls-test.sh";
    lt->data = "ls\n\
ls /home\n\
ls /home/\n\
ls /bin\n\
ls /proc\n\
cat -n /proc/0/name";
}

bifs_directory_t* bifs_allocate_dir() {
    for (int i = 0; i < BIFS_MAX_FILES; i++) {
        bifs_directory_t *d = &bifs_all_directories[i];
        if (d->flags == 0) {
            d->flags = BIFS_READABLE;
            d->lsdir = bifs_lsdir;
            return d;
        }
    }
    return 0;
}

bifs_file_t* bifs_allocate_file() {
    for (int i = 0; i < BIFS_MAX_FILES; i++) {
        bifs_file_t *f = &bifs_all_files[i];
        if (f->flags == 0) {
            f->flags = BIFS_READABLE;
            return f;
        }
    }
    return 0;
}

int32_t bifs_opendirpath(bifs_directory_t **dir, char const *path, int end) {
    int prev_slash_pos = 0;
    int slash_pos = next_slash(path, prev_slash_pos + 1);
    *dir = bifs_root;
    while (path[slash_pos] && slash_pos < end) {
        *dir = bifs_opendir(*dir, path, prev_slash_pos + 1, slash_pos);
        if (!*dir) {
            return -ENOENT;
        }
        prev_slash_pos = slash_pos;
        slash_pos = next_slash(path, slash_pos + 1);
    }
    if (path[slash_pos-1] != '/' && slash_pos <= end) {
        *dir = bifs_opendir(*dir, path, prev_slash_pos + 1, slash_pos);
        if (!*dir) {
            return -ENOENT;
        }
    }
    return 0;
}

int32_t bifs_open(char const *filepath, uint32_t flags, bifs_file_t **ppf) {
    if (*filepath != '/') {
        return -ENOSYS;
    }
    int last_slash_pos = 0;
    int zero_pos = 0;
    while (filepath[zero_pos] != '\0') {
        last_slash_pos = zero_pos;
        zero_pos = next_slash(filepath, last_slash_pos + 1);
    }
    bifs_directory_t *dir;
    int32_t status = bifs_opendirpath(&dir, filepath, last_slash_pos);
    if (status != 0) {
        return status;
    }
    return bifs_openfile(dir, filepath, last_slash_pos + 1, zero_pos, ppf);
}

int32_t bifs_read(file_t *f, uint32_t pos, void *buf, uint32_t size) {
    bifs_file_t *ff = (bifs_file_t*)f->fs_file;
    char *cbuf = (char*)buf;
    int i = 0;
    while (i < size) {
        char ch = ff->data[pos];
        if (!ch) {
            break;
        }
        cbuf[i] = ch;
        i++;
        pos++;
    }
    return i;
}

int32_t bifs_write(file_t *f, uint32_t pos, void *buf, uint32_t nbytes) {
    return -ENOSYS; // TODO: implement
}

int next_slash(char const *path, int pos) {
    char ch = path[pos];
    while (ch && ch != '/') {
        pos++;
        ch = path[pos];
    }
    return pos;
}

bifs_directory_t* bifs_opendir(bifs_directory_t *parent, char const *name, int start, int end) {
    if (parent == 0) {
        parent = &bifs_all_directories[0]; // assume root
    }
    for (int i = 1; i < BIFS_MAX_FILES; i++) {
        bifs_directory_t *d = &bifs_all_directories[i];
        if (d->parent != parent) {
            continue;
        }
        if (!strncmp(d->name, name+start, end-start)) {
            return d;
        }
    }
    return 0;
}

int32_t bifs_openfile(bifs_directory_t *parent, char const *name, int start, int end, bifs_file_t **ppf) {
    if (parent == 0) {
        parent = &bifs_all_directories[0]; // assume root
    }
    for (int i = 0; i < BIFS_MAX_FILES; i++) {
        bifs_file_t *f = &bifs_all_files[i];
        if (f->parent != parent) {
            continue;
        }
        if (!strncmp(f->name, name+start, end-start)) {
            *ppf = f;
            return f->flags;
        }
    }
    return -ENOENT;
}

int32_t bifs_lsdir(bifs_directory_t *dir, dirent_t *dirents, regsize_t size) {
    int de_index = 0;
    for (int i = 0; i < BIFS_MAX_FILES; i++) {
        if (de_index >= size) {
            return -ENOBUFS;
        }
        bifs_directory_t *d = &bifs_all_directories[i];
        if (d->parent != dir) {
            continue;
        }
        dirent_t *de = &dirents[de_index];
        de->flags = DIRENT_DIRECTORY;
        strncpy(de->name, d->name, MAX_FILENAME_LEN);
        de_index++;
    }
    for (int i = 0; i < BIFS_MAX_FILES; i++) {
        if (de_index >= size) {
            return -ENOBUFS;
        }
        bifs_file_t *f = &bifs_all_files[i];
        if (f->parent != dir) {
            continue;
        }
        dirent_t *de = &dirents[de_index];
        de->flags = 0;
        strncpy(de->name, f->name, MAX_FILENAME_LEN);
        de_index++;
    }
    return de_index;
}

int32_t bin_lsdir(bifs_directory_t *dir, dirent_t *dirents, regsize_t size) {
    int de_index = 0;
    for (int i = 0; i < MAX_USERLAND_PROGS; i++) {
        if (de_index >= size) {
            return -ENOBUFS;
        }
        user_program_t *p = &userland_programs[i];
        if (!p->entry_point) {
            continue;
        }
        dirent_t *de = &dirents[de_index];
        de->flags = DIRENT_EXECUTABLE;
        strncpy(de->name, p->name, MAX_FILENAME_LEN);
        de_index++;
    }
    return de_index;
}

bifs_directory_t *bifs_mkdir(char const *parent_path, char const *name) {
    bifs_directory_t *parent;
    int32_t status = bifs_opendirpath(&parent, parent_path, kstrlen(parent_path));
    if (status != 0) {
        return 0;
    }
    bifs_directory_t *dir = bifs_allocate_dir();
    if (!dir) {
        return 0;
    }
    dir->name = (char*)name;
    dir->parent = parent;
    return dir;
}
