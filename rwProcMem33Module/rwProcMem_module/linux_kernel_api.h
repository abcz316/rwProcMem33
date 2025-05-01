#ifndef LINUX_KERNEL_API_H_
#define LINUX_KERNEL_API_H_
#include "ver_control.h"

#include <linux/module.h>
#ifdef CONFIG_USE_DEV_FILE_NODE
#include <linux/cdev.h>
#include <linux/device.h>
#endif

#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0)
 
long probe_kernel_read(void* dst, const void* src, size_t size);
 
MY_STATIC long x_probe_kernel_read(void* bounce, const char* ptr, size_t sz) {
    return probe_kernel_read(bounce, ptr, sz);
}
 
#endif
 
#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
 
long copy_from_kernel_nofault(void* dst, const void* src, size_t size);
 
MY_STATIC long x_probe_kernel_read(void* bounce, const char* ptr, size_t sz) {
    return copy_from_kernel_nofault(bounce, ptr, sz);
}
 
#endif


#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0)
MY_STATIC inline pte_t x_pte_mkwrite(pte_t pte) {
    return pte_mkwrite(pte);
}
#else
MY_STATIC inline pte_t x_pte_mkwrite(pte_t pte) {
    struct vm_area_struct vma = {.vm_flags = VM_READ};
    return pte_mkwrite(pte, &vma);
}
#endif

#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0)
MY_STATIC size_t x_read_mm_struct_rss(struct mm_struct * mm, ssize_t offset) {
        struct mm_rss_stat *rss_stat = (struct mm_rss_stat *)((size_t)&mm->rss_stat + offset);
        size_t total_rss;
		ssize_t val1, val2, val3;
		val1 = atomic_long_read(&rss_stat->count[MM_FILEPAGES]);
		val2 = atomic_long_read(&rss_stat->count[MM_ANONPAGES]);
#ifdef MM_SHMEMPAGES
		val3 = atomic_long_read(&rss_stat->count[MM_SHMEMPAGES]);
#else
		val3 = 0;
#endif
		if (val1 < 0) { val1 = 0; }
		if (val2 < 0) { val2 = 0; }
		if (val3 < 0) { val3 = 0; }
		total_rss = val1 + val2 + val3;
        return total_rss;
}
#else
MY_STATIC size_t x_read_mm_struct_rss(struct mm_struct * mm, ssize_t offset) {
        struct percpu_counter *rss_stat = (struct percpu_counter *)((size_t)&mm->rss_stat + offset);
        size_t total_rss;
		ssize_t val1, val2, val3;
		val1 = percpu_counter_read(&rss_stat[MM_FILEPAGES]);
        val2 = percpu_counter_read(&rss_stat[MM_ANONPAGES]);

        #ifdef MM_SHMEMPAGES
        val3 = percpu_counter_read(&rss_stat[MM_SHMEMPAGES]);
        #else
        val3 = 0;
        #endif
		if (val1 < 0) { val1 = 0; }
		if (val2 < 0) { val2 = 0; }
		if (val3 < 0) { val3 = 0; }
		total_rss = val1 + val2 + val3;
        return total_rss;
}
#endif


#ifdef CONFIG_USE_DEV_FILE_NODE

#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0)
struct class *x_class_create(const char *name) {
    return class_create(THIS_MODULE, name);
}
#else
struct class *x_class_create(const char *name) {
    return class_create(name);
}
#endif

#endif /* CONFIG_USE_DEV_FILE_NODE */

#endif /* LINUX_KERNEL_API_H_ */
