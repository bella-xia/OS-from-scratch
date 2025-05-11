#include "fs/file.h"
#include "memory/memory.h"
#include "memory/kheap.h"
#include "disk/disk.h"
#include "string/string.h"
#include "fs/fat16.h"
#include "config.h"
#include "status.h"
#include "kernel.h"
#include <stdint.h>

struct filesystem *filesystems[PEACHOS_MAX_FILESYSTEMS];
struct file_descriptor *file_descriptors[PEACHOS_MAX_FILEDESCRIPTORS];


static struct filesystem **fs_get_free_filesystem() {
    int i = 0;
    for (i = 0; i < PEACHOS_MAX_FILESYSTEMS; i++) {
        if (!filesystems[i])
            return &filesystems[i];
    }

    return 0;
}

void fs_insert_filesystem(struct filesystem *filesystem) {
    struct filesystem **fs;
    fs = fs_get_free_filesystem();
    if (!fs) {
        print("problem inserting filesystem"); 
        while (1);
    }

    *fs = filesystem;
     
}

static void fs_static_load() {
   fs_insert_filesystem(fat16_init());
}

void fs_load() {
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

void fs_init() {
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}


static int file_new_descriptor(struct file_descriptor **desc_out) {
    int res = -ENOMEM;

    for (int i = 0; i < PEACHOS_MAX_FILEDESCRIPTORS; i++) {
        if (!file_descriptors[i]) {
            struct file_descriptor *desc = kzalloc(sizeof(struct file_descriptor));
            desc->index = i + 1; // file descriptors start at 1
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }
    return res;
}

static struct file_descriptor *file_get_descriptor(int fd) {
    if (fd <= 0 || fd >= PEACHOS_MAX_FILEDESCRIPTORS)
        return 0;
   
    return file_descriptors[fd-1];
}


struct filesystem *fs_resolve(struct disk *disk) {
    struct filesystem *fs = 0;
    for (int i = 0; i < PEACHOS_MAX_FILESYSTEMS; i++) {
        if (filesystems[i] && !(filesystems[i]->resolve(disk))) {
            fs = filesystems[i];
            break;
        }
    }
return fs;
}


FILE_MODE file_get_mode_by_string(const char *str) {
    FILE_MODE mode = FILE_MODE_INVALID;
    switch (str[0]) {
        case 'r':
            mode = FILE_MODE_READ;
            break;
        case 'w':
            mode = FILE_MODE_WRITE;
            break;
        case 'a':
            mode = FILE_MODE_APPEND;
            break;
        default:
            break;
    };
    
    return mode;
}

int fopen(const char *filename, const char *mode_str) {
    int res = 0;
    struct path_root *root_path = pathparser_parse(filename, NULL);
    if (!root_path) {
        res = -EIVARG;
        goto out;
    }
    // we cannot just have a root path
    // prevent fopening root directory!
    if (!root_path->first) {
        res = -EIVARG;
        goto out;
    }

    // ensure that the disk we are reading from actually exist
    struct disk *disk = disk_get(root_path->driver_no);
    if (!disk) {
        res = -EIO;
        goto out;
    }

    // check there is actually a filesys
    if (!disk->filesystems) {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -EIVARG;
        goto out;
    }

    void *descriptor_private_data = disk->filesystems->open(disk, root_path->first, mode);
    if (ISERR(descriptor_private_data)) {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor *desc = 0;
    res = file_new_descriptor(&desc);
    if (res < 0)
        goto out;

    desc->filesystem = disk->filesystems;
    desc->privates = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;

out:
    // fopen should not return negative values
    if (res < 0)
        res = 0;
    return res;
}

int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd) {
    int res = 0;

    if (size == 0 || nmemb == 0 || fd < 1) {
        res = -EIVARG;
        goto out;
    }

    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIVARG;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->privates, size, nmemb, (char *) ptr);

out:
    return res;
}