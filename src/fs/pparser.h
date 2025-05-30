#ifndef PATHPARSER_H
#define PATHPARSER_H


struct path_part {
    const char *part;
    struct path_part *next;
};

struct path_root {
    int driver_no;
    struct path_part *first;

};

struct path_root *pathparser_parse(const char *path, const char *current_directory_path);
void pathparser_free(struct path_root *root);

#endif // PATHPARSER_H