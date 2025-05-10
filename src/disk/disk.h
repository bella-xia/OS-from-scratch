#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

typedef unsigned int PEACHOS_DISK_TYPE;

#define PEACHOS_DISK_TYPE_REAL 0

struct disk {
    PEACHOS_DISK_TYPE type;
    int sector_size;
    int id;
    struct filesystem *filesystems;
    void *fs_private; // the private data of filesystem
};

struct disk *disk_get(int index);
void disk_search_and_init();
int disk_read_block(struct disk *idisk, unsigned int lba, int total, void *buf);

#endif // DISK_H