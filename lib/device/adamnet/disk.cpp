#ifdef BUILD_ADAM

#include <memory.h>
#include <string.h>

#include "../../include/debug.h"
#include "../utils/utils.h"

#include "../device/adamnet/disk.h"
#include "media.h"

adamDisk::adamDisk()
{

}

mediatype_t adamDisk::mount(FILE *f, const char *filename, uint32_t disksize, mediatype_t disk_type)
{

}

void adamDisk::unmount()
{

}

bool adamDisk::write_blank(FILE *f, uint16_t sectorSize, uint16_t numSectors)
{
    
}


void adamDisk::adamnet_read()
{

}

void adamDisk::adamnet_write()
{

}

void adamDisk::adamnet_format()
{

}

void adamDisk::adamnet_status()
{

}

void adamDisk::adamnet_process(uint8_t b)
{

}

#endif /* BUILD_ADAM */