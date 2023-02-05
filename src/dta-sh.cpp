// 2022 Takahiro Ueno uenotakahiro.jp@gmail.com
//
// Refered to
// - libdft-dta.cpp
//      https://github.com/AngoraFuzzer/libdft64/blob/066c9d8b3eaa7eeb353dc51771948f3dc8062f95/tools/libdft-dta.cpp
// - dta-dataleak.cpp
//      example code for chapter11 of https://practicalbinaryanalysis.com/

#define DEBUG_PRINT
#include "dta-sh.h"

#include <set>

// Intel Pin
#include "pin.H"

// libdft64
#include "branch_pred.h"
#include "libdft_api.h"
#include "syscall_desc.h"

extern syscall_desc_t syscall_desc[SYSCALL_MAX];
static const tag_traits<tag_t>::type tag = 1;
// fdset stores the fd's(file discripters) of opened files. The fd stored in fdset is the target of
// tracking.
static std::set<int> fdset;

// alert aborts the program, called when a data leak is detected.
static void alert(uintptr_t addr) {
    DEBUG("address 0x%lx is tainted", addr);
    DEBUG("\e[91m!!!! ABORT !!!!    Data leak detected\e[0m");
    exit(42);
}

// post_openat_hook pre-hooks openat(2) to save the fd of opend files.
static void post_openat_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int fd = (const int)ctx->ret;
    const char* pathname = (const char*)ctx->arg[SYSCALL_ARG1];

    if (unlikely(fd < 0)) {
        return;
    }

    DEBUG("open %s at fd %d", pathname, fd);

    // Exclude files with '.so' and '.so.' in the filename since dynamic linked binaries loads
    // shared libraries(.so) at runtime. Shared libraries do not need be taint.
    if (strstr(pathname, ".so") != NULL || strstr(pathname, ".so.") != NULL) {
        return;
    }

    fdset.insert(fd);
}

// taint source
// post_read_hook pre-hooks read(2) to taint the argument `buf`.
static void post_read_hook(THREADID tid, syscall_ctx_t* ctx) {
    const ssize_t nread = (const ssize_t)ctx->ret;
    const int fd = (const int)ctx->arg[SYSCALL_ARG0];
    const void* buf = (const void*)ctx->arg[SYSCALL_ARG1];

    if (unlikely(nread <= 0)) {
        return;
    }

    DEBUG("read %zd bytes from fd %d", nread, fd);

    // If the fd is to be tracked(= fdset contins fd), taint bytes from buf to buf+nread-1.
    // Otherwise(= fdset do not contains fd), the memory area is overwritten even if it has been the
    // target of tracing and is no longer the target of tracing, so clean taints from buf to
    // buf+nread-1.
    // Note that you should not clean taints in close callback function becasue the read data will
    // continue to exist after it the file is closed.
    if (fdset.find(fd) != fdset.end()) {
        DEBUG("taint 0x%lx -- 0x%lx", (uintptr_t)buf, (uintptr_t)buf + nread);
        // NOTE: tagmap_setn taints [buf, buf+nread). Data at address buf+len is not tainted.
        // see libdft64/src/tagmap.cpp
        tagmap_setn((uintptr_t)buf, nread, tag);
    } else {
        DEBUG("clear taint 0x%lx -- 0x%lx", (uintptr_t)buf, (uintptr_t)buf + nread);
        tagmap_clrn((uintptr_t)buf, nread);
    }
}

// taint sink
// pre_sendto_hook post-hooks sendto(2) to check if the argument `buf` is tainted.
static void pre_sendto_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int sockfd = (const int)ctx->arg[SYSCALL_ARG0];
    const void* buf = (const void*)ctx->arg[SYSCALL_ARG1];
    const size_t len = (const size_t)ctx->arg[SYSCALL_ARG2];

    DEBUG("send %zu bytes '%s' to sockfd %d", len, (char*)buf, sockfd);

    // Check if each byte between address buf and buf+len is tainted.
    const uintptr_t start = (const uintptr_t)buf;
    const uintptr_t end = (const uintptr_t)buf + len;

    DEBUG("check taint 0x%lx -- 0x%lx", start, end);

    for (uintptr_t addr = start; addr < end; addr++) {
        if (tagmap_getb(addr) != 0) {
            alert(addr);
        }
    }
    DEBUG("OK");
}

// post_close_hook post-hooks close(2) to delete fd saved in post_openat_hook.
static void post_close_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int ret = (const int)ctx->ret;
    const int fd = (const int)ctx->arg[SYSCALL_ARG0];

    if (unlikely(ret < 0)) {
        return;
    }

    DEBUG("close %d", fd);

    if (likely(fdset.find(fd) != fdset.end())) {
        fdset.erase(fd);
    }
}

// post_dup2_hook post-hooks dup2(2) to save the duplicated newfd.
static void post_dup2_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int ret = (const int)ctx->ret;
    const int oldfd = (const int)ctx->arg[SYSCALL_ARG0];
    const int newfd = (const int)ctx->arg[SYSCALL_ARG1];

    if (unlikely(ret < 0)) {
        return;
    }

    DEBUG("duplicate %d and get %d\n", oldfd, newfd);

    if (likely(fdset.find(oldfd) != fdset.end())) {
        DEBUG("insert fd %d", newfd);
        fdset.insert(newfd);
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
    syscall_set_post(&syscall_desc[__NR_dup2], post_dup2_hook);

    PIN_StartProgram();

    return 0;
}
