#ifndef CONTEXT_H_
#define CONTEXT_H_
#ifdef HAS_LINUX_USER_H
#include <linux/user.h>
#else
#include <sys/user.h>
#endif

#include <sys/ptrace.h>
#include <linux/socket.h>

#ifdef __aarch64__
#include <elf.h>
#endif



#include <sys/socket.h>


#ifdef __i386__
typedef struct user_regs_struct CONTEXT_REGS;
#endif

#ifdef __x86_64__
typedef struct user_regs_struct CONTEXT_REGS;
#endif


#ifdef __arm__
typedef struct pt_regs CONTEXT_REGS;
#endif


#ifdef __aarch64__
typedef struct user_pt_regs CONTEXT_REGS;
#endif

struct CONTEXT {
	CONTEXT_REGS regs;
};




#endif /* CONTEXT_H_ */