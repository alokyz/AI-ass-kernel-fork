#ifndef ORANGEX_VFS_H
#define ORANGEX_VFS_H

#include "types.h"

#define VFS_FILE   0x01
#define VFS_DIR    0x02
#define VFS_LINK   0x04
#define VFS_NAME_LEN 256

typedef struct vfs_node {
    char             name[VFS_NAME_LEN];
    uint32_t         flags;
    uint32_t         type;
    uint32_t         size;
    uint32_t         perms;
    uint32_t         uid;
    uint32_t         gid;
    char*            content;
    char*            link_target;
    struct vfs_node* parent;
    struct vfs_node* children;
    struct vfs_node* next;
} vfs_node_t;

void         vfs_init(void);
vfs_node_t*  vfs_get_root(void);
vfs_node_t*  vfs_lookup(const char* path);
vfs_node_t*  vfs_find_child(vfs_node_t* dir, const char* name);
vfs_node_t*  vfs_create_node(const char* name, uint32_t type, vfs_node_t* parent);
void         vfs_add_child(vfs_node_t* parent, vfs_node_t* child);
const char*  vfs_get_path(vfs_node_t* node);

int          vfs_mkdir(const char* path);
vfs_node_t*  vfs_touch(const char* path);
int          vfs_rm(const char* path);
int          vfs_cp(const char* src, const char* dst);
int          vfs_mv(const char* src, const char* dst);
vfs_node_t*  vfs_find_recursive(vfs_node_t* dir, const char* name);
uint32_t     vfs_disk_usage(vfs_node_t* node);
uint32_t     vfs_count(vfs_node_t* dir);
vfs_node_t*  vfs_resolve_path(const char* path, vfs_node_t* cwd);

#endif
