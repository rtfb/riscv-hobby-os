#include "kernel.h"
#include "riscv.h"
#include "timer.h"
#include "vm.h"

#if CONFIG_MMU

// defined in kernel.ld:
extern void* RAM_START;
extern void* user_code_start;
extern void* rodata_start;
extern void* data_start;
extern void* _end;

// make_kernel_page_table allocates and populates a pagetable for kernel address
// space. It maps all relevant memory ranges with identity mapping (i.e. the
// physical address and the virtual address have the same numeric value).
void* make_kernel_page_table(page_t *pages, int num_pages) {
    void *pagetable = kalloc("make_kernel_page_table", -1);
    if (!pagetable) {
        // TODO: panic
        return 0;
    }
    clear_page_table(pagetable);

    // map kernel code as kernel-executable:
    map_range_id(pagetable, &RAM_START, &user_code_start, PERM_KCODE);

    // map rodata:
    map_range_id(pagetable, &rodata_start, &data_start, PERM_KRODATA);

    // map data segment:
    void *start = (void*)PAGE_ROUND_DOWN(&data_start);
    void *end = (void*)PAGE_ROUND_UP(&_end);
    map_range_id(pagetable, start, end, PERM_KDATA);

    // pre-map all pages in kernel space:
    page_t *p = pages;
    for (int i = 0; i < num_pages; i++, p++) {
        map_page_id(pagetable, p->ptr, PERM_KDATA);
    }

    // map all special-purpose memory addresses as kernel-read-writable:
    map_page_id(pagetable, (void*)PAGE_ROUND_DOWN(MTIME), PERM_KDATA);
    map_page_id(pagetable, (void*)MTIMECMP_BASE, PERM_KDATA);
    map_page_id(pagetable, (void*)PLIC_THRESHOLD, PERM_KDATA);
    map_page_id(pagetable, (void*)UART_BASE, PERM_KDATA);

    return pagetable;
}

// init_user_page_table initializes a given pagetable for userland consumption.
void init_user_page_table(void *pagetable, uint32_t pid, void *kpagetable) {
    clear_page_table(pagetable);

    // Copy non-leaf pointers from the kernel pagetable to user. That's because
    // the kernel sets satp while still executing its own code, and if it
    // weren't mapped as executable, it would page fault. It's safe to do
    // because the userland will not have access to anything mapped for
    // supervisor access anyway.
    copy_kernel_pagemap(pagetable, kpagetable);

    // map rodata to user address space:
    void *start = &rodata_start;
    void *end = &data_start;
    map_range(pagetable, start, end, (void*)USR_VIRT(start), PERM_URODATA, pid);

    // now map all userland code as user-executable:
    start = &user_code_start;
    end = &rodata_start;
    map_range(pagetable, start, end, (void*)USR_VIRT(start), PERM_UCODE, pid);
}

// copy_kernel_pagemap copies the non-leaf pointers to lower level page tables
// from the kernel page table to a given user table.
void copy_kernel_pagemap(regsize_t *upt, regsize_t *kpt) {
    for (int i = 0; i < PAGE_SIZE/sizeof(regsize_t); i++) {
        if ((*kpt & 0xf) == PERM_NONLEAF) {
            upt[i] = *kpt;
        }
        kpt++;
    }
}

// map_page_sv39 populates a given pagetable with an entry that maps a given
// virt_addr to a given phys_addr with given permissions. It might allocate
// additional pages of physical memory for extra page tables, in which case the
// ownership of the pages will be tagged with a given pid.
void map_page_sv39(regsize_t *pagetable, void *phys_addr, regsize_t virt_addr, int perm, uint32_t pid) {
    for (int level = 2; level >= 0; level--) {
        int vpn_n = VPN(virt_addr, level);
        regsize_t pte = pagetable[vpn_n];
        if (pte == 0 && level > 0) {
            regsize_t *pagetable_next = kalloc("pagetable", pid);
            if (pagetable_next == 0) {
                // TODO: panic OOM
                return;
            }
            clear_page_table(pagetable_next);
            pagetable[vpn_n] = PHYS_TO_PTE(pagetable_next) | PERM_NONLEAF;
            pagetable = pagetable_next;
            continue;
        }
        if (level > 0) {
            pagetable = PTE_TO_PHYS(pte);
            continue;
        }
        // if (IS_VALID(pte)) {
        //     // TODO: panic - should never remap?
        //     return;
        // }
        pagetable[vpn_n] = PHYS_TO_PTE(phys_addr) | perm | PTE_V;
        break;
    }
}

// map_range calls map_page_sv39 for all pages in the given range.
void map_range(void *pagetable, void *pa_start, void *pa_end, void *va_start, int perm, uint32_t pid) {
    void *page_pa = pa_start;
    regsize_t va = (regsize_t)va_start;
    while (page_pa != pa_end) {
        map_page_sv39(pagetable, page_pa, va, perm, -1);
        page_pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }
}

// map_range_id is a convenience wrapper around map_range for identity-mapping
// the range in kernel address space.
void map_range_id(void *pagetable, void *pa_start, void *pa_end, int perm) {
    map_range(pagetable, pa_start, pa_end, pa_start, perm, -1);
}

// map_page_id is a convenience wrapper around map_page_sv39 for
// identity-mapping the page in kernel address space.
void map_page_id(void *pagetable, void *pa, int perm) {
    map_page_sv39(pagetable, pa, (regsize_t)pa, perm, -1);
}

void clear_page_table(void *page) {
    for (regsize_t *pte = (regsize_t*)page; pte != page+PAGE_SIZE; pte++) {
        *pte = 0;
    }
}

void free_page_table_r(regsize_t *pt, int level) {
    regsize_t *end = pt + PAGE_SIZE/sizeof(regsize_t);
    for (regsize_t *pte = pt; pte != end; pte++) {
        if (IS_NONLEAF(*pte) && IS_USER(*pte)) {
            if (level == 1) {
                release_page(PTE_TO_PHYS(*pte));
                continue;
            }
            free_page_table_r(PTE_TO_PHYS(*pte), level+1);
        }
    }
    if (level == 1) {
        release_page(PTE_TO_PHYS(*pt));
    }
}

void free_page_table(regsize_t *pt) {
    free_page_table_r(pt, 0);
    release_page(pt);
}

// XXX: this is a quick hack to get me going, but it's wrong for a general case.
// It should be recursive, like free_page_table
void copy_page_table(regsize_t *dst, regsize_t *src, uint32_t pid) {
    copy_page(dst, src); // copy root table
    regsize_t *l2 = find_next_level_page_table(src);
    if (l2 == 0) {
        return;
    }
    regsize_t *l2dst = kalloc("pagetable", pid);
    if (l2dst == 0) {
        // TODO: panic OOM
        return;
    }
    copy_page(l2dst, l2);
    dst[VPN((regsize_t)l2dst, 2)] = PHYS_TO_PTE(l2dst) | PERM_NONLEAF;
    regsize_t *l3 = find_next_level_page_table(l2);
    if (l3 == 0) {
        return;
    }
    regsize_t *l3dst = kalloc("pagetable", pid);
    if (l3dst == 0) {
        // TODO: panic OOM
        return;
    }
    copy_page(l3dst, l3);
    l2dst[VPN((regsize_t)l3dst, 1)] = PHYS_TO_PTE(l3dst) | PERM_NONLEAF;
}

regsize_t* find_next_level_page_table(regsize_t *pagetable) {
    regsize_t *end = pagetable + PAGE_SIZE/sizeof(regsize_t);
    for (regsize_t *pte = pagetable; pte != end; pte++) {
        if (IS_NONLEAF(*pte) && IS_USER(*pte)) {
            return PTE_TO_PHYS(*pte);
        }
    }
    return 0;
}

// va2pa traverses a given page table trying to resolve a given virtual address
// into a physical one. Returns null on failure.
//
// NOTE: This implementation does not handle superpages as they're not used yet.
void* va2pa(regsize_t *pagetable, void *va) {
    int level = 2;
    while (1) {
        if (level < 0) {
            return 0;
        }
        int pte_n = VPN((regsize_t)va, level);
        regsize_t pte = pagetable[pte_n];
        if (!IS_VALID(pte)) {
            return 0;
        }
        if (!IS_NONLEAF(pte)) {
            return (void*)((regsize_t)PTE_TO_PHYS(pte) | PAGE_OFFS(va));
        }
        pagetable = PTE_TO_PHYS(pte);
        level--;
    }
}

void print_perms(regsize_t pte) {
    if (HAS_D(pte)) kprintf("D");
    if (HAS_A(pte)) kprintf("A");
    if (HAS_G(pte)) kprintf("G");
    if (HAS_U(pte)) kprintf("U");
    if (HAS_X(pte)) kprintf("X");
    if (HAS_W(pte)) kprintf("W");
    if (HAS_R(pte)) kprintf("R");
    if (HAS_V(pte)) kprintf("V");
}

void dump_page_table_r(regsize_t *pagetable, int level) {
    kprintf("PT DUMP, L%d: %p\n", level, pagetable);
    for (int i = 0; i < PAGE_SIZE/sizeof(regsize_t); i++) {
        regsize_t pte = pagetable[i];
        if (IS_VALID(pte)) {
            if (IS_NONLEAF(pte)) {
                void *pte_pa = (void*)PTE_TO_PHYS(pte);
                kprintf("recurse %d: %x - %x\n", i, pte, pte_pa);
                dump_page_table_r(pte_pa, level - 1);
            } else {
                void *pa = (void*)PTE_TO_PHYS(pte);
                int perms = PERM_MASK(pte);
                if (i < 10) {
                    kprintf(" ");
                }
                kprintf("%d: %x - %x; ", i, pte, pa);
                print_perms(pte);
                kprintf("\n");
            }
        }
    }
}

void dump_page_table(regsize_t *pagetable) {
    dump_page_table_r(pagetable, 2);
}

#endif // if CONFIG_MMU
