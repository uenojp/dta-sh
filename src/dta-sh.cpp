// 2022 Takahiro Ueno uenotakahiro.jp@gmail.com
//
// Refered to
// - libdft-dta.cpp (https://github.com/AngoraFuzzer/libdft64/blob/066c9d8b3eaa7eeb353dc51771948f3dc8062f95/tools/libdft-dta.cpp)
// - dta-dataleak.cpp (example code for chapter11 of https://practicalbinaryanalysis.com/)

#include <set>

// Intel Pin
#include "pin.H"

// libdft64
#include "branch_pred.h"
#include "libdft_api.h"
#include "syscall_desc.h"

#define DEBUG

extern syscall_desc_t syscall_desc[SYSCALL_MAX];
static tag_traits<tag_t>::type tag = 1;
static std::map<int, std::string> fd2filename;
static std::map<tag_traits<tag_t>::type, int> tag2fd;

// alert aborts the program, called when a data leak is detected.
void alert(tag_traits<tag_t>::type t) {
    int fd = tag2fd[t];
    std::string filename = fd2filename[fd];
    printf("tag %ld, fd %d, filename %s\n", t, fd, filename.c_str());

    for (auto& entry : tag2fd) {
        printf("tag %d fd %d\n", entry.first, entry.second);
    }

    for (auto& entry : fd2filename) {
        printf("fd %d name %s\n", entry.first, entry.second);
    }

    fprintf(stderr, "\n\n!!!!ABORT!!!! detected data leak '%s'\n\n", filename.c_str());
    exit(42);
}

// post_openat_hook pre-hooks openat(2) to save `fd` of opend files.
static void post_openat_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int fd = (const int)ctx->ret;
    const char* pathname = (const char*)ctx->arg[SYSCALL_ARG1];

#ifdef DEBUG
    fprintf(stderr, "post_openat_hook: open %s at fd %d\n", pathname, fd);
#endif

    if (strcmp(pathname, "/usr/lib/ssl/openssl.cnf") != 0) {
        return;
    }
    if (strstr(pathname, ".so") != NULL || strstr(pathname, ".so.") != NULL) {
        return;
    }

    fd2filename.insert(std::make_pair(fd, std::string(pathname)));
}

// taint source
// post_read_hook pre-hooks read(2) to taint the argument `buf`.
static void post_read_hook(THREADID tid, syscall_ctx_t* ctx) {
    const ssize_t nread = (ssize_t)ctx->ret;
    const int fd = (int)ctx->arg[SYSCALL_ARG0];
    const void* buf = (void*)ctx->arg[SYSCALL_ARG1];

    if (unlikely(nread) <= 0) {
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "post_read_hook: read %zd bytes from fd %d\n", nread, fd);
#endif

    if (fd2filename.find(fd) != fd2filename.end()) {
        tagmap_setn((uintptr_t)buf, nread, tag++);
        tag2fd.insert(std::make_pair(tag, fd));
    } else {
        tagmap_clrn((uintptr_t)buf, nread);
    }
}

// taint sink
// pre_sendto_hook post-hooks sendto(2) to check if the argument `buf` is tainted.
static void pre_sendto_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int sockfd = (int)ctx->arg[SYSCALL_ARG0];
    const void* buf = (void*)ctx->arg[SYSCALL_ARG1];
    const size_t len = (size_t)ctx->arg[SYSCALL_ARG2];

#ifdef DEBUG
    fprintf(stderr, "pre_sendto_hook: send %zu bytes '%s' to sockfd %d\n", len, (char*)buf, sockfd);
#endif

    const uintptr_t start = (uintptr_t)buf;
    const uintptr_t end = (uintptr_t)buf + len;
    for (uintptr_t addr = start; addr <= end; addr++) {
        tag_traits<tag_t>::type t = tagmap_getb(addr);
        if (t != 0) {
            alert(t);
        }
    }
}

// post_close_hook post-hooks close(2) to delete fd saved in post_openat_hook.
static void post_close_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int ret = (int)ctx->ret;
    const int fd = (int)ctx->arg[SYSCALL_ARG0];

    if (unlikely(ret < 0)) {
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "post_close_hook: close %d\n", fd);
#endif

    if (likely(fd2filename.find(fd) != fd2filename.end())) {
        // fd2filename.erase(fd);
    }
}

int main(int argc, char** argv) {
    PIN_InitSymbols();

    if (unlikely(PIN_Init(argc, argv))) {
        return 1;
    }

    if (unlikely(libdft_init()) != 0) {
        libdft_die();
        return 1;
    }

    syscall_set_post(&syscall_desc[__NR_openat], post_openat_hook);
    syscall_set_post(&syscall_desc[__NR_read], post_read_hook);
    syscall_set_pre(&syscall_desc[__NR_sendto], pre_sendto_hook);
    syscall_set_post(&syscall_desc[__NR_close], post_close_hook);

    PIN_StartProgram();

    return 0;
}
