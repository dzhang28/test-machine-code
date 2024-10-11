#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef int (*CMFunc)(int condition, int value);

static char *buffer;

    static void
handler(int sig, siginfo_t *si, void *unused)
{
    /* Note: calling printf() from a signal handler is not safe
       (and should not be done in production programs), since
       printf() is not async-signal-safe; see signal-safety(7).
       Nevertheless, we use printf() here as a simple way of
       showing that the handler was called. */

    printf("Got signal %d at address: %p\n", sig, si->si_addr);
    exit(EXIT_FAILURE);
}

char codes[] = {
          0x0B, 0x95, 0xA5, 0x42, 0x01, 0x25, 0x8B, 0x15, 0xA0, 0x40, 0x2E, 0x85, 0x82, 0x80
      };

    int
main(void)
{
    int               pagesize;
    struct sigaction  sa;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    // if (sigaction(SIGSEGV, &sa, NULL) == -1)
    //     handle_error("sigaction");
    if (sigaction(SIGILL, &sa, NULL) == -1)
        handle_error("sigaction");

    pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize == -1)
        handle_error("sysconf");

    /* Allocate a buffer aligned on a page boundary;
       initial protection is PROT_READ | PROT_WRITE. */

    buffer = memalign(pagesize, 4 * pagesize);
    if (buffer == NULL)
        handle_error("memalign");

    printf("Start of region:        %p\n", buffer);

    memcpy(buffer, codes, sizeof(codes));

    if (mprotect(buffer, pagesize,
                PROT_READ | PROT_EXEC) == -1)
        handle_error("mprotect");

    CMFunc p = (CMFunc)buffer;

    int result = p(1, 2);
    printf("result = %d\n", result);

    exit(EXIT_SUCCESS);
}
