// 2022 Takahiro Ueno uenotakahiro.jp@gmail.com
//
// Refered to
// - libdft-dta.cpp
//      https://github.com/AngoraFuzzer/libdft64/blob/066c9d8b3eaa7eeb353dc51771948f3dc8062f95/tools/libdft-dta.cpp
// - dta-dataleak.cpp
//      example code for chapter11 of https://practicalbinaryanalysis.com/

#include "dta-sh.h"

#include <set>

// Intel Pin
#include "pin.H"

// libdft64
#include "branch_pred.h"
#include "libdft_api.h"
#include "syscall_desc.h"

// #define DEBUG_PRINT

extern syscall_desc_t syscall_desc[SYSCALL_MAX];
static const tag_traits<tag_t>::type tag = 1;
// fdset stores the fd's(file discripters) of opened files. The fd stored in fdset is the target of
// tracking.
static std::set<int> fdset;

// alert aborts the program, called when a data leak is detected.
void alert() {
    fprintf(stderr, "\n\n!!!!ABORT!!!! detected data leak\n\n");
    exit(42);
}

// post_openat_hook pre-hooks openat(2) to save the fd of opend files.
static void post_openat_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int fd = (const int)ctx->ret;
    const char* pathname = (const char*)ctx->arg[SYSCALL_ARG1];

    if (unlikely(fd < 0)) {
        return;
    }

#ifdef DEBUG_PRINT
    fprintf(stderr, "%-16s: open %s at fd %d\n", __FUNCTION__, pathname, fd);
#endif

#ifdef DEBUG_PRINT
    // fprintf(stderr, "%-16s: open %s at fd %d\n", __FUNCTION__, pathname, fd);
    DEBUG("open %s at fd %d", pathname, fd);
#endif

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

#ifdef DEBUG_PRINT
    // fprintf(stderr, "%-16s: read %zd bytes from fd %d\n", __FUNCTION__, nread, fd);
    DEBUG("read %zd bytes from fd %d", nread, fd);
#endif

    // If the fd is to be tracked(= fdset contins fd), taint bytes from buf to buf+nread-1.
    // Otherwise(= fdset do not contains fd), the memory area is overwritten even if it has been the
    // target of tracing and is no longer the target of tracing, so clean taints from buf to
    // buf+nread-1.
    // Note that you should not clean taints in close callback function becasue the read data will
    // continue to exist after it the file is closed.
    if (fdset.find(fd) != fdset.end()) {
        // NOTE: tagmap_setn taints [buf, buf+nread). Data at address buf+len is not tainted.
        // see libdft64/src/tagmap.cpp
        tagmap_setn((uintptr_t)buf, nread, tag);
#ifdef DEBUG_PRINT
        // fprintf(stderr, "%-16s: taint 0x%lx -- 0x%lx\n", __FUNCTION__, (uintptr_t)buf,
        //         (uintptr_t)buf + nread);
        DEBUG("taint 0x%lx -- 0x%lx", (uintptr_t)buf, (uintptr_t)buf + nread);
#endif
    } else {
        tagmap_clrn((uintptr_t)buf, nread);
#ifdef DEBUG_PRINT
        // fprintf(stderr, "%-16s: clear taint 0x%lx -- 0x%lx\n", __FUNCTION__, (uintptr_t)buf,
        //         (uintptr_t)buf + nread);
        DEBUG("clear taint 0x%lx -- 0x%lx", (uintptr_t)buf, (uintptr_t)buf + nread);
#endif
    }
}

// taint sink
// pre_sendto_hook post-hooks sendto(2) to check if the argument `buf` is tainted.
static void pre_sendto_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int sockfd = (const int)ctx->arg[SYSCALL_ARG0];
    const void* buf = (const void*)ctx->arg[SYSCALL_ARG1];
    const size_t len = (const size_t)ctx->arg[SYSCALL_ARG2];

#ifdef DEBUG_PRINT
    // fprintf(stderr, "%-16s: send %zu bytes '%s' to sockfd %d\n", __FUNCTION__, len, (char*)buf,
    //         sockfd);
    DEBUG("send %zu bytes '%s' to sockfd %d", __FUNCTION__, len, (char*)buf, sockfd);
#endif

    // Check if each byte between address buf and buf+len is tainted.
    const uintptr_t start = (const uintptr_t)buf;
    const uintptr_t end = (const uintptr_t)buf + len;
#ifdef DEBUG_PRINT
    // fprintf(stderr, "%-16s: check taint 0x%lx -- 0x%lx\n", __FUNCTION__, start, end);
    DEBUG("check taint 0x%lx -- 0x%lx", __FUNCTION__, start, end);
#endif
    for (uintptr_t addr = start; addr < end; addr++) {
        if (tagmap_getb(addr) != 0) {
            alert();
        }
    }
#ifdef DEBUG_PRINT
    // fprintf(stderr, "%-16s: OK\n", __FUNCTION__);
    DEBUG("OK");
#endif
}

// post_close_hook post-hooks close(2) to delete fd saved in post_openat_hook.
static void post_close_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int ret = (const int)ctx->ret;
    const int fd = (const int)ctx->arg[SYSCALL_ARG0];

    if (unlikely(ret < 0)) {
        return;
    }

#ifdef DEBUG_PRINT
    // fprintf(stderr, "%-16s: close %d\n", __FUNCTION__, fd);
    DEBUG("close %d", fd);
#endif

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

#ifdef DEBUG_PRINT
    fprintf(stderr, "%-16s: duplicate %d and got %d\n", __FUNCTION__, oldfd, newfd);
#endif

    if (likely(fdset.find(oldfd) != fdset.end())) {
#ifdef DEBUG_PRINT
        fprintf(stderr, "%-16s: insert %d\n", __FUNCTION__, newfd);
#endif
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
