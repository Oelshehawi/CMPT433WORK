#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "memory_handler.h"
#include "sharedDataLayout.h"

// Shared memory definitions
#define SHARED_MEM_BTCM_START 0x79020000
#define MEM_LENGTH 0x8000 // Larger size to accommodate offset

// Base pointer to shared memory
static volatile uint8_t *pSharedMem = NULL;
static int mem_fd = -1; // File descriptor for /dev/mem

// Initialize and set up shared memory mapping
bool MemoryHandler_init(void)
{
    // Open /dev/mem for memory mapping
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0)
    {
        perror("Failed to open /dev/mem");
        return false;
    }

    // Map the shared memory region with larger size
    pSharedMem = (volatile uint8_t *)mmap(NULL, MEM_LENGTH,
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED,
                                          mem_fd,
                                          SHARED_MEM_BTCM_START);

    if (pSharedMem == MAP_FAILED)
    {
        perror("mmap failed");
        close(mem_fd);
        return false;
    }

    printf("Shared memory mapped successfully at %p\n", (void *)pSharedMem);
    return true;
}

// Write color values to the LED array at the correct offset
void MemoryHandler_writeColors(uint32_t *colors, int numLeds)
{
    // Write calculated colors to shared memory at the correct offset
    volatile uint8_t *base_addr = pSharedMem + NEO_COLOR_ARRAY_OFFSET;
    volatile uint32_t *pixel_map_offset = (volatile uint32_t *)base_addr;

    for (int i = 0; i < numLeds; i++)
    {
        pixel_map_offset[i] = colors[i];
    }
}

// Clean up shared memory resources
void MemoryHandler_cleanup(void)
{
    if (pSharedMem != NULL)
    {
        munmap((void *)pSharedMem, MEM_LENGTH);
        pSharedMem = NULL;
    }

    if (mem_fd >= 0)
    {
        close(mem_fd);
        mem_fd = -1;
    }

    printf("Shared memory unmapped successfully\n");
}