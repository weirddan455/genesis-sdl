#include "pcgrandom.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

typedef struct RngState
{
    uint64_t state;
    uint64_t inc;
} RngState;

static RngState rng_state;

void seed_rng(void)
{
    #ifdef _WIN32
    uint64_t buffer[2];
    HMODULE library = LoadLibraryA("advapi32.dll");
    if (library == NULL) {
        fprintf(stderr, "Failed to load advapi32.dll\n");
        exit(EXIT_FAILURE);
    }
    // Calls RtlGenRandom with dynamic function pointer per MSDN: https://docs.microsoft.com/en-us/windows/win32/api/ntsecapi/nf-ntsecapi-rtlgenrandom
    BOOLEAN (*rng_ptr)(PVOID, ULONG) = GetProcAddress(library, "SystemFunction036");
    if (rng_ptr == NULL) {
        fprintf(stderr, "GetProcAddress failed for RtlGenRandom\n");
        exit(EXIT_FAILURE);
    }
    if (rng_ptr(buffer, 16) == FALSE) {
        fprintf(stderr, "RtlGenRandom failed\n");
        exit(EXIT_FAILURE);
    }
    rng_state.state = buffer[0];
    rng_state.inc = buffer[1] | 1;
    FreeLibrary(library);
    #else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        perror("failed to open /dev/urandom");
        exit(EXIT_FAILURE);
    }
    uint64_t buffer[2];
    ssize_t bytes = read(fd, buffer, 16);
    if (bytes != 16) {
        if (bytes == -1) {
            perror("failed to read from /dev/urandom");
        } else {
            fprintf(stderr, "read from /dev/urandom: %zd bytes read. 16 expected\n", bytes);
        }
        exit(EXIT_FAILURE);
    }
    rng_state.state = buffer[0];
    rng_state.inc = buffer[1] | 1;
    close(fd);
    #endif
}

uint32_t pcg_get_random(void)
{
    uint64_t oldstate = rng_state.state;
    rng_state.state = oldstate * 6364136223846793005ULL + rng_state.inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((0 - rot) & 31));
}

uint32_t pcg_ranged_random(uint32_t range)
{
    uint32_t x = pcg_get_random();
    uint64_t m = (uint64_t)x * (uint64_t)range;
    uint32_t l = (uint32_t)m;
    if (l < range)
    {
        uint32_t t = (0 - range) % range;
        while (l < t)
        {
            x = pcg_get_random();
            m = (uint64_t)x * (uint64_t)range;
            l = (uint32_t)m;
        }
    }
    return m >> 32;
}
