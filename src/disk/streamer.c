#include "disk/streamer.h"
#include "memory/kheap.h"
#include "config.h"


struct disk_stream * diskstreamer_new(int disk_id) {
    struct disk *disk = disk_get(disk_id);

    if (!disk)
        return 0;
    
    struct disk_stream *streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0;
    streamer->disk = disk;
    return streamer;
}

int diskstreamer_seek(struct disk_stream *streamer, int pos) {
    streamer->pos = pos;
    return 0;
}

int diskstreamer_read(struct disk_stream *streamer, void *out, int total) {
    int sector = streamer->pos / PEACHOS_SECTOR_SIZE;
    int offset = streamer->pos % PEACHOS_SECTOR_SIZE;

    char buf[PEACHOS_SECTOR_SIZE];
    int res = disk_read_block(streamer->disk, sector, 1, buf);
    if (res < 0)
        goto out;
    
    int total_to_read = total > PEACHOS_SECTOR_SIZE ? PEACHOS_SECTOR_SIZE : total;
    for (int i = 0; i < total_to_read; i++)
        *(char *)out++ = buf[offset+i];


    streamer->pos += total_to_read;
    if (total > PEACHOS_SECTOR_SIZE) 
        res = diskstreamer_read(streamer, out, total-PEACHOS_SECTOR_SIZE);

out:
    return res;
}

void diskstreamer_close(struct disk_stream *streamer) {
    kfree((void *)streamer);
}