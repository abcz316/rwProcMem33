#ifndef PROC_ROOT_AUTO_OFFSET_H_
#define PROC_ROOT_AUTO_OFFSET_H_
#include <linux/ctype.h>
#include "ver_control.h"

static ssize_t g_real_cred_offset = 0;
static bool g_init_real_cred_offset_success = false;

static inline int init_proc_root_offset(const char* my_name) {
	const ssize_t offset_lookup_min = -100;
	const ssize_t offset_lookup_max = 300;

	const ssize_t min_real_cred_offset_limit = offset_lookup_min + sizeof(void*) * 3;

	if(g_init_real_cred_offset_success) {
		return 0;
	}
	
	g_init_real_cred_offset_success = false;
	for (g_real_cred_offset = offset_lookup_min; g_real_cred_offset <= offset_lookup_max; g_real_cred_offset++) {

		char* pcomm = (char*)&current->real_cred;
		pcomm += g_real_cred_offset;

		printk_debug(KERN_EMERG "curent g_real_cred_offset:%zd, bytes:%x\n", g_real_cred_offset, *(unsigned char*)pcomm);

		if(g_real_cred_offset < min_real_cred_offset_limit) {
			continue;
		}
		if (strcmp(pcomm, my_name) == 0) {
			ssize_t maybe_real_cred_offset = g_real_cred_offset - sizeof(void*) * 2;
			char * p_test_mem1 = (char*)&current->real_cred + maybe_real_cred_offset;
			char * p_test_mem2 = (char*)&current->real_cred + maybe_real_cred_offset + sizeof(void*); // for get cred *cred;
			if(memcmp(p_test_mem1, p_test_mem2, sizeof(void*)) != 0 ) { // becasuse the real_cred is equal the cred
				maybe_real_cred_offset = g_real_cred_offset - sizeof(void*) * 3;
				p_test_mem1 = (char*)&current->real_cred + maybe_real_cred_offset;
				p_test_mem2 = (char*)&current->real_cred + maybe_real_cred_offset + sizeof(void*); // for get cred *cred;
				if(memcmp(p_test_mem1, p_test_mem2, sizeof(void*)) != 0 ) { // becasuse the real_cred is equal the cred
					break; // failed
				}
			}
			g_real_cred_offset = maybe_real_cred_offset;

			printk_debug(KERN_EMERG "strcmp found %zd\n", g_real_cred_offset);

			g_init_real_cred_offset_success = true;
			break;
		}

	}

	if (!g_init_real_cred_offset_success) {
		printk_debug(KERN_INFO "real_cred offset failed\n");
		return -ESPIPE;
	}
	printk_debug(KERN_INFO "g_real_cred_offset:%zu\n", g_real_cred_offset);
	return 0;
}
#endif /* PROC_ROOT_AUTO_OFFSET_H_ */


