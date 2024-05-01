#include "kernel.h"
#include "mem.h"
#include "plic.h"
#include "proc.h"
#include "riscv.h"
#include "timer.h"
#include "vm.h"

#if CONFIG_MMU

// map_range_id is a convenience wrapper around map_range for identity-mapping
// the range in kernel address space.
#define map_range_id(pagetable, pa_start, pa_end, perm) \
    map_range(pagetable, pa_start, pa_end, pa_start, perm, -1)

// map_page_id is a convenience wrapper around map_page_sv39 for
// identity-mapping the page in kernel address space.
#define map_page_id(pagetable, pa, perm, pid) \
    map_page_sv39(pagetable, pa, (regsize_t)(pa), perm, pid)

// defined in kernel.ld:
extern void* RAM_START;
extern void* user_code_start;
extern void* rodata_start;
extern void* rodata_end;
extern void* data_start;
extern void* heap_start;

// Note: 512 here means 512 words, which in case of 64 bits makes it a
// PAGE_SIZE, 4KiB. That's because the bottom 4KiB of RAM are reserved for
// kernel stack, and the actual code starts above it.
#define KERNEL_CODE_START (&RAM_START + 512)

// make_kernel_page_table allocates and populates a pagetable for kernel address
// space. It maps all relevant memory ranges with identity mapping (i.e. the
// physical address and the virtual address have the same numeric value).
void* make_kernel_page_table(page_t *pages, int num_pages) {
    void *pagetable = kalloc("make_kernel_page_table", -1);
    if (!pagetable) {
        panic("kernel pagetable alloc");
        return 0;
    }
    memset(pagetable, PAGE_SIZE, 0);

    map_page_id(pagetable, &RAM_START, PERM_KDATA, -1);

    // map kernel code as kernel-executable:
    map_range_id(pagetable, KERNEL_CODE_START, &user_code_start, PERM_KCODE);

    // map rodata:
    map_range_id(pagetable, &rodata_start, (void*)PAGE_ROUND_UP(&rodata_end), PERM_KRODATA);

    // map data segment:
    void *start = (void*)PAGE_ROUND_DOWN(&data_start);
    void *end = (void*)PAGE_ROUND_UP(&heap_start);
    map_range_id(pagetable, start, end, PERM_KDATA);

    // pre-map all pages in kernel space:
    page_t *p = pages;
    for (int i = 0; i < num_pages; i++, p++) {
        map_page_id(pagetable, p->ptr, PERM_KDATA, -1);
    }

    // map all special-purpose memory addresses as kernel-read-writable:
    map_page_id(pagetable, (void*)PAGE_ROUND_DOWN(MTIME), PERM_KDATA, -1);
    map_page_id(pagetable, (void*)MTIMECMP_BASE, PERM_KDATA, -1);
    map_page_id(pagetable, (void*)PLIC_THRESHOLD, PERM_KDATA, -1);
    map_page_id(pagetable, (void*)UART_BASE, PERM_KDATA, -1);

    return pagetable;
}

// init_user_page_table initializes a given pagetable for userland consumption.
void init_user_page_table(void *pagetable, uint32_t pid) {
    memset(pagetable, PAGE_SIZE, 0);

    // Set a few kernel address space pages in the user pagetable. That's
    // needed because the kernel sets satp while still executing its own code,
    // and if it weren't mapped as executable, it would page fault. It's safe
    // to do because the userland will not have access to anything mapped for
    // supervisor access anyway.
    map_page_id(pagetable, &RAM_START, PERM_KDATA, pid);
    map_page_id(pagetable, KERNEL_CODE_START, PERM_KCODE, pid);
    void *trap_frame_page = (void*)PAGE_ROUND_DOWN(&trap_frame);
    map_page_id(pagetable, trap_frame_page, PERM_KDATA, pid);
    void *paged_memory_page = (void*)PAGE_ROUND_DOWN(&paged_memory);
    map_page_id(pagetable, paged_memory_page, PERM_KDATA, pid);

    // map rodata to user address space:
    map_range(pagetable, &rodata_start, &data_start,
        (void*)USR_VIRT(&rodata_start), PERM_URODATA, pid);

    // now map all userland code as user-executable:
    map_range(pagetable, &user_code_start, &rodata_start,
        (void*)USR_VIRT(&user_code_start), PERM_UCODE, pid);
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
                panic("pagetable subtable alloc");
                return;
            }
            memset(pagetable_next, PAGE_SIZE, 0);
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
        pagetable[vpn_n] = PHYS_TO_PTE(phys_addr) | perm | PTE_A | PTE_D | PTE_V;
        break;
    }
}

// map_range calls map_page_sv39 for all pages in the given range.
void map_range(void *pagetable, void *pa_start, void *pa_end, void *va_start, int perm, uint32_t pid) {
    void *page_pa = pa_start;
    regsize_t va = (regsize_t)va_start;
    while (page_pa != pa_end) {
        map_page_sv39(pagetable, page_pa, va, perm, pid);
        page_pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }
}

void free_page_table_r(regsize_t *pt, int level) {
    regsize_t *end = pt + PAGE_SIZE/sizeof(regsize_t);
    for (regsize_t *pte = pt; pte != end; pte++) {
        if (IS_NONLEAF(*pte)) {
            free_page_table_r(PTE_TO_PHYS(*pte), level+1);
            if (level < 2) {
                release_page(PTE_TO_PHYS(*pte));
                continue;
            }
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

void copy_page_table(regsize_t *dst, regsize_t *src, uint32_t pid) {
    int num_ptes = PAGE_SIZE/sizeof(regsize_t);
    for (int vpn2 = 0; vpn2 < num_ptes; vpn2++) {
        if (!IS_VALID(src[vpn2])) {
            continue;
        }
        regsize_t *src2 = PTE_TO_PHYS(src[vpn2]);
        for (int vpn1 = 0; vpn1 < num_ptes; vpn1++) {
            if (!IS_VALID(src2[vpn1])) {
                continue;
            }
            regsize_t *src3 = PTE_TO_PHYS(src2[vpn1]);
            for (int vpn0 = 0; vpn0 < num_ptes; vpn0++) {
                regsize_t pte = src3[vpn0];
                if (IS_VALID(pte) && IS_USER(pte)) {
                    void *pa = PTE_TO_PHYS(pte);
                    int perm = PERM_MASK(pte);
                    regsize_t va = (vpn2 << 30) | (vpn1 << 21) | (vpn0 << 12);
                    map_page_sv39(dst, pa, va, perm, pid);
                }
            }
        }
    }
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
