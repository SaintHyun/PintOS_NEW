#include <stdint.h>
#include <stdbool.h>
#include <list.h>
#include <hash.h>
#include "filesys/file.h"
#include "filesys/filesys.h"`

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

struct vm_entry
{
    uint8_t type;
    void *vaddr;
    bool writable;
    bool is_loaded;
    struct file* file;
    struct list_elem mmap_elem;
    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;
    size_t swap_slot;
    struct hash_elem elem;
};

void vm_init (struct hash *vm);
static unsigned vm_hash_func(const struct hash_elem *elem, void *aux);
static bool compare_vaddr(const struct hash_elem *a, const struct hash_elem *b);
