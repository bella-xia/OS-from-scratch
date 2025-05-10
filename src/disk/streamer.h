#ifndef STREAMER_H
#define STREAMER_H

#include "disk/disk.h"


struct disk_stream {
    int pos;
    struct disk *disk;
};


struct disk_stream * diskstreamer_new(int disk_id);
int diskstreamer_seek(struct disk_stream *streamer, int pos);
int diskstreamer_read(struct disk_stream *streamer, void *out, int total);
void diskstreamer_close(struct disk_stream *streamer);


#endif