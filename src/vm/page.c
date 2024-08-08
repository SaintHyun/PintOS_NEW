#include "vm/page.h"

void vm_init(struct hash *vm)
{
    hash_init(vm, vm_hash_func, compare_vaddr, NULL);
}

static unsigned vm_hash_func(const struct hash_elem *elem, void *aux)
{
    struct vm_entry *vme = hash_entry(elem, struct vm_entry, elem);
    return hash_int(vme->vaddr);
}

static bool compare_vaddr(const struct hash_elem *a, const struct hash_elem *b)
{
    return hash_int(hash_entry(a, struct vm_entry, elem)->vaddr) < hash_int(hash_entry(b, struct vm_entry, elem)->vaddr);
}