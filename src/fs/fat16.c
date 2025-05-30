#include "fs/fat16.h"
#include "status.h"
#include "config.h"
#include "kernel.h"
#include "string/string.h"
#include "disk/streamer.h"
#include "memory/memory.h"
#include "disk/disk.h"
#include "memory/kheap.h"
#include <stdint.h>

#define PEACHOS_FAT16_SIGNATURE         0x29
#define PEACHOS_FAT16_FAT_ENTRY_SIZE    0x02
#define PEACHOS_FAT16_BAD_SECTOR        0xFF7
#define PEACHOS_FAT16_UNUSED            0x00

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY         0x00
#define FAT_ITEM_TYPE_FILE              0x01

// fat directory entry attributes bitmask
#define FAT_FILE_READ_ONLY              0x01
#define FAT_FILE_HIDDEN                 0x02
#define FAT_FILE_SYSTEM                 0x04
#define FAT_FILE_VOLUME_LABEL           0x08
#define FAT_FILE_SUBDIRECTORY           0x10
#define FAT_FILE_ARCHIVED               0x20
#define FAT_FILE_DEVICE                 0x40
#define FAT_FILE_RESERVED               0x80

struct fat_header_extended {
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

struct fat_header {
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_directory_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;
} __attribute__((packed));

struct fat_h {
    struct fat_header primary_header;
    union fat_h_e {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_item {
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bites_first_cluster;
    uint32_t filesize;
} __attribute__((packed));


struct fat_directory {
    struct fat_directory_item *item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item {
    union {
        struct fat_directory_item *item;
        struct fat_directory *directory;
    };
    FAT_ITEM_TYPE type;
};

struct fat_file_descriptor {
    struct fat_item *item;
    uint32_t pos;
};

struct fat_private {
    struct fat_h header;
    struct fat_directory root_directory;

    // used to stream data clusters
    struct disk_stream *cluster_read_stream;

    // used to stream the file allocation table
    struct disk_stream *fat_read_stream;

    // used in situations where we stream the directory
    struct disk_stream *directory_stream;
};

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *part, FILE_MODE mode);
int fat16_read(struct disk *disk, void *descriptor, uint32_t size, uint32_t nmemb, char *out_ptr);
int fat16_seek(void *privates, uint32_t offset, FILE_SEEK_MODE seek_mode);
int fat16_stat(struct disk *disk, void *fat_private, struct file_stat *stat);
int fat16_close(void *fs_private);

struct filesystem fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close
};

struct filesystem *fat16_init() {
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

static void fat16_init_private(struct disk *disk, struct fat_private *fat_private) {
    memset(fat_private, 0, sizeof(struct fat_private));
    fat_private->cluster_read_stream = diskstreamer_new(disk->id);
    fat_private->fat_read_stream = diskstreamer_new(disk->id);
    fat_private->directory_stream = diskstreamer_new(disk->id);
}
 
int fat16_sector_to_absolute(struct disk *disk, int sector) {
    return sector * disk->sector_size;
}

int fat16_get_total_items_for_directory(struct disk *disk, uint32_t directory_start_sector) {
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private *fat_private = disk->fs_private;

    int res = 0, i = 0;
    int directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream *stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, directory_start_pos) != PEACHOS_ALL_OK) {
        res = -EIO;
        goto out;
    }
    while (1) {
        if (diskstreamer_read(stream, &item, sizeof(item)) != PEACHOS_ALL_OK) {
            res = -EIO;
            goto out;
        }

        if (item.filename[0] == 0x00)
            break;
        
        if (item.filename[0] == 0xE5)
            continue;
        
        i++;
    }
    res = i;

out:
    return res;
}

int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *directory) {
    int res = 0;
    struct fat_header *primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_directory_entries = fat_private->header.primary_header.root_directory_entries;
    int root_dir_size = (root_directory_entries * sizeof(struct fat_directory_item));
    int total_sectors = root_dir_size / disk->sector_size;
    if (root_dir_size % disk->sector_size)
            total_sectors++;
        
    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);
    struct fat_directory_item *dir = kzalloc(root_dir_size);
    if (!dir) {
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream *stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != PEACHOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    if (diskstreamer_read(stream, dir, root_dir_size) != PEACHOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    directory->item = dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;
}

int fat16_resolve(struct disk *disk) {
    int res = 0;

    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);
    disk->fs_private = fat_private;
    disk->filesystems = &fat16_fs;

    struct disk_stream *stream = diskstreamer_new(disk->id);
    if (!stream) {
        res = -ENOMEM;
        goto out;
    }
    if (diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != PEACHOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature != 0x29) {
        res = -EFSNOTUS;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != PEACHOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

out:
    if (stream)
        diskstreamer_close(stream);
    if (res < 0) {
        kfree(fat_private);
        disk->fs_private = 0;
    }
    return res;
}

void fat16_to_proper_string(char **out, const char *in) {
    while (*in != 0x00 && *in != 0x20) 
        *((*out)++) = *in++;

    if (*in == 0x20) 
        **out = 0x00;
}

void fat16_get_full_relative_filename(struct fat_directory_item *item, char *out, int max_len) {
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char *)item->filename);
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20) {
        *out_tmp++ = '.';
        fat16_to_proper_string(&out_tmp, (const char *)item->ext);
    }
}

struct fat_directory_item *fat16_clone_directory_item(struct fat_directory_item *item, int size) {
    struct fat_directory_item *item_copy = 0;
    if (size < sizeof(struct fat_directory_item))
        goto out;
    item_copy = kzalloc(sizeof(struct fat_directory_item));

    if (!item_copy)
        goto out;
    
    memcpy(item_copy, item, size);

out:
    return item_copy;
}

static int fat16_cluster_to_sector(struct fat_private *fat_private, int cluster) {
    return fat_private->root_directory.ending_sector_pos + (cluster - 2) * fat_private->header.primary_header.sectors_per_cluster;
}

static uint32_t fat16_get_first_fat_sector(struct fat_private *fat_private) {
    return fat_private->header.primary_header.reserved_sectors;
}

static int fat16_get_fat_entry(struct disk *disk, int cluster) {
    int res = 0;
    struct fat_private *fat_private = disk->fs_private;
    struct disk_stream *stream = fat_private->fat_read_stream;
    if (!stream)
        goto out;
    
    uint32_t fat_table_position = fat16_get_first_fat_sector(fat_private) * disk->sector_size;
    res = diskstreamer_seek(stream, fat_table_position * (cluster * PEACHOS_FAT16_FAT_ENTRY_SIZE));

    if (res < 0)
        goto out;
    
    uint16_t result = 0;
    res = diskstreamer_read(stream, &result, sizeof(result));
    if (res < 0)
        goto out;
    res = result;
    
out:
    return res;
}


static uint32_t fat16_get_first_cluster(struct fat_directory_item* item)
{
    return (item->high_16_bits_first_cluster) | item->low_16_bites_first_cluster;
};

/**
 * get the correct cluster to use based on the starting cluster and the offset
 */
static int fat16_get_cluster_for_offset(struct disk *disk, int starting_cluster, int offset) {
    int res = 0;
    struct fat_private *fat_private = disk->fs_private;
    int size_of_cluster_bytes = fat_private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / size_of_cluster_bytes;

    for (int i = 0; i < clusters_ahead; i++) {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        if (entry == 0xFF8 || entry == 0xFFF) {
            // end of entry
            res = -EIO;
            goto out;
        }

        // check if the sector is marked as bad
        if (entry == PEACHOS_FAT16_BAD_SECTOR) {
            res = -EIO;
            goto out;
        }

        // reserved sectors?
        if (entry == 0xFF0 || entry == 0xFF6) {
            res = -EIO;
            goto out;
        }
        
        // no cluster
        if (entry == 0x00) {
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry;
        break;
    }

    res = cluster_to_use;

out:
    return res;
}

void fat16_free_directory(struct fat_directory *directory) {
    if (!directory)
        return;
    
    if (directory->item)
        kfree(directory->item);
    
    kfree(directory);
}


void fat16_item_free(struct fat_item *item) {
    switch (item->type) {
        case FAT_ITEM_TYPE_DIRECTORY:
            fat16_free_directory(item->directory);
            break;
        case FAT_ITEM_TYPE_FILE:
            kfree(item->item);
            break;
        default:
            break;
    }
    kfree(item);

}

static int fat16_read_internal_from_stream(struct disk *disk, struct disk_stream *stream, int cluster, int offset, int total, void *out) {
    int res = 0;

    struct fat_private *fat_private = disk->fs_private;
    int size_of_cluster_bytes = fat_private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0) {
        res = cluster_to_use;
        goto out;
    }

    int offset_from_cluster = offset % size_of_cluster_bytes;
    int starting_sector = fat16_cluster_to_sector(fat_private, cluster_to_use);
    int starting_pos = (starting_sector * disk->sector_size ) + offset_from_cluster;
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;
    res = diskstreamer_seek(stream, starting_pos);
    if (res != PEACHOS_ALL_OK)
        goto out;

    res = diskstreamer_read(stream, out, total_to_read);
    if (res != PEACHOS_ALL_OK)
        goto out;
    
    total -= total_to_read;
    if (total > 0) {
        // we still have more to read
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset + total_to_read, total, out + total_to_read);
    }
out:
    return res;
}

static int fat16_read_internal(struct disk *disk, int starting_cluster, int offset, int total, void *out) {
    struct fat_private *fs_private = disk->fs_private;
    struct disk_stream *stream = fs_private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

struct fat_directory *fat16_load_fat_directory(struct disk *disk, struct fat_directory_item *item) {
    int res = 0;
    struct fat_directory *directory = 0;
    struct fat_private *fat_private = disk->fs_private;

    if (!(item->attribute & FAT_FILE_SUBDIRECTORY)) {
        res = -EIVARG;
        goto out;
    }

    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory) {
        res = -ENOMEM;
        goto out;
    }

    int cluster = fat16_get_first_cluster(item);
    int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);
    int total_items = fat16_get_total_items_for_directory(disk, cluster_sector);
    directory->total = total_items;
    int directory_size = directory->total + sizeof(struct fat_directory_item);
    directory->item = kzalloc(directory_size);
    if (!directory->item) {
        res = -ENOMEM;
        goto out;
    }

    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    if (res != PEACHOS_ALL_OK)
        goto out;

out:
    if (res != PEACHOS_ALL_OK)
        fat16_free_directory(directory);

    return directory;
}

struct fat_item *fat16_new_fat_file_for_directory_item(struct disk *disk, struct fat_directory_item *item) {
    struct fat_item *f = kzalloc(sizeof(struct fat_item));
    if (!f)
        return 0;
    
    if (item->attribute & FAT_FILE_SUBDIRECTORY) {
        f->directory = fat16_load_fat_directory(disk, item);
        f->type = FAT_ITEM_TYPE_DIRECTORY;
        goto out;
    }

    f->type = FAT_ITEM_TYPE_FILE;
    f->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));

out:
    return f;
}

struct fat_item *fat16_find_item_in_directory(struct disk *disk, struct fat_directory *directory, const char *name) {
    struct fat_item *f = 0;
    char temp_filename[PEACHOS_MAX_PATH];
    for (int i = 0; i < directory->total; i++) {
        fat16_get_full_relative_filename(&directory->item[i], temp_filename, sizeof(temp_filename));
        if (!istrnmp(temp_filename, name, sizeof(temp_filename))) {
            // found it! create a new fat item
            f = fat16_new_fat_file_for_directory_item(disk, &directory->item[i]);
        }
    }
    return f;
}

struct fat_item *fat16_get_directory_entry(struct disk *disk, struct path_part *path) {
    struct fat_private *fat_private = disk->fs_private;
    struct fat_item *current_item = 0;
    struct fat_item *root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);

    if (!root_item)
        goto out;
    
    struct path_part *next_part = path->next;
    current_item = root_item;
    while (next_part) {
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY) {
            current_item = 0;
            break;
        }
        struct fat_item *tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        fat16_item_free(current_item);
        current_item = tmp_item;
        next_part = next_part->next;
    }

out:
    return current_item;
}

static void fat16_free_file_descriptor(struct fat_file_descriptor *desc) {
    fat16_item_free(desc->item);
    kfree(desc);
}

void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode) {
    if (mode != FILE_MODE_READ)
        return ERROR(-ERDONLY);
    
    struct fat_file_descriptor *descriptor = 0;
    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor)
        return ERROR(-ENOMEM);
    
    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item)
    {
        return ERROR(-EIO);
    }
    
    descriptor->pos = 0;
    return descriptor;
}

int fat16_read(struct disk *disk, void *descriptor, uint32_t size, uint32_t nmemb, char *out_ptr) {
    int res = 0;
    struct fat_file_descriptor *fat_desc = descriptor;
    struct fat_directory_item *item = fat_desc->item->item;
    int offset = fat_desc->pos;
    for (uint32_t i = 0; i < nmemb; i++) {
        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out_ptr);
        if (ISERR(res))
            goto out;
        
        out_ptr += size;
        offset += size;
    }

    res = nmemb;

out:
    return res;
}


int fat16_seek(void *privates, uint32_t offset, FILE_SEEK_MODE seek_mode) {
    int res = 0;
    struct fat_file_descriptor *desc = privates;
    struct fat_item *desc_item = desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE) {
        res = -EIVARG;
        goto out;
    }
    struct fat_directory_item *ritem = desc_item->item;
    if (offset >= ritem->filesize) {
        res = -EIO;
        goto out;
    }

    switch(seek_mode) {
        case SEEK_SET:
            desc->pos = offset;
        break;
        case SEEK_END:
            res = EUNIMP;
        break;
        case SEEK_CUR:
            desc->pos += offset;
        break;
        default:
            res = -EIVARG;
        break;
    }
out:
    return res;
}


int fat16_stat(struct disk *disk, void *fat_private, struct file_stat *stat) {
    int res = 0;
    struct fat_file_descriptor *desc = (struct fat_file_descriptor *)fat_private;
    struct fat_item *desc_item = desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE) {
        res = -EIVARG;
        goto out;
    }

    struct fat_directory_item *ritem = desc_item->item;
    stat->file_size = ritem->filesize;
    stat->flags = 0;

    if (ritem->attribute & FAT_FILE_READ_ONLY)
        stat->flags |= FLAG_STAT_READ_ONLY;
out:
    return res;
}

int fat16_close(void *fs_private) {
    fat16_free_file_descriptor((struct fat_file_descriptor *)fs_private);
    return 0;
}
