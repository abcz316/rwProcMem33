# ifndef __CVECTOR_H__
# define __CVECTOR_H__

#include <linux/types.h>
# define MIN_LEN 1024
# define CVEFAILED  -1
# define CVESUCCESS  0
# define CVEPUSHBACK 1
# define CVEPOPBACK  2
# define CVEINSERT   3
# define CVERM       4
# define EXPANED_VAL 1
# define REDUSED_VAL 2

typedef void *citerator;
typedef struct _cvector *cvector;

# ifdef __cplusplus
extern "C" {
# endif
	
	cvector   cvector_create(const size_t size                           );
	void      cvector_destroy  (const cvector cv                            );
	size_t    cvector_length   (const cvector cv                            );
	int       cvector_pushback (const cvector cv, void *memb                );
	int       cvector_popback  (const cvector cv, void *memb                );
	size_t    cvector_iter_at  (const cvector cv, citerator iter            );
	int       cvector_iter_val (const cvector cv, citerator iter, void *memb);
	citerator cvector_begin    (const cvector cv                            );
	citerator cvector_end      (const cvector cv                            );
	citerator cvector_next     (const cvector cv, citerator iter            );
	int       cvector_val_at   (const cvector cv, size_t index, void *memb  );
	int       cvector_insert   (const cvector cv, citerator iter, void *memb);
	int       cvector_insert_at(const cvector cv, size_t index, void *memb  );
	int       cvector_rm       (const cvector cv, citerator iter            );
	int       cvector_rm_at    (const cvector cv, size_t index              );
	
	/* for test  */
	void      cv_info          (const cvector cv                            );
	void      cv_print         (const cvector cv                            );

# ifdef __cplusplus
}
# endif

#endif /* EOF file cvector.h */


 
// #include "cvector.h"
// 
// int main()
// {
// 	int i = 1;
// 	cvector cv = cvector_create(sizeof(int));
// 	cvector_pushback(cv, &i);
// 	cvector_pushback(cv, &i);
// 	cvector_pushback(cv, &i);
// 	cvector_pushback(cv, &i);
// 	cv_print(cv);
// 	cvector_destroy(cv);
// 	return 0;
// }




#include "api_proxy.h"
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#ifndef __gnu_linux__
//#define __func__ "unknown"
//#define inline __forceinline
#endif

# define CWARNING_ITER(cv, iter, file, func, line) \
	do {\
	if ((cvector_begin(cv) > iter) || (cvector_end(cv) <= iter)) {\
	printk(KERN_INFO "var(" #iter ") warng out of range, "\
	"at file:%s func:%s line:%d!!\n", file, func, line);\
	return CVEFAILED;\
	}\
	} while (0)

void *vmalloc_realloc(void *old_ptr, size_t old_size, size_t new_size) {
    void *new_ptr;
    if (!old_ptr) {
        return vmalloc(new_size);
    }
    if (new_size == 0) {
        vfree(old_ptr);
        return NULL;
    }
    new_ptr = vmalloc(new_size);
    if (!new_ptr) {
        pr_err("vmalloc realloc failed for size: %zu\n", new_size);
        return NULL;
    }
    memcpy(new_ptr, old_ptr, min(old_size, new_size));
    vfree(old_ptr);
    return new_ptr;
}

struct _cvector
{
	void *cv_pdata;
	size_t cv_len, cv_tot_len, cv_size;
};


cvector cvector_create(const size_t size)
{
	cvector cv = (cvector)x_kmalloc(sizeof (struct _cvector), GFP_KERNEL);

	if (!cv) return NULL;

	cv->cv_pdata = vmalloc(MIN_LEN * size);

	if (!cv->cv_pdata)
	{
		kfree(cv);
		return NULL;
	}

	cv->cv_size = size;
	cv->cv_tot_len = MIN_LEN;
	cv->cv_len = 0;

	return cv;
}  

void cvector_destroy(const cvector cv)  
{  
	vfree(cv->cv_pdata);  
	kfree(cv);
	return;  
}  

size_t cvector_length(const cvector cv)  
{  
	return cv->cv_len;  
}  

int cvector_pushback(const cvector cv, void *memb)  
{  
	if (cv->cv_len >= cv->cv_tot_len)   
	{  
		void *pd_sav = cv->cv_pdata;
		size_t old_size = cv->cv_tot_len * cv->cv_size;
		cv->cv_tot_len <<= EXPANED_VAL;
		cv->cv_pdata = vmalloc_realloc(cv->cv_pdata, old_size, cv->cv_tot_len * cv->cv_size);

		if (!cv->cv_pdata)   
		{  
			cv->cv_pdata = pd_sav;  
			cv->cv_tot_len >>= EXPANED_VAL;  
			return CVEPUSHBACK;  
		}  
	}  

	memcpy((char *)cv->cv_pdata + cv->cv_len * cv->cv_size, memb, cv->cv_size);  
	cv->cv_len++;  

	return CVESUCCESS;  
}  

int cvector_popback(const cvector cv, void *memb)  
{  
	if (cv->cv_len <= 0) return CVEPOPBACK;  

	cv->cv_len--;  
	memcpy(memb, (char *)cv->cv_pdata + cv->cv_len * cv->cv_size, cv->cv_size);  

	if ((cv->cv_tot_len >= (MIN_LEN << REDUSED_VAL))   
		&& (cv->cv_len <= (cv->cv_tot_len >> REDUSED_VAL)))   
	{  
		void *pd_sav = cv->cv_pdata;
		size_t old_size = cv->cv_tot_len * cv->cv_size;
		cv->cv_tot_len >>= EXPANED_VAL;
		cv->cv_pdata = vmalloc_realloc(cv->cv_pdata, old_size, cv->cv_tot_len * cv->cv_size);

		if (!cv->cv_pdata)   
		{  
			cv->cv_tot_len <<= EXPANED_VAL;  
			cv->cv_pdata = pd_sav;  
			return CVEPOPBACK;  
		}  
	}  

	return CVESUCCESS;  
}  

size_t cvector_iter_at(const cvector cv, citerator iter)  
{  
	CWARNING_ITER(cv, iter, __FILE__, __func__, __LINE__);  
	return ((char *)iter - (char *)cv->cv_pdata) / cv->cv_size;  
}  

int cvector_iter_val(const cvector cv, citerator iter, void *memb)  
{  
	CWARNING_ITER(cv, iter, __FILE__, __func__, __LINE__);  
	memcpy(memb, iter, cv->cv_size);  
	return 0;  
}  

citerator cvector_begin(const cvector cv)  
{  
	return cv->cv_pdata;  
}  

citerator cvector_end(const cvector cv)  
{  
	return (char *)cv->cv_pdata + (cv->cv_size * cv->cv_len);  
}  

static inline void cvmemove_foreward(const cvector cv, void *from, void *to)  
{  
	size_t size = cv->cv_size;  
	char *p;  
	for (p = (char *)to; p >= (char *)from; p -= size) memcpy(p + size, p, size);  
	return;  
}  

static inline void cvmemove_backward(const cvector cv, void *from, void *to)  
{  
	memcpy(from, (char *)from + cv->cv_size, (char *)to - (char *)from);  
	return;  
}  

int cvector_insert(const cvector cv, citerator iter, void *memb)  
{  
	CWARNING_ITER(cv, iter, __FILE__, __func__, __LINE__);  

	if (cv->cv_len >= cv->cv_tot_len)   
	{  
		void *pd_sav = cv->cv_pdata;
		size_t old_size = cv->cv_tot_len * cv->cv_size;
		cv->cv_tot_len <<= EXPANED_VAL;  
		cv->cv_pdata = vmalloc_realloc(cv->cv_pdata, old_size, cv->cv_tot_len * cv->cv_size);

		if (!cv->cv_pdata)   
		{  
			cv->cv_pdata = pd_sav;  
			cv->cv_tot_len >>= EXPANED_VAL;  
			return CVEINSERT;  
		}  
	}  

	cvmemove_foreward(cv, iter, (char *)cv->cv_pdata + cv->cv_len * cv->cv_size);  
	memcpy(iter, memb, cv->cv_size);  
	cv->cv_len++;  

	return CVESUCCESS;  
}  

int cvector_insert_at(const cvector cv, size_t index, void *memb)  
{  
	citerator iter;  

	if (index >= cv->cv_tot_len)   
	{  
		// 保存旧大小
		size_t old_size = cv->cv_tot_len * cv->cv_size;
		cv->cv_len = index + 1;  
		while (cv->cv_len >= cv->cv_tot_len) cv->cv_tot_len <<= EXPANED_VAL;  
		cv->cv_pdata = vmalloc_realloc(cv->cv_pdata, old_size, cv->cv_tot_len * cv->cv_size);
		iter = (char *)cv->cv_pdata + cv->cv_size * index;  
		memcpy(iter, memb, cv->cv_size);  
	}  
	else   
	{  
		iter = (char *)cv->cv_pdata + cv->cv_size * index;  
		cvector_insert(cv, iter, memb);  
	}  

	return 0;  
}  

citerator cvector_next(const cvector cv, citerator iter)  
{  
	return (char *)iter + cv->cv_size;  
}  

int cvector_val(const cvector cv, citerator iter, void *memb)  
{  
	memcpy(memb, iter, cv->cv_size);  
	return 0;  
}  

int cvector_val_at(const cvector cv, size_t index, void *memb)  
{  
	memcpy(memb, (char *)cv->cv_pdata + index * cv->cv_size, cv->cv_size);  
	return 0;  
}  

int cvector_rm(const cvector cv, citerator iter)  
{  
	citerator from;  
	citerator end;  
	CWARNING_ITER(cv, iter, __FILE__, __func__, __LINE__);  
	from = iter;  
	end = cvector_end(cv);  
	memcpy(from, (char *)from + cv->cv_size, (char *)end - (char *)from);  
	cv->cv_len--;  

	if ((cv->cv_tot_len >= (MIN_LEN << REDUSED_VAL))  
		&& (cv->cv_len <= (cv->cv_tot_len >> REDUSED_VAL)))   
	{  
		void *pd_sav = cv->cv_pdata;
		size_t old_size = cv->cv_tot_len * cv->cv_size;
		cv->cv_tot_len >>= EXPANED_VAL;  
		cv->cv_pdata = vmalloc_realloc(cv->cv_pdata, old_size, cv->cv_tot_len * cv->cv_size);

		if (!cv->cv_pdata)   
		{  
			cv->cv_tot_len <<= EXPANED_VAL;  
			cv->cv_pdata = pd_sav;  
			return CVERM;  
		}  
	}  

	return CVESUCCESS;  
}  

int cvector_rm_at(const cvector cv, size_t index)  
{  
	citerator iter;  
	iter = (char *)cv->cv_pdata + cv->cv_size * index;  
	CWARNING_ITER(cv, iter, __FILE__, __func__, __LINE__);  
	return cvector_rm(cv, iter);  
}  

void cv_info(const cvector cv)  
{  
	printk("\n\ntot :%s : %zu\n", __func__, cv->cv_tot_len);  
	printk("len :%s : %zu\n",     __func__, cv->cv_len);  
	printk("size:%s : %zu\n\n",   __func__, cv->cv_size);  
	return;  
}  

void cv_print(const cvector cv)  
{  
	int num;  
	citerator iter;  

	if (cvector_length(cv) == 0)  
		printk(KERN_INFO "file:%s func:%s line:%d error, null length cvector!!\n", __FILE__, __func__, __LINE__);  

	for (iter = cvector_begin(cv);   
		iter != cvector_end(cv);  
		iter = cvector_next(cv, iter))   
	{  
		cvector_iter_val(cv, iter, &num);  
		printk("var:%d at:%zu\n", num, cvector_iter_at(cv, iter));  
	}


	return;  
}
