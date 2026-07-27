#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <stdint.h>
#ifdef BFETCH_ENABLE_BTRFS_PACMAN
#include <sys/ioctl.h>
#include <sys/vfs.h>
#include <linux/capability.h>
#include <linux/magic.h>
#include <linux/btrfs.h>
#include <linux/btrfs_tree.h>
#endif
#if defined(__linux__) && defined(SYS_getdents64)
/* linux/dirent.h is not always installed with libc-dev; match kernel layout. */
struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};
#endif
#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define HOT         __attribute__((hot))
#define INLINE      static inline __attribute__((always_inline))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#define HOT
#define INLINE static inline
#endif

/* Nix manifest needles; lengths known at compile time (no per-call strstr/strlen). */
#define NIX_JSON_NEEDLE "\"active\":true"
#define NIX_JSON_NLEN   (sizeof(NIX_JSON_NEEDLE) - 1)
#define NIX_NIX_NEEDLE  "name = \""
#define NIX_NIX_NLEN    (sizeof(NIX_NIX_NEEDLE) - 1)

// Buffer sizes optimized for performance
#define BUFFER_SIZE 65536
#define SMALL_BUFFER 256
#define DENTS_BUFFER 131072
#define PACMAN_DENTS_BUFFER 98304
#ifdef BFETCH_ENABLE_BTRFS_PACMAN
#define BTRFS_SEARCH_BUFFER 262144
#endif
#define UEVENT_BUFFER 4096
#define CPUINFO_BUFFER 16384

// Nord colors as ANSI escape codes for maximum speed
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define NORD0 "\033[30m"    // #2E3440
#define NORD1 "\033[90m"    // #3B4252
#define NORD2 "\033[37m"    // #434C5E
#define NORD3 "\033[97m"    // #4C566A
#define NORD4 "\033[97m"    // #D8DEE9
#define NORD5 "\033[37m"    // #E5E9F0
#define NORD6 "\033[97m"    // #ECEFF4
#define NORD7 "\033[36m"    // #8FBCBB
#define NORD8 "\033[96m"    // #88C0D0
#define NORD9 "\033[34m"    // #81A1C1
#define NORD10 "\033[94m"   // #5E81AC
#define NORD11 "\033[91m"   // #BF616A
#define NORD12 "\033[93m"   // #D08770
#define NORD13 "\033[33m"   // #EBCB8B
#define NORD14 "\033[32m"   // #A3BE8C
#define NORD15 "\033[95m"   // #B48EAD

// System types for different ASCII art
typedef enum {
    SYSTEM_BEDROCK,
    SYSTEM_GENTOO,
    SYSTEM_CACHYOS,
    SYSTEM_OTHER
} system_type_t;

// System info structure
struct sysinfo_fast {
    char distro[SMALL_BUFFER];
    char kernel[SMALL_BUFFER];
    char uptime[SMALL_BUFFER];
    char memory[SMALL_BUFFER];
    char wm[SMALL_BUFFER];
    char terminal[SMALL_BUFFER];
    char shell[SMALL_BUFFER];
    char cpu[SMALL_BUFFER];
    char gpu[SMALL_BUFFER];
    char packages[SMALL_BUFFER];
    system_type_t system_type;
};

struct env_cache {
    const char* home;
    const char* shell;
    const char* term_program;
    const char* term;
    const char* xdg_current_desktop;
    const char* desktop_session;
};

static struct env_cache g_env;

static int read_file_fast(const char* path, char* buffer, size_t size);
static int read_file_at(int dirfd, const char* name, char* buffer, size_t size);

// --------------------------------------------------------------------------------
// Performance Helper Functions
// --------------------------------------------------------------------------------

#if defined(__x86_64__)
INLINE long raw_syscall0(long n) {
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n) : "rcx", "r11", "memory");
    return ret;
}

INLINE long raw_syscall1(long n, long a1) {
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

INLINE long raw_syscall2(long n, long a1, long a2) {
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return ret;
}

INLINE long raw_syscall3(long n, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}

INLINE long raw_syscall4(long n, long a1, long a2, long a3, long a4) {
    register long r10 __asm__("r10") = a4;
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
                      : "rcx", "r11", "memory");
    return ret;
}

INLINE long raw_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    register long r9 __asm__("r9") = a6;
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret)
                      : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
                      : "rcx", "r11", "memory");
    return ret;
}
#else
INLINE long raw_syscall0(long n) { return syscall(n); }
INLINE long raw_syscall1(long n, long a1) { return syscall(n, a1); }
INLINE long raw_syscall2(long n, long a1, long a2) { return syscall(n, a1, a2); }
INLINE long raw_syscall3(long n, long a1, long a2, long a3) { return syscall(n, a1, a2, a3); }
INLINE long raw_syscall4(long n, long a1, long a2, long a3, long a4) { return syscall(n, a1, a2, a3, a4); }
INLINE long raw_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    return syscall(n, a1, a2, a3, a4, a5, a6);
}
#endif

INLINE int raw_errno(long r) { return (unsigned long)r >= (unsigned long)-4095; }

INLINE int os_open(const char* path, int flags) {
    long r = raw_syscall4(SYS_openat, AT_FDCWD, (long)path, flags, 0);
    return raw_errno(r) ? -1 : (int)r;
}

INLINE int os_openat(int dirfd, const char* name, int flags) {
    long r = raw_syscall4(SYS_openat, dirfd, (long)name, flags, 0);
    return raw_errno(r) ? -1 : (int)r;
}

INLINE ssize_t os_read(int fd, void* buf, size_t size) {
    long r = raw_syscall3(SYS_read, fd, (long)buf, size);
    return raw_errno(r) ? -1 : (ssize_t)r;
}

INLINE int os_close(int fd) {
    long r = raw_syscall1(SYS_close, fd);
    return raw_errno(r) ? -1 : 0;
}

INLINE ssize_t os_write(int fd, const void* buf, size_t size) {
    long r = raw_syscall3(SYS_write, fd, (long)buf, size);
    return raw_errno(r) ? -1 : (ssize_t)r;
}

INLINE ssize_t os_readlink(const char* path, char* buf, size_t size) {
    long r = raw_syscall4(SYS_readlinkat, AT_FDCWD, (long)path, (long)buf, size);
    return raw_errno(r) ? -1 : (ssize_t)r;
}

INLINE int os_fstat(int fd, struct stat* st) {
    long r = raw_syscall2(SYS_fstat, fd, (long)st);
    return raw_errno(r) ? -1 : 0;
}

#ifdef BFETCH_ENABLE_BTRFS_PACMAN
INLINE int os_fstatfs(int fd, struct statfs* st) {
    long r = raw_syscall2(SYS_fstatfs, fd, (long)st);
    return raw_errno(r) ? -1 : 0;
}

INLINE int os_have_cap_sys_admin(void) {
    struct __user_cap_header_struct hdr;
    struct __user_cap_data_struct data[2];
    memset(&hdr, 0, sizeof(hdr));
    memset(data, 0, sizeof(data));
    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    long r = raw_syscall2(SYS_capget, (long)&hdr, (long)data);
    if (raw_errno(r)) return 0;
    return (data[CAP_TO_INDEX(CAP_SYS_ADMIN)].effective & CAP_TO_MASK(CAP_SYS_ADMIN)) != 0;
}
#endif

INLINE pid_t os_getppid(void) {
    long r = raw_syscall0(SYS_getppid);
    return raw_errno(r) ? 0 : (pid_t)r;
}

INLINE int os_uname(struct utsname* u) {
    long r = raw_syscall1(SYS_uname, (long)u);
    return raw_errno(r) ? -1 : 0;
}

INLINE int os_write_all(int fd, const char* buf, size_t len) {
    while (len) {
        ssize_t wr = os_write(fd, buf, len);
        if (wr <= 0) return -1;
        buf += wr;
        len -= (size_t)wr;
    }
    return 0;
}

INLINE int os_path_exists_dir(const char* path) {
    int fd = os_open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return 0;
    os_close(fd);
    return 1;
}

INLINE void* os_mmap(size_t len, int prot, int flags, int fd, off_t off) {
    long r = raw_syscall6(SYS_mmap, 0, len, prot, flags, fd, off);
    return raw_errno(r) ? MAP_FAILED : (void*)r;
}

INLINE int os_munmap(void* addr, size_t len) {
    long r = raw_syscall2(SYS_munmap, (long)addr, len);
    return raw_errno(r) ? -1 : 0;
}

INLINE int os_madvise(void* addr, size_t len, int advice) {
    long r = raw_syscall3(SYS_madvise, (long)addr, len, advice);
    return raw_errno(r) ? -1 : 0;
}

static void init_env_cache(char* envp[]) {
    g_env.home = NULL;
    g_env.shell = NULL;
    g_env.term_program = NULL;
    g_env.term = NULL;
    g_env.xdg_current_desktop = NULL;
    g_env.desktop_session = NULL;

    for (char** p = envp; p && *p; p++) {
        const char* e = *p;
        switch (e[0]) {
            case 'D':
                if (!g_env.desktop_session && memcmp(e, "DESKTOP_SESSION=", 16) == 0)
                    g_env.desktop_session = e + 16;
                break;
            case 'H':
                if (!g_env.home && memcmp(e, "HOME=", 5) == 0)
                    g_env.home = e + 5;
                break;
            case 'S':
                if (!g_env.shell && memcmp(e, "SHELL=", 6) == 0)
                    g_env.shell = e + 6;
                break;
            case 'T':
                if (!g_env.term && memcmp(e, "TERM=", 5) == 0) {
                    g_env.term = e + 5;
                } else if (!g_env.term_program && memcmp(e, "TERM_PROGRAM=", 13) == 0) {
                    g_env.term_program = e + 13;
                }
                break;
            case 'X':
                if (!g_env.xdg_current_desktop && memcmp(e, "XDG_CURRENT_DESKTOP=", 20) == 0)
                    g_env.xdg_current_desktop = e + 20;
                break;
        }
    }
}

static int parse_cpu_range_list(const char* s) {
    int total = 0;
    while (*s) {
        while (*s == ',' || *s == ' ' || *s == '\t' || *s == '\n') s++;
        if (*s < '0' || *s > '9') break;
        int start = 0;
        while (*s >= '0' && *s <= '9') {
            start = start * 10 + (*s - '0');
            s++;
        }
        int end = start;
        if (*s == '-') {
            s++;
            end = 0;
            while (*s >= '0' && *s <= '9') {
                end = end * 10 + (*s - '0');
                s++;
            }
        }
        if (end >= start) total += end - start + 1;
        while (*s && *s != ',') s++;
    }
    return total;
}

static int get_cpu_threads(void) {
    char buf[128];
    if (read_file_fast("/sys/devices/system/cpu/online", buf, sizeof(buf))) {
        int n = parse_cpu_range_list(buf);
        if (n > 0) return n;
    }
    if (read_file_fast("/sys/devices/system/cpu/present", buf, sizeof(buf))) {
        int n = parse_cpu_range_list(buf);
        if (n > 0) return n;
    }
    return 1;
}

static int print_text_and_return(const char* s) {
    os_write_all(STDOUT_FILENO, s, strlen(s));
    return 0;
}

/* pci.ids uses lowercase hex; amdgpu.ids uses uppercase in keys. */
INLINE void hex4_lower(char* out, unsigned v) {
    static const char xd[] = "0123456789abcdef";
    out[0] = xd[(v >> 12) & 15];
    out[1] = xd[(v >> 8) & 15];
    out[2] = xd[(v >> 4) & 15];
    out[3] = xd[v & 15];
}

INLINE void hex4_upper(char* out, unsigned v) {
    static const char Xd[] = "0123456789ABCDEF";
    out[0] = Xd[(v >> 12) & 15];
    out[1] = Xd[(v >> 8) & 15];
    out[2] = Xd[(v >> 4) & 15];
    out[3] = Xd[v & 15];
}

INLINE void hex2_upper(char* out, unsigned v) {
    static const char Xd[] = "0123456789ABCDEF";
    out[0] = Xd[(v >> 4) & 15];
    out[1] = Xd[v & 15];
}

static void copy_trunc(char* dst, const char* src, size_t cap) {
    if (cap == 0) return;
    size_t n = strnlen(src, cap - 1);
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static size_t map_len(off_t sz) { return (sz > 0) ? (size_t)sz : (size_t)0; }

static const char* skip_inline_ws_const(const char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static int read_file_fast(const char* path, char* buffer, size_t size) {
    int fd = os_open(path, O_RDONLY | O_CLOEXEC);
    if (UNLIKELY(fd == -1)) return 0;
    ssize_t bytes_read = os_read(fd, buffer, size - 1);
    os_close(fd);
    if (LIKELY(bytes_read > 0)) {
        buffer[bytes_read] = '\0';
        return 1;
    }
    return 0;
}

static int read_file_at(int dirfd, const char* name, char* buffer, size_t size) {
    int fd = os_openat(dirfd, name, O_RDONLY | O_CLOEXEC);
    if (UNLIKELY(fd == -1)) return 0;
    ssize_t bytes_read = os_read(fd, buffer, size - 1);
    os_close(fd);
    if (LIKELY(bytes_read > 0)) {
        buffer[bytes_read] = '\0';
        return 1;
    }
    return 0;
}

static int extract_line_value(char* dst, size_t cap, const char* buf, const char* key) {
    const size_t klen = strlen(key);
    const char* p = strstr(buf, key);
    if (!p) return 0;
    p = skip_inline_ws_const(p + klen);
    const char* end = strchr(p, '\n');
    if (!end) end = p + strlen(p);
    while (end > p && (end[-1] == ' ' || end[-1] == '\t')) end--;
    size_t len = (size_t)(end - p);
    if (len >= cap) len = cap - 1;
    memcpy(dst, p, len);
    dst[len] = '\0';
    return len != 0;
}

static int get_single_nvidia_proc_gpu(char* gpu) {
    int fd = os_open("/proc/driver/nvidia/gpus", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return 0;

#if defined(__linux__) && defined(SYS_getdents64)
    char buf[4096];
    char slot[32] = "";
    int seen = 0;

    for (;;) {
        long nr = raw_syscall3(SYS_getdents64, fd, (long)buf, sizeof(buf));
        if (nr <= 0) break;
        for (long pos = 0; pos < nr;) {
            struct linux_dirent64* de = (struct linux_dirent64*)(buf + pos);
            unsigned short reclen = de->d_reclen;
            if (reclen < sizeof(struct linux_dirent64)) break;
            if (de->d_name[0] != '.') {
                if (++seen > 1) {
                    os_close(fd);
                    return 0;
                }
                copy_trunc(slot, de->d_name, sizeof(slot));
            }
            pos += reclen;
        }
    }
    os_close(fd);

    if (seen != 1 || slot[0] == '\0') return 0;

    char path[96];
    char info_buf[2048];
    snprintf(path, sizeof(path), "/proc/driver/nvidia/gpus/%s/information", slot);
    return read_file_fast(path, info_buf, sizeof(info_buf)) &&
           extract_line_value(gpu, SMALL_BUFFER, info_buf, "Model:");
#else
    close(fd);
    return 0;
#endif
}

#if defined(__linux__) && defined(SYS_getdents64)
HOT static int count_dirfd(int fd) {
    char buf[DENTS_BUFFER];
    int count = 0;
    for (;;) {
        long nr = raw_syscall3(SYS_getdents64, fd, (long)buf, sizeof(buf));
        if (nr <= 0) break;
        for (long pos = 0; pos < nr;) {
            const struct linux_dirent64* de = (const struct linux_dirent64*)(buf + pos);
            count += (de->d_name[0] != '.');
            pos += de->d_reclen;
        }
    }
    return count;
}

#ifdef BFETCH_ENABLE_BTRFS_PACMAN
static int count_btrfs_dir_index_fd(int fd) {
    struct stat st;
    if (os_fstat(fd, &st) != 0) return -1;

    struct {
        struct btrfs_ioctl_search_key key;
        __u64 buf_size;
        unsigned char buf[BTRFS_SEARCH_BUFFER];
    } search;

    memset(&search, 0, sizeof(search));
    search.buf_size = sizeof(search.buf);
    search.key.tree_id = 0; /* Search the subvolume tree of this inode. */
    search.key.min_objectid = (uint64_t)st.st_ino;
    search.key.max_objectid = (uint64_t)st.st_ino;
    search.key.min_offset = 0;
    search.key.max_offset = UINT64_MAX;
    search.key.min_transid = 0;
    search.key.max_transid = UINT64_MAX;
    search.key.min_type = BTRFS_DIR_INDEX_KEY;
    search.key.max_type = BTRFS_DIR_INDEX_KEY;

    int count = 0;
    for (;;) {
        search.key.nr_items = UINT32_MAX;
        if (ioctl(fd, BTRFS_IOC_TREE_SEARCH_V2, &search) != 0) return -1;
        if (search.key.nr_items == 0) break;

        uint64_t last_offset = search.key.min_offset;
        unsigned char* p = search.buf;
        for (uint32_t i = 0; i < search.key.nr_items; i++) {
            const struct btrfs_ioctl_search_header* h =
                (const struct btrfs_ioctl_search_header*)p;
            count++;
            last_offset = h->offset;
            p += sizeof(*h) + h->len;
        }

        if (last_offset == UINT64_MAX) break;
        search.key.min_offset = last_offset + 1;
    }
    return count;
}
#endif

static int count_pacman_packages(void) {
    int fd = os_open("/var/lib/pacman/local", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return 0;
#ifdef BFETCH_ENABLE_BTRFS_PACMAN
    struct statfs sfs;
    if (os_fstatfs(fd, &sfs) == 0 &&
        (unsigned long)sfs.f_type == (unsigned long)BTRFS_SUPER_MAGIC &&
        os_have_cap_sys_admin()) {
        int btrfs_count = count_btrfs_dir_index_fd(fd);
        if (btrfs_count >= 0) {
            os_close(fd);
            return btrfs_count;
        }
    }
#endif
    char buf[PACMAN_DENTS_BUFFER];
    int n = 0;
    for (;;) {
        long nr = raw_syscall3(SYS_getdents64, fd, (long)buf, sizeof(buf));
        if (nr <= 0) break;
        for (long pos = 0; pos < nr;) {
            const struct linux_dirent64* de = (const struct linux_dirent64*)(buf + pos);
            n += (de->d_name[0] != '.');
            pos += de->d_reclen;
        }
    }
    os_close(fd);
    return n;
}

static int count_dir(const char* path) {
    int fd = os_open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return 0;
    int n = count_dirfd(fd);
    os_close(fd);
    return n;
}

static int count_portage_packages(void) {
    int fd = os_open("/var/db/pkg", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return 0;
    char buf[DENTS_BUFFER];
    int total = 0;
    for (;;) {
        long nr = raw_syscall3(SYS_getdents64, fd, (long)buf, sizeof(buf));
        if (nr <= 0) break;
        for (long pos = 0; pos < nr;) {
            const struct linux_dirent64* de = (const struct linux_dirent64*)(buf + pos);
            if (de->d_name[0] == '.') {
                pos += de->d_reclen;
                continue;
            }
            unsigned char ty = de->d_type;
            if (ty != DT_DIR && ty != DT_UNKNOWN) {
                pos += de->d_reclen;
                continue;
            }
            int sub = os_openat(fd, de->d_name, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
            if (sub >= 0) {
                total += count_dirfd(sub);
                os_close(sub);
            }
            pos += de->d_reclen;
        }
    }
    os_close(fd);
    return total;
}
#else
static int count_dir(const char* path) {
    DIR* d = opendir(path);
    if (!d) return 0;
    int count = 0;
    struct dirent* de;
    while ((de = readdir(d))) {
        if (de->d_name[0] != '.') count++;
    }
    closedir(d);
    return count;
}

static int count_portage_packages(void) {
    DIR* cat_dir = opendir("/var/db/pkg");
    if (!cat_dir) return 0;
    int e_count = 0;
    struct dirent* ce;
    while ((ce = readdir(cat_dir))) {
        if (ce->d_name[0] == '.' || ce->d_type != DT_DIR) continue;
        char cp[1024];
        snprintf(cp, sizeof(cp), "/var/db/pkg/%s", ce->d_name);
        e_count += count_dir(cp);
    }
    closedir(cat_dir);
    return e_count;
}
#endif

// --------------------------------------------------------------------------------
// CPU Detection: CPUID Assembly (Fastest)
// --------------------------------------------------------------------------------
static void get_cpu(char* cpu) {
#if defined(__i386__) || defined(__x86_64__)
    unsigned int eax = 0, ebx, ecx, edx;
    char brand[49] = {0};

    __get_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000004) {
        unsigned int* b = (unsigned int*)brand;
        for (unsigned int i = 0; i < 3; i++) {
            __get_cpuid(0x80000002 + i, &b[i*4+0], &b[i*4+1], &b[i*4+2], &b[i*4+3]);
        }
        brand[48] = '\0';
        char* s = brand;
        while (*s == ' ') s++;
        
        char clean_brand[SMALL_BUFFER];
        char* d = clean_brand;
        int space = 0;
        static const struct {
            const char* s;
            unsigned char len;
        } keywords[] = {
            { "with Radeon Graphics", 20 }, { "Twelve-Core", 11 }, { "Sixteen-Core", 12 },
            { "Eight-Core", 10 }, { "Six-Core", 8 }, { "Quad-Core", 9 },
            { "Processor", 9 }, { "with Graphics", 13 }, { "24-Core", 7 }, { "32-Core", 7 },
            { "64-Core", 7 }, { "12-Core", 7 }, { "16-Core", 7 }, { "6-Core", 6 },
            { "8-Core", 6 }, { "-Core", 5 }, { "Core", 4 },
        };

        while (*s && (d - clean_brand) < SMALL_BUFFER - 64) {
             if (*s == '@') break;

             int matched = 0;
             for (size_t ki = 0; ki < sizeof(keywords) / sizeof(keywords[0]); ki++) {
                 unsigned char klen = keywords[ki].len;
                 if (strncasecmp(s, keywords[ki].s, klen) == 0) {
                     s += klen;
                     matched = 1;
                     break;
                 }
             }
             if (matched) continue;

             if (*s == ' ') {
                 if (!space) { *d++ = ' '; space = 1; }
                 s++;
             } else {
                 *d++ = *s++;
                 space = 0;
             }
        }
        while (d > clean_brand) {
            char last = *(d-1);
            if (last == ' ' || last == '-' || last == '/') { d--; continue; }
            if (isdigit(last)) {
                if (d > clean_brand + 1 && *(d-2) == ' ') { d--; continue; }
            }
            break;
        }
        *d = '\0';

        int threads = get_cpu_threads();
        double ghz = 0.0;
        char freq_buf[32];
        if (read_file_fast("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", freq_buf, sizeof(freq_buf))) {
            ghz = strtod(freq_buf, NULL) / 1000000.0;
        }

        if (ghz > 0.1) {
            snprintf(cpu, SMALL_BUFFER, "%s (%d) @ %.2f GHz", clean_brand, threads, ghz);
        } else {
            snprintf(cpu, SMALL_BUFFER, "%s (%d)", clean_brand, threads);
        }
    } else {
        strcpy(cpu, "Unknown Processor");
    }
#elif defined(__arm__) || defined(__aarch64__)
    char buf[CPUINFO_BUFFER];
    if (read_file_fast("/proc/cpuinfo", buf, sizeof(buf))) {
        char* model = strstr(buf, "model name");
        if (!model) model = strstr(buf, "Hardware");
        if (!model) model = strstr(buf, "Processor");
        
        if (model) {
            char* start = strchr(model, ':');
            if (start) {
                start++;
                while (*start == ' ') start++;
                char* end = strchr(start, '\n');
                if (end) {
                    size_t len = end - start;
                    if (len > SMALL_BUFFER - 1) len = SMALL_BUFFER - 1;
                    memcpy(cpu, start, len);
                    cpu[len] = '\0';
                    
                    int threads = get_cpu_threads();
                    char freq_buf[32];
                    double ghz = 0.0;
                    if (read_file_fast("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", freq_buf, sizeof(freq_buf))) {
                        ghz = strtod(freq_buf, NULL) / 1000000.0;
                    }
                    
                    if (ghz > 0.1) {
                        char final_cpu[SMALL_BUFFER];
                        snprintf(final_cpu, SMALL_BUFFER, "%s (%d) @ %.2f GHz", cpu, threads, ghz);
                        strcpy(cpu, final_cpu);
                    } else {
                        char final_cpu[SMALL_BUFFER];
                        snprintf(final_cpu, SMALL_BUFFER, "%s (%d)", cpu, threads);
                        strcpy(cpu, final_cpu);
                    }
                    return;
                }
            }
        }
    }
    strcpy(cpu, "ARM Processor");
#else
    strcpy(cpu, "Unknown Processor");
#endif
}

// --------------------------------------------------------------------------------
// GPU Detection: Fast DRM-based lookup
// --------------------------------------------------------------------------------
static void get_gpu(char* gpu) {
    if (get_single_nvidia_proc_gpu(gpu)) return;

    char rev_buf[16], pci_slot[32] = "";
    unsigned int vendor = 0, device = 0, sub_vendor = 0, sub_device = 0, revision = 0;
    int drmfd = os_open("/sys/class/drm", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (drmfd < 0) {
        strcpy(gpu, "Unknown GPU");
        return;
    }

    for (int i = 0; i < 10 && vendor == 0; i++) {
        char cardname[6] = { 'c', 'a', 'r', 'd', (char)('0' + i), '\0' };
        int cfd = os_openat(drmfd, cardname, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (cfd < 0) continue;

        char uevent[UEVENT_BUFFER];
        if (read_file_at(cfd, "device/uevent", uevent, sizeof(uevent))) {
            char* p = strstr(uevent, "PCI_ID=");
            if (p) {
                p += 7;
                vendor = (unsigned int)(strtoul(p, NULL, 16) & 0xffffu);
                char* sep = strchr(p, ':');
                if (sep) device = (unsigned int)(strtoul(sep + 1, NULL, 16) & 0xffffu);

                p = strstr(uevent, "PCI_SUBSYS_ID=");
                if (p) {
                    p += 14;
                    sub_vendor = (unsigned int)(strtoul(p, NULL, 16) & 0xffffu);
                    char* sep = strchr(p, ':');
                    if (sep) sub_device = (unsigned int)(strtoul(sep + 1, NULL, 16) & 0xffffu);
                }

                if (vendor == 0x10de) {
                    extract_line_value(pci_slot, sizeof(pci_slot), uevent, "PCI_SLOT_NAME=");
                } else if (vendor == 0x1002 && read_file_at(cfd, "device/revision", rev_buf, sizeof(rev_buf))) {
                    revision = (unsigned int)(strtoul(rev_buf, NULL, 16) & 0xffu);
                }
            } else {
                p = strstr(uevent, "DRIVER=");
                if (p) {
                    p += 7;
                    char* end = strchr(p, '\n');
                    if (end) {
                        size_t len = (size_t)(end - p);
                        if (len > SMALL_BUFFER - 1) len = SMALL_BUFFER - 1;
                        memcpy(gpu, p, len);
                        gpu[len] = '\0';
                        gpu[0] = (char)toupper((unsigned char)gpu[0]);
                        os_close(cfd);
                        os_close(drmfd);
                        return;
                    }
                }
            }
        }
        os_close(cfd);
    }
    os_close(drmfd);

    if (vendor == 0) {
        strcpy(gpu, "Unknown GPU");
        return;
    }

    if (vendor == 0x10de && pci_slot[0]) {
        char info_path[96];
        char info_buf[2048];
        snprintf(info_path, sizeof(info_path), "/proc/driver/nvidia/gpus/%s/information", pci_slot);
        if (read_file_fast(info_path, info_buf, sizeof(info_buf)) &&
            extract_line_value(gpu, SMALL_BUFFER, info_buf, "Model:")) {
            return;
        }
    }

    if (vendor == 0x1002) {
        int afd = os_open("/usr/share/libdrm/amdgpu.ids", O_RDONLY | O_CLOEXEC);
        if (afd != -1) {
            struct stat ast;
            os_fstat(afd, &ast);
            const size_t alen = map_len(ast.st_size);
            if (alen > 0) {
                char* amap = os_mmap(alen, PROT_READ, MAP_PRIVATE, afd, 0);
                os_close(afd);
                if (amap != MAP_FAILED) {
                    (void)os_madvise(amap, alen, MADV_SEQUENTIAL);
                    char s_key[12];
                    const size_t aklen = 9;
                    hex4_upper(s_key, device & 0xffff);
                    s_key[4] = ',';
                    s_key[5] = '\t';
                    hex2_upper(s_key + 6, revision & 0xff);
                    s_key[8] = ',';
                    const char* s_line = memmem(amap, alen, s_key, aklen);
                    if (!s_line) {
                        s_key[5] = ' ';
                        s_line = memmem(amap, alen, s_key, aklen);
                    }
                    if (s_line) {
                        const char* s_name = s_line + aklen;
                        while (*s_name == ' ' || *s_name == '\t') s_name++;
                        const char* s_end = strchr(s_name, '\n');
                        if (s_end) {
                            size_t slen = (size_t)(s_end - s_name);
                            if (slen > SMALL_BUFFER - 1) slen = SMALL_BUFFER - 1;
                            memcpy(gpu, s_name, slen);
                            gpu[slen] = '\0';
                            os_munmap(amap, alen);
                            return;
                        }
                    }
                    os_munmap(amap, alen);
                }
            } else {
                os_close(afd);
            }
        }
    }

    // Open pci.ids
    int fd = os_open("/usr/share/hwdata/pci.ids", O_RDONLY | O_CLOEXEC);
    if (fd == -1) fd = os_open("/usr/share/misc/pci.ids", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        if (vendor == 0x10de) snprintf(gpu, SMALL_BUFFER, "NVIDIA GPU 0x%04x", device);
        else if (vendor == 0x1002) snprintf(gpu, SMALL_BUFFER, "AMD GPU 0x%04x", device);
        else snprintf(gpu, SMALL_BUFFER, "GPU 0x%04x:0x%04x", vendor, device);
        return;
    }
    
    struct stat st;
    os_fstat(fd, &st);
    const size_t plen = map_len(st.st_size);
    if (plen == 0) {
        os_close(fd);
        strcpy(gpu, "Unknown GPU");
        return;
    }
    char* map = os_mmap(plen, PROT_READ, MAP_PRIVATE, fd, 0);
    os_close(fd);
    if (map == MAP_FAILED) {
        strcpy(gpu, "Unknown GPU");
        return;
    }
    (void)os_madvise(map, plen, MADV_SEQUENTIAL);

    const char* const map_end = map + plen;

    char v_search[5];
    v_search[0] = '\n';
    hex4_lower(v_search + 1, vendor & 0xffff);
    const char* v_line = memmem(map, plen, v_search, 5);
    if (v_line) {
        v_line++;
        const char* v_end = strchr(v_line + 4, '\n');
        if (v_end) {
            char d_search[6];
            d_search[0] = '\n';
            d_search[1] = '\t';
            hex4_lower(d_search + 2, device & 0xffff);
            const char* d_line = memmem(v_end, (size_t)(map_end - v_end), d_search, 6);
            if (d_line) {
                const char* d_name = d_line + 6;
                while (*d_name == ' ' || *d_name == '\t') d_name++;
                const char* d_end = strchr(d_name, '\n');

                // If we have subsystem info, try to find the specific manufacturer name
                if (sub_vendor && sub_device) {
                    char s_search[12];
                    s_search[0] = '\n';
                    s_search[1] = '\t';
                    s_search[2] = '\t';
                    hex4_lower(s_search + 3, sub_vendor & 0xffff);
                    s_search[7] = ' ';
                    hex4_lower(s_search + 8, sub_device & 0xffff);
                    const size_t sslen = 12;
                    const char* s_line = memmem(d_name, (size_t)(map_end - d_name), s_search, sslen);
                    if (s_line && s_line < (d_line + 10000)) {
                        const char* s_name = s_line + sslen;
                        while (*s_name == ' ' || *s_name == '\t') s_name++;
                        const char* s_end = strchr(s_name, '\n');
                        if (s_end) { d_name = s_name; d_end = s_end; }
                    }
                }

                const char* br = strchr(d_name, '[');
                if (d_end && br && br < d_end) {
                    const char* br_end = strchr(br, ']');
                    if (br_end && br_end < d_end) { d_name = br + 1; d_end = br_end; }
                }
                if (d_end) {
                    size_t len = (size_t)(d_end - d_name);
                    if (len > 120) len = 120;
                    const char* prefix = "";
                    if (d_name[0] != 'N' && d_name[0] != 'A') { // Simple heuristic to avoid double prefix
                        prefix = (vendor == 0x10de) ? "NVIDIA " : (vendor == 0x1002) ? "AMD " : "";
                    }
                    snprintf(gpu, SMALL_BUFFER, "%s%.*s", prefix, (int)len, d_name);
                    os_munmap(map, plen);
                    return;
                }
            }
        }
    }
    os_munmap(map, plen);
    if (vendor == 0x10de) strcpy(gpu, "NVIDIA GPU");
    else if (vendor == 0x1002) strcpy(gpu, "AMD GPU");
    else strcpy(gpu, "Unknown GPU");
}


// --------------------------------------------------------------------------------
// Terminal Detection: readlink (Fastest)
// --------------------------------------------------------------------------------
static void get_terminal(char* terminal) {
    const char* tp = g_env.term_program;
    if (tp && tp[0]) {
        size_t n = strnlen(tp, SMALL_BUFFER - 1);
        memcpy(terminal, tp, n);
        terminal[n] = '\0';
        return;
    }
    char path[64], buf[256];
    pid_t ppid = os_getppid();
    snprintf(path, sizeof(path), "/proc/%d/exe", ppid);
    ssize_t len = os_readlink(path, buf, sizeof(buf)-1);
    if (len != -1) {
        buf[len] = '\0';
        char* name = strrchr(buf, '/');
        name = name ? name + 1 : buf;
        if (strcmp(name, "bash") != 0 && strcmp(name, "zsh") != 0 && strcmp(name, "fish") != 0 && strcmp(name, "sh") != 0) {
            copy_trunc(terminal, name, SMALL_BUFFER);
            return;
        }
        char stat_path[64], content[512];
        snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", ppid);
        if (read_file_fast(stat_path, content, sizeof(content))) {
            char* p = strrchr(content, ')');
            if (p && p[1] == ' ' && p[2] != '\0' && p[3] == ' ') {
                pid_t pppid = 0;
                char* q = p + 4;
                while (*q >= '0' && *q <= '9') {
                    pppid = (pppid * 10) + (pid_t)(*q - '0');
                    q++;
                }
                snprintf(path, sizeof(path), "/proc/%d/exe", pppid);
                len = os_readlink(path, buf, sizeof(buf)-1);
                if (len != -1) {
                    buf[len] = '\0';
                    name = strrchr(buf, '/');
                    copy_trunc(terminal, name ? name + 1 : buf, SMALL_BUFFER);
                    return;
                }
            }
        }
    }
    {
        const char* term = g_env.term;
        if (term && term[0]) copy_trunc(terminal, term, SMALL_BUFFER);
        else copy_trunc(terminal, "Unknown", SMALL_BUFFER);
    }
}

// --------------------------------------------------------------------------------
// Package Counting Optimized
// --------------------------------------------------------------------------------

static int count_nix_manifest(const char* path, const char* needle, size_t nlen) {
    int fd = os_open(path, O_RDONLY | O_CLOEXEC);
    if (UNLIKELY(fd == -1)) return 0;
    struct stat st;
    if (UNLIKELY(os_fstat(fd, &st) != 0 || st.st_size <= 0)) {
        os_close(fd);
        return 0;
    }

    const size_t len = (size_t)st.st_size;
    char* map = os_mmap(len, PROT_READ, MAP_PRIVATE, fd, 0);
    os_close(fd);

    if (UNLIKELY(map == MAP_FAILED)) return 0;
    (void)os_madvise(map, len, MADV_SEQUENTIAL);

    int count = 0;
    const char* p = map;
    const char* end = map + len;
    const char* found;
    while ((found = memmem(p, (size_t)(end - p), needle, nlen))) {
        count++;
        p = found + nlen;
    }
    os_munmap(map, len);
    return count;
}

#if defined(__linux__) && defined(SYS_getdents64)
HOT static int count_dpkg(void) {
    int fd = os_open("/var/lib/dpkg/info", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return 0;
    char buf[DENTS_BUFFER];
    int count = 0;
    for (;;) {
        long nr = raw_syscall3(SYS_getdents64, fd, (long)buf, sizeof(buf));
        if (nr <= 0) break;
        for (long pos = 0; pos < nr;) {
            const struct linux_dirent64* de = (const struct linux_dirent64*)(buf + pos);
            const char* name = de->d_name;
            if (name[0] == '.') {
                pos += de->d_reclen;
                continue;
            }
            unsigned int len = 0;
            while (name[len]) len++;
            if (len > 5 && memcmp(name + len - 5, ".list", 5) == 0) count++;
            pos += de->d_reclen;
        }
    }
    os_close(fd);
    return count;
}
#else
static int count_dpkg(void) {
    DIR* d = opendir("/var/lib/dpkg/info");
    if (!d) return 0;
    int count = 0;
    struct dirent* de;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.') continue;
        size_t len = strlen(de->d_name);
        if (len > 5 && strcmp(de->d_name + len - 5, ".list") == 0) count++;
    }
    closedir(d);
    return count;
}
#endif

static void append_pkg_count(char* res, size_t* po, int n, const char* label) {
    if (n <= 0 || *po >= SMALL_BUFFER) return;
    char tmp[16];
    unsigned u = (unsigned)n;
    unsigned digits = 0;
    do {
        tmp[digits++] = (char)('0' + (u % 10u));
        u /= 10u;
    } while (u);

    size_t rem = SMALL_BUFFER - *po;
    if (rem <= digits + 1) return;

    for (unsigned i = 0; i < digits; i++) {
        res[*po + i] = tmp[digits - 1u - i];
    }
    *po += digits;
    res[(*po)++] = ' ';

    while (*label && *po < SMALL_BUFFER - 1) {
        res[(*po)++] = *label++;
    }
    res[*po] = '\0';
}

/* system_type affects ASCII art only; package totals always aggregate every backend present. */
static void get_packages(char* packages) {
#if !defined(BFETCH_DISABLE_NIX) || !defined(BFETCH_DISABLE_FLATPAK)
    const char* home = g_env.home;
#endif
#ifndef BFETCH_DISABLE_PACMAN
    int total_pacman = count_pacman_packages();
#else
    int total_pacman = 0;
#endif
#ifndef BFETCH_DISABLE_DPKG
    int total_dpkg = count_dpkg();
#else
    int total_dpkg = 0;
#endif
#ifndef BFETCH_DISABLE_FLATPAK
    int total_flatpak = count_dir("/var/lib/flatpak/app");
#else
    int total_flatpak = 0;
#endif
#ifndef BFETCH_DISABLE_SNAP
    int total_snap = count_dir("/var/lib/snapd/snaps");
#else
    int total_snap = 0;
#endif
#ifndef BFETCH_DISABLE_EMERGE
    int total_emerge = count_portage_packages();
#else
    int total_emerge = 0;
#endif
#ifndef BFETCH_DISABLE_NIX
    int total_nix = 0;
#else
    int total_nix = 0;
#endif

#if !defined(BFETCH_DISABLE_NIX) || !defined(BFETCH_DISABLE_FLATPAK)
    if (home && home[0]) {
        char pbuf[512];
        char* out = pbuf;
        size_t home_len = strnlen(home, sizeof(pbuf) - 64);
        memcpy(out, home, home_len);
        out += home_len;

#ifndef BFETCH_DISABLE_NIX
        memcpy(out, "/.nix-profile/manifest.json", sizeof("/.nix-profile/manifest.json"));
        total_nix += count_nix_manifest(pbuf, NIX_JSON_NEEDLE, NIX_JSON_NLEN);
        if (total_nix == 0) {
            memcpy(out, "/.nix-profile/manifest.nix", sizeof("/.nix-profile/manifest.nix"));
            total_nix += count_nix_manifest(pbuf, NIX_NIX_NEEDLE, NIX_NIX_NLEN);
        }
#endif

#ifndef BFETCH_DISABLE_FLATPAK
        memcpy(out, "/.local/share/flatpak/app", sizeof("/.local/share/flatpak/app"));
        total_flatpak += count_dir(pbuf);
#endif

#ifndef BFETCH_DISABLE_NIX
        memcpy(out, "/.local/state/nix/profiles/home-manager/manifest.json",
               sizeof("/.local/state/nix/profiles/home-manager/manifest.json"));
        total_nix += count_nix_manifest(pbuf, NIX_JSON_NEEDLE, NIX_JSON_NLEN);
#endif
    }
#endif
#ifndef BFETCH_DISABLE_NIX
    total_nix += count_nix_manifest("/nix/var/nix/profiles/default/manifest.json", NIX_JSON_NEEDLE,
                                    NIX_JSON_NLEN);
    total_nix += count_nix_manifest("/run/current-system/sw/manifest.json", NIX_JSON_NEEDLE, NIX_JSON_NLEN);
#endif

    char res[SMALL_BUFFER];
    res[0] = '\0';
    size_t off = 0;
    append_pkg_count(res, &off, total_pacman, "(pacman), ");
    append_pkg_count(res, &off, total_dpkg, "(dpkg), ");
    append_pkg_count(res, &off, total_flatpak, "(flatpak), ");
    append_pkg_count(res, &off, total_snap, "(snap), ");
    append_pkg_count(res, &off, total_nix, "(nix), ");
    append_pkg_count(res, &off, total_emerge, "(emerge), ");
    if (off > 2) {
        res[off - 2] = '\0';
        copy_trunc(packages, res, SMALL_BUFFER);
    } else {
        copy_trunc(packages, "Unknown", SMALL_BUFFER);
    }
}

/* Art theme only: Bedrock install wins over Gentoo stratum hints in os-release. */
static system_type_t get_distro_and_type(char* distro) {
    char buf[1024];
    system_type_t type = SYSTEM_OTHER;
    const int on_bedrock = os_path_exists_dir("/bedrock");

    if (read_file_fast("/etc/os-release", buf, sizeof(buf))) {
        char* p = strstr(buf, "PRETTY_NAME=\"");
        if (p) {
            p += 13;
            char* end = strchr(p, '"');
            if (end) {
                *end = 0;
                copy_trunc(distro, p, SMALL_BUFFER);
            }
        }
        if (strcasestr(buf, "cachyos")) type = SYSTEM_CACHYOS;
        else if (on_bedrock || strcasestr(buf, "bedrock")) type = SYSTEM_BEDROCK;
        else if (strcasestr(buf, "gentoo")) type = SYSTEM_GENTOO;
    }

    if (type == SYSTEM_OTHER && on_bedrock) type = SYSTEM_BEDROCK;
    if (distro[0] == '\0') copy_trunc(distro, "Linux", SMALL_BUFFER);
    return type;
}

static void get_kernel(char* kernel) {
    struct utsname u;
    if (os_uname(&u) == 0) copy_trunc(kernel, u.release, SMALL_BUFFER);
    else copy_trunc(kernel, "Unknown", SMALL_BUFFER);
}

static void get_uptime(char* uptime) {
    char buf[64];
    if (read_file_fast("/proc/uptime", buf, sizeof(buf))) {
        unsigned long secs = strtoul(buf, NULL, 10);
        long m = (long)(secs / 60UL);
        long h = m / 60;
        long d = h / 24;
        if (d > 0) snprintf(uptime, SMALL_BUFFER, "%ldd %ldh %ldm", d, h % 24, m % 60);
        else snprintf(uptime, SMALL_BUFFER, "%ldh %ldm", h, m % 60);
    } else copy_trunc(uptime, "Unknown", SMALL_BUFFER);
}

static void get_memory(char* memory) {
    char buf[4096];
    if (!read_file_fast("/proc/meminfo", buf, sizeof(buf))) {
        copy_trunc(memory, "Unknown", SMALL_BUFFER);
        return;
    }
    unsigned long long total = 0, available = 0;
    unsigned got_total = 0, got_avail = 0;
    for (char* p = buf; *p;) {
        if (memcmp(p, "MemTotal:", 9) == 0) {
            p += 9;
            while (*p == ' ' || *p == '\t') p++;
            total = strtoull(p, &p, 10);
            got_total = 1;
            if (got_avail) break;
            continue;
        }
        if (memcmp(p, "MemAvailable:", 13) == 0) {
            p += 13;
            while (*p == ' ' || *p == '\t') p++;
            available = strtoull(p, &p, 10);
            got_avail = 1;
            if (got_total) break;
            continue;
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    if (total == 0) {
        copy_trunc(memory, "Unknown", SMALL_BUFFER);
        return;
    }
    unsigned long long used = total - available;
    snprintf(memory, SMALL_BUFFER, "%.2f GiB / %.2f GiB",
             (double)used / 1048576.0, (double)total / 1048576.0);
}

static void get_wm(char* wm) {
    const char* v = g_env.xdg_current_desktop;
    if (!v) v = g_env.desktop_session;
    if (v) copy_trunc(wm, v, SMALL_BUFFER);
    else copy_trunc(wm, "Unknown", SMALL_BUFFER);
}


// Buffered output builder
#define OUTPUT_BUFFER 16384
static char g_out[OUTPUT_BUFFER];
static size_t g_off = 0;

INLINE void out_bytes(const char* s, size_t n) {
    if (g_off >= OUTPUT_BUFFER) return;
    size_t rem = OUTPUT_BUFFER - g_off;
    if (n > rem) n = rem;
    memcpy(g_out + g_off, s, n);
    g_off += n;
}

INLINE void out_value(const char* s) {
    out_bytes(s, strnlen(s, SMALL_BUFFER));
}

static void out_line_value(const char* prefix, const char* value) {
    out_bytes(prefix, strlen(prefix));
    out_value(value);
    out_bytes("\n", 1);
}

#define OUT_LIT(s) out_bytes((s), sizeof(s) - 1)

static void print_gentoo_fetch(const struct sysinfo_fast* info) {
    OUT_LIT(RESET BOLD " ┌──┐" NORD1 " ┌──────────────────────────────────┐ " NORD15 BOLD "┌─────┐\n");
    OUT_LIT(RESET BOLD " │" NORD1 "▒▒" RESET BOLD "│" NORD1 " │─────────" RESET BOLD "\\\\\\\\\\\\\\\\\\\\" NORD1 "───────────────│ " NORD15 BOLD "│  G  │\n");
    OUT_LIT(RESET BOLD " │" NORD0 "██" RESET BOLD "│" NORD1 " │───────" RESET BOLD "//+++++++++++\\" NORD1 BOLD "─────────────│ " NORD15 BOLD "│  e  │\n");
    OUT_LIT(RESET BOLD " │" NORD1 "██" RESET BOLD "│" NORD1 " │──────" RESET BOLD "//+++++" NORD1 BOLD "\\\\\\" RESET BOLD "+++++\\" NORD1 BOLD "────────────│ " NORD15 BOLD "│  n  │\n");
    OUT_LIT(RESET BOLD " │" NORD11 "██" RESET BOLD "│" NORD1 " │─────" RESET BOLD "//+++++" NORD1 BOLD "// " RESET BOLD "/" RESET BOLD "+++++++\\" NORD1 BOLD "──────────│ " NORD15 BOLD "│  t  │\n");
    OUT_LIT(RESET BOLD " │" NORD12 "██" RESET BOLD "│" NORD1 " │──────" RESET BOLD "+++++++" NORD1 BOLD "\\\\" RESET BOLD "++++++++++\\" NORD1 BOLD "────────│ " NORD15 BOLD "│  o  │\n");
    OUT_LIT(RESET BOLD " │" NORD13 "██" RESET BOLD "│" NORD1 " │────────" RESET BOLD "++++++++++++++++++" NORD1 BOLD "\\\\" NORD1 "──────│ " NORD15 BOLD "│  o  │\n");
    OUT_LIT(RESET BOLD " │" NORD14 "██" RESET BOLD "│" NORD1 " │─────────" RESET BOLD "//++++++++++++++" NORD1 BOLD "//" NORD1 "───────│ " NORD15 BOLD "└─────┘\n");
    OUT_LIT(RESET BOLD " │" NORD7 "██" RESET BOLD "│" NORD1 " │───────" RESET BOLD "//++++++++++++++" NORD1 BOLD "//" NORD1 "─────────│ \n");
    OUT_LIT(RESET BOLD " │" NORD8 "██" RESET BOLD "│" NORD1 " │──── " RESET BOLD "//++++++++++++++" NORD1 BOLD "//" NORD1 "───────────│ \n");
    OUT_LIT(RESET BOLD " │" NORD9 "██" RESET BOLD "│" NORD1 " │─────" RESET BOLD "//++++++++++" NORD1 BOLD "//" NORD1 "───────────────│\n");
    OUT_LIT(RESET BOLD " │" NORD10 "██" RESET BOLD "│" NORD1 " │─────" RESET BOLD "//+++++++" NORD1 BOLD "//" NORD1 "──────────────────│\n");
    OUT_LIT(RESET BOLD " │" NORD15 "██" RESET BOLD "│" NORD1 " │──────" RESET BOLD "////////" NORD1 BOLD "────────────────────│\n");
    OUT_LIT(RESET BOLD " │" NORD7 "██" RESET BOLD "│" NORD1 " └──────────────────────────────────┘\n");
    out_line_value(RESET BOLD " │" NORD8 "██" RESET BOLD "│ " NORD12 "Distro: " NORD4, info->distro);
    out_line_value(RESET BOLD " │" NORD9 "██" RESET BOLD "│ " NORD12 "Kernel: " NORD4, info->kernel);
    out_line_value(RESET BOLD " │" NORD10 "██" RESET BOLD "│ " NORD15 "Uptime: " NORD4, info->uptime);
    out_line_value(RESET BOLD " │" NORD15 "██" RESET BOLD "│ " NORD15 "WM: " NORD4, info->wm);
    out_line_value(RESET BOLD " │" NORD11 "██" RESET BOLD "│ " NORD15 "Packages: " NORD4, info->packages);
    out_line_value(RESET BOLD " │" NORD12 "██" RESET BOLD "│ " NORD13 "Terminal: " NORD4, info->terminal);
    out_line_value(RESET BOLD " │" NORD13 "██" RESET BOLD "│ " NORD13 "Memory: " NORD4, info->memory);
    out_line_value(RESET BOLD " │" NORD14 "██" RESET BOLD "│ " NORD13 "Shell: " NORD4, info->shell);
    out_line_value(RESET BOLD " │" NORD7 "██" RESET BOLD "│ " NORD9 "CPU: " NORD4, info->cpu);
    out_line_value(RESET BOLD " │" NORD1 "▒▒" RESET BOLD "│ " NORD9 "GPU: " NORD4, info->gpu);
    OUT_LIT(RESET BOLD " └──┘" RESET "\n");
}

static void print_bedrock_fetch(const struct sysinfo_fast* info) {
    OUT_LIT(RESET BOLD " ┌──┐" NORD1 BOLD " ┌──────────────────────────────────┐ " NORD11 BOLD "┌────┐\n");
    OUT_LIT(RESET BOLD " │" NORD1 "▒▒" RESET BOLD "│" NORD1 BOLD " │─" RESET BOLD "\\\\\\\\\\\\\\\\\\\\\\\\\\" NORD1 BOLD "────────────────────│ " NORD11 BOLD "│ 境 │\n");
    OUT_LIT(RESET BOLD " │" NORD0 "██" RESET BOLD "│" NORD1 BOLD " │──" RESET BOLD "\\\\\\      \\\\\\" NORD1 BOLD "────────────────────│ " NORD11 BOLD "│    │\n");
    OUT_LIT(RESET BOLD " │" NORD1 "██" RESET BOLD "│" NORD1 BOLD " │───" RESET BOLD "\\\\\\      \\\\\\" NORD1 BOLD "───────────────────│ " NORD11 BOLD "│ 界 │\n");
    OUT_LIT(RESET BOLD " │" NORD11 "██" RESET BOLD "│" NORD1 BOLD " │────" RESET BOLD "\\\\\\      \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" NORD1 BOLD "────│ " NORD11 BOLD "└────┘\n");
    OUT_LIT(RESET BOLD " │" NORD12 "██" RESET BOLD "│" NORD1 BOLD " │─────" RESET BOLD "\\\\\\                    \\\\\\" NORD1 BOLD "───│\n");
    OUT_LIT(RESET BOLD " │" NORD13 "██" RESET BOLD "│" NORD1 BOLD " │──────" RESET BOLD "\\\\\\                    \\\\\\" NORD1 BOLD "──│\n");
    OUT_LIT(RESET BOLD " │" NORD14 "██" RESET BOLD "│" NORD1 BOLD " │───────" RESET BOLD "\\\\\\        ──────      \\\\\\" NORD1 BOLD "─│\n");
    OUT_LIT(RESET BOLD " │" NORD7 "██" RESET BOLD "│" NORD1 BOLD " │────────" RESET BOLD "\\\\\\                   ///" NORD1 BOLD "─│\n");
    OUT_LIT(RESET BOLD " │" NORD8 "██" RESET BOLD "│" NORD1 BOLD " │─────────" RESET BOLD "\\\\\\                 ///" NORD1 BOLD "──│\n");
    OUT_LIT(RESET BOLD " │" NORD9 "██" RESET BOLD "│" NORD1 BOLD " │──────────" RESET BOLD "\\\\\\               ///" NORD1 BOLD "───│\n");
    OUT_LIT(RESET BOLD " │" NORD10 "██" RESET BOLD "│" NORD1 BOLD " │───────────" RESET BOLD "\\\\\\////////////////" NORD1 BOLD "────│\n");
    OUT_LIT(RESET BOLD " │" NORD15 "██" RESET BOLD "│" NORD1 BOLD " └──────────────────────────────────┘\n");
    out_line_value(RESET BOLD " │" NORD7 "██" RESET BOLD "│ " NORD12 "Distro: " NORD4, info->distro);
    out_line_value(RESET BOLD " │" NORD8 "██" RESET BOLD "│ " NORD12 "Kernel: " NORD4, info->kernel);
    out_line_value(RESET BOLD " │" NORD9 "██" RESET BOLD "│ " NORD15 "Uptime: " NORD4, info->uptime);
    out_line_value(RESET BOLD " │" NORD10 "██" RESET BOLD "│ " NORD15 "WM: " NORD4, info->wm);
    out_line_value(RESET BOLD " │" NORD15 "██" RESET BOLD "│ " NORD15 "Packages: " NORD4, info->packages);
    out_line_value(RESET BOLD " │" NORD11 "██" RESET BOLD "│ " NORD13 "Terminal: " NORD4, info->terminal);
    out_line_value(RESET BOLD " │" NORD12 "██" RESET BOLD "│ " NORD13 "Memory: " NORD4, info->memory);
    out_line_value(RESET BOLD " │" NORD13 "██" RESET BOLD "│ " NORD13 "Shell: " NORD4, info->shell);
    out_line_value(RESET BOLD " │" NORD14 "██" RESET BOLD "│ " NORD9 "CPU: " NORD4, info->cpu);
    out_line_value(RESET BOLD " │" NORD1 "▒▒" RESET BOLD "│ " NORD9 "GPU: " NORD4, info->gpu);
    OUT_LIT(RESET BOLD " └──┘" RESET "\n");
}

static void print_cachyos_fetch(const struct sysinfo_fast* info) {
    OUT_LIT(RESET BOLD " ┌──┐" NORD1 BOLD " ┌──────────────────────────────────┐ " NORD11 BOLD "┌────┐\n");
    OUT_LIT(RESET BOLD " │" NORD1 "▒▒" RESET BOLD "│" NORD1 BOLD " │─────" NORD7 "/" NORD3 "--" NORD4 "++++++++++" NORD3 "----" NORD7 "/" NORD1 BOLD "───────────│ " NORD11 BOLD "│ 境 │\n");
    OUT_LIT(RESET BOLD " │" NORD0 "██" RESET BOLD "│" NORD1 BOLD " │────" NORD7 "//" NORD4 "+++++++++++" NORD3 "----" NORD7 "/" NORD1 BOLD "─────" NORD7 "/\\\\" NORD1 BOLD "────│ " NORD11 BOLD "│    │\n");
    OUT_LIT(RESET BOLD " │" NORD1 "██" RESET BOLD "│" NORD1 BOLD " │───" NORD7 "//" NORD4 "++++++++++++++++" NORD1 BOLD "──────" NORD7 "\\//" NORD1 BOLD "────│ " NORD11 BOLD "│ 界 │\n");
    OUT_LIT(RESET BOLD " │" NORD11 "██" RESET BOLD "│" NORD1 BOLD " │──" NORD7 "//" NORD4 "++" NORD3 "---" NORD4 "+" NORD7 "//" NORD1 BOLD "──────────────────────│ " NORD11 BOLD "└────┘\n");
    OUT_LIT(RESET BOLD " │" NORD12 "██" RESET BOLD "│" NORD1 BOLD " │─" NORD7 "//" NORD3 "---" NORD4 "+++" NORD7 "//" NORD1 BOLD "────────────" NORD7 "/+\\\\" NORD1 BOLD "───────│\n");
    OUT_LIT(RESET BOLD " │" NORD13 "██" RESET BOLD "│" NORD1 BOLD " │─" NORD7 "\\\\" NORD4 "++++" NORD3 "--" NORD7 "/" NORD1 BOLD "─────────────" NORD7 "\\-//" NORD1 BOLD "───────│\n");
    OUT_LIT(RESET BOLD " │" NORD14 "██" RESET BOLD "│" NORD1 BOLD " │──" NORD7 "\\\\" NORD3 "--" NORD4 "+++" NORD7 "\\" NORD1 BOLD "──────────────────" NORD7 "/++\\\\" NORD1 BOLD "─│\n");
    OUT_LIT(RESET BOLD " │" NORD7 "██" RESET BOLD "│" NORD1 BOLD " │───" NORD7 "\\\\" NORD4 "+++" NORD3 "--" NORD7 "\\" NORD1 BOLD "─────────────────" NORD7 "\\--//" NORD1 BOLD "─│\n");
    OUT_LIT(RESET BOLD " │" NORD8 "██" RESET BOLD "│" NORD1 BOLD " │────" NORD7 "\\\\" NORD3 "--" NORD4 "++++" NORD3 "-+" NORD4 "---" NORD4 "+" NORD3 "--" NORD4 "++++++" NORD7 "/" NORD1 BOLD "───────│\n");
    OUT_LIT(RESET BOLD " │" NORD9 "██" RESET BOLD "│" NORD1 BOLD " │─────" NORD7 "\\" NORD3 "--" NORD4 "+++++++++++++++" NORD3 "--" NORD7 "/" NORD1 BOLD "────────│\n");
    OUT_LIT(RESET BOLD " │" NORD10 "██" RESET BOLD "│" NORD1 BOLD " │──────" NORD7 "\\" NORD3 "-" NORD4 "++++++++++++" NORD3 "----" NORD7 "/" NORD1 BOLD "─────────│\n");
    OUT_LIT(RESET BOLD " │" NORD15 "██" RESET BOLD "│" NORD1 BOLD " └──────────────────────────────────┘\n");
    out_line_value(RESET BOLD " │" NORD7 "██" RESET BOLD "│ " NORD12 "Distro: " NORD4, info->distro);
    out_line_value(RESET BOLD " │" NORD8 "██" RESET BOLD "│ " NORD12 "Kernel: " NORD4, info->kernel);
    out_line_value(RESET BOLD " │" NORD9 "██" RESET BOLD "│ " NORD15 "Uptime: " NORD4, info->uptime);
    out_line_value(RESET BOLD " │" NORD10 "██" RESET BOLD "│ " NORD15 "WM: " NORD4, info->wm);
    out_line_value(RESET BOLD " │" NORD15 "██" RESET BOLD "│ " NORD15 "Packages: " NORD4, info->packages);
    out_line_value(RESET BOLD " │" NORD11 "██" RESET BOLD "│ " NORD13 "Terminal: " NORD4, info->terminal);
    out_line_value(RESET BOLD " │" NORD12 "██" RESET BOLD "│ " NORD13 "Memory: " NORD4, info->memory);
    out_line_value(RESET BOLD " │" NORD13 "██" RESET BOLD "│ " NORD13 "Shell: " NORD4, info->shell);
    out_line_value(RESET BOLD " │" NORD14 "██" RESET BOLD "│ " NORD9 "CPU: " NORD4, info->cpu);
    out_line_value(RESET BOLD " │" NORD1 "▒▒" RESET BOLD "│ " NORD9 "GPU: " NORD4, info->gpu);
    OUT_LIT(RESET BOLD " └──┘" RESET "\n");
}

static void print_fetch(const struct sysinfo_fast* info) {
    if (info->system_type == SYSTEM_GENTOO) print_gentoo_fetch(info);
    else if (info->system_type == SYSTEM_CACHYOS) print_cachyos_fetch(info);
    else print_bedrock_fetch(info);
}

#ifdef BFETCH_ENABLE_PACKAGE_THREAD
struct pkg_thread_arg {
    char* packages;
};

static void* package_thread_main(void* arg) {
    struct pkg_thread_arg* p = (struct pkg_thread_arg*)arg;
    get_packages(p->packages);
    return NULL;
}
#endif

int bfetch_main(int argc, char* argv[], char* envp[]) {
    init_env_cache(envp);
    struct sysinfo_fast info;
    info.distro[0] = '\0';
    info.kernel[0] = '\0';
    info.uptime[0] = '\0';
    info.memory[0] = '\0';
    info.wm[0] = '\0';
    info.terminal[0] = '\0';
    info.shell[0] = '\0';
    info.cpu[0] = '\0';
    info.gpu[0] = '\0';
    info.packages[0] = '\0';
    info.system_type = SYSTEM_OTHER;
    int force_type = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--gentoo") == 0) force_type = SYSTEM_GENTOO;
        else if (strcmp(argv[i], "--cachyos") == 0) force_type = SYSTEM_CACHYOS;
        else if (strcmp(argv[i], "--bedrock") == 0) force_type = SYSTEM_BEDROCK;
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            return print_text_and_return("bfetch version 2.4.0-fastasf\n");
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--Help") == 0) {
            (void)argv;
            return print_text_and_return(
                "Usage: bfetch [OPTIONS]\n"
                "Options:\n"
                "  -v, --version    Show version\n"
                "  -h, --Help       Show this help\n"
                "  --gentoo         Force Gentoo ASCII art\n"
                "  --cachyos        Force CachyOS ASCII art\n"
                "  --bedrock        Force Bedrock ASCII art\n"
            );
        }
    }
    
    // Combined distro + system type (single file read)
    info.system_type = (force_type != -1) ? (system_type_t)force_type : get_distro_and_type(info.distro);
    if (force_type != -1 && info.distro[0] == '\0') {
        get_distro_and_type(info.distro);
    }
    
#ifndef BFETCH_DISABLE_KERNEL
    get_kernel(info.kernel);
#endif
#if !defined(BFETCH_DISABLE_PACKAGES) && defined(BFETCH_ENABLE_PACKAGE_THREAD)
    pthread_t pkg_thread;
    struct pkg_thread_arg pkg_arg = { info.packages };
    const int pkg_thread_ok = (pthread_create(&pkg_thread, NULL, package_thread_main, &pkg_arg) == 0);
#endif
#ifndef BFETCH_DISABLE_UPTIME
    get_uptime(info.uptime);
#endif
#ifndef BFETCH_DISABLE_MEMORY
    get_memory(info.memory);
#endif
#ifndef BFETCH_DISABLE_WM
    get_wm(info.wm);
#endif
#ifndef BFETCH_DISABLE_TERMINAL
    get_terminal(info.terminal);
#endif
#ifndef BFETCH_DISABLE_CPU
    get_cpu(info.cpu);
#endif
#ifndef BFETCH_DISABLE_GPU
    get_gpu(info.gpu);
#endif
#ifndef BFETCH_DISABLE_PACKAGES
#if defined(BFETCH_ENABLE_PACKAGE_THREAD)
    if (pkg_thread_ok) pthread_join(pkg_thread, NULL);
    else get_packages(info.packages);
#else
    get_packages(info.packages);
#endif
#endif

    const char* sh = g_env.shell;
    if (sh) {
        char* b = strrchr(sh, '/');
        copy_trunc(info.shell, b ? b + 1 : sh, SMALL_BUFFER);
    } else {
        copy_trunc(info.shell, "Unknown", SMALL_BUFFER);
    }

#ifndef BFETCH_DISABLE_PRINT
    print_fetch(&info);
    
    // Single write syscall
    if (g_off > 0) os_write_all(STDOUT_FILENO, g_out, (size_t)g_off);
#endif
    
    return 0;
}

int main(int argc, char* argv[]) {
    extern char** environ;
    return bfetch_main(argc, argv, environ);
}
