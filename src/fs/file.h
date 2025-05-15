#ifndef FILE_H
#define FILE_H

#include "fs/pparser.h"
#include "stdint.h"

typedef unsigned int FILE_SEEK_MODE;
enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

typedef unsigned int FILE_MODE;
enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

typedef unsigned int FILE_STAT_FALGS;
enum {
    FLAG_STAT_READ_ONLY = 0b00000001,
};

struct file_stat {
    FILE_STAT_FALGS flags;
    uint32_t file_size;
};

struct disk;
typedef void*(*FS_OPEN_FUNCTION)(struct disk *disk, struct path_part *part, FILE_MODE mode);
typedef int (*FS_RESOLVE_FUNCTION)(struct disk *disk);
typedef int (*FS_READ_FUNCTION)(struct disk *disk, void *fs_private, uint32_t size, uint32_t nmemb, char *out);
typedef int (*FS_SEEK_FUNCTION)(void *fat_private, uint32_t offset, FILE_SEEK_MODE seek_mode);
typedef int (*FS_STAT_FUNCTION)(struct disk *disk, void *fat_private, struct file_stat *stat);
typedef int (*FS_CLOSE_FUNCTION)(void *fs_private);


struct filesystem {
    // filesystem should return zero from resolve if the provided disk is using its filesystem
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;
    char name[20];
};


struct file_descriptor {
    int index;
    struct filesystem *filesystem;
    void *privates; // provide data for internal file descriptor
    struct disk *disk;
};


void fs_init();
int fopen(const char *filename, const char *mode);
int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, int offset, FILE_SEEK_MODE whence);
int fstat(int fd, struct file_stat *stat);
int fclose(int fd);
void fs_insert_filesystem(struct filesystem *filesystem);
struct filesystem *fs_resolve(struct disk *disk);


#endif // FILE_H
