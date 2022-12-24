// Intel Pin.
#include "pin.H"

// libdft64
#include <set>

#include "branch_pred.h"
#include "libdft_api.h"
#include "syscall_desc.h"

// #define DEBUG

extern syscall_desc_t syscall_desc[SYSCALL_MAX];
static tag_traits<tag_t>::type tag = 1;
static std::set<int> fdset;

void alert() {
    fprintf(stderr, "\n\n!!!!ABORT!!!! detected data leak\n\n");
    exit(42);
}

static void post_openat_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int fd = (const int)ctx->ret;
    const char* pathname = (const char*)ctx->arg[SYSCALL_ARG1];

#ifdef DEBUG
    fprintf(stderr, "post_openat_hook: open %s at fd %d\n", pathname, fd);
#endif

    if (strstr(pathname, ".so") != NULL || strstr(pathname, ".so.")) {
        return;
    }

    fdset.insert(fd);
}

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

    if (fdset.find(fd) != fdset.end()) {
        tagmap_setn((uintptr_t)buf, nread, tag);
    } else {
        tagmap_clrn((uintptr_t)buf, nread);
    }
}

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
        if (tagmap_getb(addr) != 0) {
            alert();
        }
    }
}

static void post_close_hook(THREADID tid, syscall_ctx_t* ctx) {
    const int ret = (int)ctx->ret;
    const int fd = (int)ctx->arg[SYSCALL_ARG0];

    if (unlikely(ret < 0)) {
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "post_close_hook: close %d\n", fd);
#endif

    if (likely(fdset.find(fd) != fdset.end())) {
        fdset.erase(fd);
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
