#include "vfs.h"
#include "string.h"
#include "heap.h"

static vfs_node_t root_node;
static vfs_node_t node_pool[128];
static int pool_idx;

void vfs_init(void) {
    uint32_t i;
    for (i = 0; i < sizeof(vfs_node_t); i++)
        ((uint8_t*)&root_node)[i] = 0;
    root_node.name[0] = '/';
    root_node.name[1] = '\0';
    root_node.type = VFS_DIR;
    root_node.perms = 0755;
    pool_idx = 0;

    vfs_node_t* bin   = vfs_create_node("bin",   VFS_DIR, &root_node);
    vfs_node_t* dev   = vfs_create_node("dev",   VFS_DIR, &root_node);
    vfs_node_t* etc   = vfs_create_node("etc",   VFS_DIR, &root_node);
    vfs_node_t* tmp   = vfs_create_node("tmp",   VFS_DIR, &root_node);
    vfs_node_t* usr   = vfs_create_node("usr",   VFS_DIR, &root_node);
    vfs_node_t* users = vfs_create_node("Users", VFS_DIR, &root_node);
    (void)bin; (void)dev; (void)tmp;

    vfs_node_t* passwd = vfs_create_node("passwd", VFS_FILE, etc);
    passwd->content = "root::0:0:root:/root:/bin/sh\n";
    passwd->size = 29;

    vfs_node_t* hostname = vfs_create_node("hostname", VFS_FILE, etc);
    hostname->content = "orangex";
    hostname->size = 7;

    vfs_node_t* local = vfs_create_node("local", VFS_DIR, usr);
    vfs_create_node("bin", VFS_DIR, local);

    vfs_node_t* root_home = vfs_create_node("root", VFS_DIR, users);

    vfs_node_t* profile = vfs_create_node(".profile", VFS_FILE, root_home);
    profile->content = "# OrangeX user profile\n";
    profile->size = 23;

    vfs_create_node("Documents", VFS_DIR, root_home);

    vfs_node_t* readme = vfs_create_node("readme.txt", VFS_FILE, root_home);
    readme->content = "Welcome to O&angeX_Kernel v0.1.0\n";
    readme->size = 32;

    vfs_add_child(&root_node, bin);
    vfs_add_child(&root_node, dev);
    vfs_add_child(&root_node, etc);
    vfs_add_child(&root_node, tmp);
    vfs_add_child(&root_node, usr);
    vfs_add_child(&root_node, users);
}

vfs_node_t* vfs_get_root(void) { return &root_node; }

vfs_node_t* vfs_create_node(const char* name, uint32_t type, vfs_node_t* parent) {
    vfs_node_t* n;
    uint32_t i;
    if (pool_idx >= 128) return NULL;
    n = &node_pool[pool_idx++];
    for (i = 0; i < sizeof(vfs_node_t); i++) ((uint8_t*)n)[i] = 0;
    for (i = 0; name[i] && i < VFS_NAME_LEN - 1; i++) n->name[i] = name[i];
    n->name[i] = '\0';
    n->type = type;
    n->parent = parent;
    n->perms = (type == VFS_DIR) ? 0755 : 0644;
    return n;
}

void vfs_add_child(vfs_node_t* parent, vfs_node_t* child) {
    if (!parent || !child) return;
    child->next = parent->children;
    parent->children = child;
}

vfs_node_t* vfs_find_child(vfs_node_t* dir, const char* name) {
    vfs_node_t* c;
    if (!dir || !(dir->type & VFS_DIR)) return NULL;
    for (c = dir->children; c; c = c->next)
        if (strcmp(c->name, name) == 0) return c;
    return NULL;
}

static vfs_node_t* find_in_dir(vfs_node_t* dir, const char* name) {
    vfs_node_t* c;
    if (!dir) return NULL;
    for (c = dir->children; c; c = c->next) {
        if (strcmp(c->name, name) == 0) return c;
        if (c->type & VFS_DIR) {
            vfs_node_t* found = find_in_dir(c, name);
            if (found) return found;
        }
    }
    return NULL;
}

vfs_node_t* vfs_find_recursive(vfs_node_t* dir, const char* name) {
    return find_in_dir(dir, name);
}

vfs_node_t* vfs_lookup(const char* path) {
    vfs_node_t* cur;
    char buf[VFS_NAME_LEN];
    if (!path || !*path) return NULL;
    if (path[0] == '/' && path[1] == '\0') return &root_node;
    cur = &root_node;
    if (*path == '/') path++;
    while (*path) {
        uint32_t i = 0;
        while (*path && *path != '/' && i < VFS_NAME_LEN - 1)
            buf[i++] = *path++;
        buf[i] = '\0';
        if (i == 0) { if (*path == '/') path++; continue; }
        if (buf[0] == '.' && buf[1] == '.' && buf[2] == '\0') {
            if (cur->parent) cur = cur->parent;
        } else {
            vfs_node_t* child = vfs_find_child(cur, buf);
            if (!child) return NULL;
            cur = child;
        }
        if (*path == '/') path++;
    }
    return cur;
}

vfs_node_t* vfs_resolve_path(const char* path, vfs_node_t* cwd) {
    if (!path || !*path) return cwd;
    if (path[0] == '/') return vfs_lookup(path);
    return vfs_lookup(path);
}

const char* vfs_get_path(vfs_node_t* node) {
    static char path_buf[512];
    static char rev[512];
    int rpos = 0;
    int pos = 0;
    int i;
    if (!node || node == &root_node) { path_buf[0] = '/'; path_buf[1] = '\0'; return path_buf; }
    while (node && node != &root_node) {
        int len = 0;
        const char* nm = node->name;
        while (nm[len]) len++;
        for (i = 0; i < len; i++) rev[rpos++] = nm[i];
        rev[rpos++] = '/';
        node = node->parent;
    }
    for (i = rpos - 1; i >= 0; i--) path_buf[pos++] = rev[i];
    path_buf[pos] = '\0';
    return path_buf;
}

static vfs_node_t* get_parent(const char* path, char* child_name) {
    char tmp[512];
    int i, last = 0;
    vfs_node_t* parent;
    strcpy(tmp, path);
    for (i = 0; tmp[i]; i++) if (tmp[i] == '/') last = i;
    if (last == 0 && tmp[0] == '/') {
        strcpy(child_name, &tmp[1]);
        return &root_node;
    }
    tmp[last] = '\0';
    i = 0;
    const char* nm = &tmp[last + 1];
    while (nm[i]) { child_name[i] = nm[i]; i++; }
    child_name[i] = '\0';
    parent = vfs_lookup(tmp);
    return parent;
}

int vfs_mkdir(const char* path) {
    char name[VFS_NAME_LEN];
    vfs_node_t* parent = get_parent(path, name);
    if (!parent || !(parent->type & VFS_DIR)) return -1;
    if (vfs_find_child(parent, name)) return -2;
    vfs_node_t* n = vfs_create_node(name, VFS_DIR, parent);
    if (!n) return -3;
    vfs_add_child(parent, n);
    return 0;
}

vfs_node_t* vfs_touch(const char* path) {
    char name[VFS_NAME_LEN];
    vfs_node_t* parent = get_parent(path, name);
    if (!parent || !(parent->type & VFS_DIR)) return NULL;
    vfs_node_t* existing = vfs_find_child(parent, name);
    if (existing) return existing;
    vfs_node_t* n = vfs_create_node(name, VFS_FILE, parent);
    if (!n) return NULL;
    vfs_add_child(parent, n);
    return n;
}

static void vfs_rm_recursive(vfs_node_t* node) {
    vfs_node_t* c = node->children;
    while (c) {
        vfs_node_t* next = c->next;
        if (c->type & VFS_DIR) vfs_rm_recursive(c);
        if (c->content) kfree(c->content);
        if (c->link_target) kfree(c->link_target);
        c = next;
    }
}

int vfs_rm(const char* path) {
    char name[VFS_NAME_LEN];
    vfs_node_t* parent = get_parent(path, name);
    if (!parent) return -1;
    vfs_node_t* target = vfs_find_child(parent, name);
    if (!target) return -2;
    if (target->type & VFS_DIR) vfs_rm_recursive(target);
    if (target->content) kfree(target->content);
    if (target->link_target) kfree(target->link_target);
    if (parent->children == target) {
        parent->children = target->next;
    } else {
        vfs_node_t* prev = parent->children;
        while (prev && prev->next != target) prev = prev->next;
        if (prev) prev->next = target->next;
    }
    target->type = 0;
    return 0;
}

static vfs_node_t* deep_copy(vfs_node_t* src, vfs_node_t* new_parent) {
    vfs_node_t* n = vfs_create_node(src->name, src->type, new_parent);
    if (!n) return NULL;
    n->size = src->size;
    n->perms = src->perms;
    if (src->content && src->size > 0) {
        n->content = (char*)kmalloc(src->size + 1);
        if (n->content) {
            uint32_t i;
            for (i = 0; i <= src->size; i++) n->content[i] = src->content[i];
        }
    }
    vfs_add_child(new_parent, n);
    vfs_node_t* c;
    for (c = src->children; c; c = c->next) deep_copy(c, n);
    return n;
}

int vfs_cp(const char* src_path, const char* dst_path) {
    vfs_node_t* src = vfs_lookup(src_path);
    if (!src) return -1;
    char name[VFS_NAME_LEN];
    vfs_node_t* dst_parent;
    vfs_node_t* existing;
    if (dst_path[0] == '/') {
        existing = vfs_lookup(dst_path);
        if (existing && (existing->type & VFS_DIR)) {
            return deep_copy(src, existing) ? 0 : -3;
        }
    }
    dst_parent = get_parent(dst_path, name);
    if (!dst_parent || !(dst_parent->type & VFS_DIR)) return -4;
    existing = vfs_find_child(dst_parent, name);
    if (existing && (existing->type & VFS_DIR)) {
        return deep_copy(src, existing) ? 0 : -3;
    }
    if (src->type & VFS_DIR) {
        return deep_copy(src, dst_parent) ? 0 : -3;
    }
    vfs_node_t* n = vfs_create_node(name, src->type, dst_parent);
    if (!n) return -5;
    n->size = src->size;
    n->perms = src->perms;
    if (src->content && src->size > 0) {
        n->content = (char*)kmalloc(src->size + 1);
        if (n->content) {
            uint32_t i;
            for (i = 0; i <= src->size; i++) n->content[i] = src->content[i];
        }
    }
    vfs_add_child(dst_parent, n);
    return 0;
}

int vfs_mv(const char* src_path, const char* dst_path) {
    int ret = vfs_cp(src_path, dst_path);
    if (ret == 0) vfs_rm(src_path);
    return ret;
}

uint32_t vfs_disk_usage(vfs_node_t* node) {
    uint32_t total = node->size;
    vfs_node_t* c;
    for (c = node->children; c; c = c->next)
        total += vfs_disk_usage(c);
    return total;
}

uint32_t vfs_count(vfs_node_t* dir) {
    uint32_t count = 0;
    vfs_node_t* c;
    for (c = dir->children; c; c = c->next) count++;
    return count;
}
