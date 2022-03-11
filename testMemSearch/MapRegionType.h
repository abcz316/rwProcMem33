//
// Created by abcz316 on 2020/2/26.
//

#ifndef MEMSEARCHER_MAPREGIONTYPE_H
#define MEMSEARCHER_MAPREGIONTYPE_H
#include "../testKo/MemoryReaderWriter37.h"
#ifndef __linux__
#include <windows.h>
#endif
static inline std::string MapsTypeToString(const DRIVER_REGION_INFO * rInfo) {
	std::string type;
	switch (rInfo->protection) {
	case PAGE_NOACCESS:
		type = "---";
		break;

	case PAGE_READONLY:
		type = "r--";
		break;

	case PAGE_READWRITE:
		type = "rw-";
		break;

	case PAGE_WRITECOPY:
		type = "-w-";
		break;

	case PAGE_EXECUTE:
		type = "--x";
		break;

	case PAGE_EXECUTE_READ:
		type = "r-x";
		break;

	case PAGE_EXECUTE_READWRITE:
		type = "rwx";
		break;

	default:
		break;
	}

	if (rInfo->type == MEM_PRIVATE) {
		type += "p";
	} else {
		type += "s";
	}
	return type;
}
static inline void StringToMapsType(const std::string & type, uint32_t & out_protection, uint32_t & out_type) {
	if (type.length() < 4) { return; }
	std::string head = type.substr(0, 3);
	if (head == "---") { out_protection = PAGE_NOACCESS; } else if (head == "r--") { out_protection = PAGE_READONLY; } else if (head == "rw-") { out_protection = PAGE_READWRITE; } else if (head == "-w-") { out_protection = PAGE_WRITECOPY; } else if (head == "--x") { out_protection = PAGE_EXECUTE; } else if (head == "r-x") { out_protection = PAGE_EXECUTE_READ; } else if (head == "rwx") { out_protection = PAGE_EXECUTE_READWRITE; }
	if (type[3] == 'p') { out_type = MEM_PRIVATE; } else if (type[3] == 's') { out_type = MEM_MAPPED; }
	return;
}
static inline int is__000(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_NOACCESS) {
		return 1;
	}
	return 0;
}
static inline int is_rw_p(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_READWRITE) {
		if (rInfo->type == MEM_PRIVATE) {
			return 1;
		}
	}
	return 0;
}
static inline int is_rw_s(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_READWRITE) {
		if (rInfo->type == MEM_MAPPED) {
			return 1;
		}
	}
	return 0;
}
static inline int is_rw00(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_READWRITE) {
		return 1;
	}
	return 0;
}
static inline int is_rw_0(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_READWRITE) {
		return 1;
	}
	return 0;
}
static inline int is_r__p(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_READONLY) {
		if (rInfo->type == MEM_PRIVATE) {
			return 1;
		}
	}
	return 0;
}
static inline int is_r__s(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_READONLY) {
		if (rInfo->type == MEM_MAPPED) {
			return 1;
		}
	}
	return 0;
}
static inline int is_r0xp(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_EXECUTE_READ || rInfo->protection == PAGE_EXECUTE_READWRITE) {
		if (rInfo->type == MEM_PRIVATE) {
			return 1;
		}
	}
	return 0;
}
static inline int is_r0_0(const DRIVER_REGION_INFO * rInfo) {
	if (
		(rInfo->protection != PAGE_EXECUTE_READ) &&
		(rInfo->protection != PAGE_EXECUTE_READWRITE) &&
		(rInfo->protection != PAGE_NOACCESS)
		) {
		return 1;
	}
	return 0;
}
static inline int is_r__0(const DRIVER_REGION_INFO * rInfo) {
	if (
		(rInfo->protection != PAGE_EXECUTE_READ) &&
		(rInfo->protection != PAGE_NOACCESS)
		) {
		return 1;
	}
	return 0;
}
static inline int is_0w_0(const DRIVER_REGION_INFO * rInfo) {
	return is_r0_0(rInfo);
}
static inline int is_r0_p(const DRIVER_REGION_INFO * rInfo) {
	if (
		(rInfo->protection == PAGE_READONLY) ||
		(rInfo->protection == PAGE_READWRITE)
		) {
		if (rInfo->type == MEM_PRIVATE) {
			return 1;
		}
	}
	return 0;
}
static inline int is_r_xp(const DRIVER_REGION_INFO * rInfo) {
	if (rInfo->protection == PAGE_EXECUTE_READ) {
		if (rInfo->type == MEM_PRIVATE) {
			return 1;
		}
	}
	return 0;
}

static inline int is_Ch(const DRIVER_REGION_INFO * rInfo) {
	if (is_rw_p(rInfo)) {
		if (strcmp(rInfo->name, "[heap]") == 0) {
			return 1;
		}
	}
	return 0;
}

static inline int is_Jh(const DRIVER_REGION_INFO * rInfo) {
	if (is_rw_p(rInfo)) {
		//if(strstr(rInfo->name,"/dev/ashmem/dalvik") != 0)
		if (strstr(rInfo->name, "/dev/ashmem/") != 0) {
			return 1;
		}
	}
	return 0;
}
static inline int is_S(const DRIVER_REGION_INFO * rInfo) {
	if (strstr(rInfo->name, "stack") != 0) {
		if (rInfo->name[0] == '[') {
			if (rInfo->name[strlen(rInfo->name) - 1] == ']') {
				return 1;
			}
		}
	}
	return 0;
}

static inline int is_A(const DRIVER_REGION_INFO * rInfo) {
	if (is_r0_0(rInfo)) {
		if (rInfo->name[0] == '\x00') {
			return 1;
		}
	}
	return 0;
}

static inline int is_As(const DRIVER_REGION_INFO * rInfo) {
	if (is_rw_s(rInfo)) {
		if (strstr(rInfo->name, "/dev/ashmem/") != 0) {
			if (strstr(rInfo->name, "/dev/ashmem/dalvik") == 0) {
				return 1;
			}
		}
	}
	return 0;
}


static inline int is_B(const DRIVER_REGION_INFO * rInfo) {
	if (is_r0_0(rInfo)) {
		if (strstr(rInfo->name, "/system/fonts/") != 0) { return 1; } else if (strstr(rInfo->name, "kgsl-3d0") != 0) { return 1; }
	}

	return 0;
}

static inline int is_Jb(const DRIVER_REGION_INFO * rInfo) {
	if (is_r0_p(rInfo)) {
		if (strstr(rInfo->name, "system@framework@boot.oat") != 0) { return 1; } else if (strstr(rInfo->name, "zygote") != 0) { return 1; } else if (strstr(rInfo->name, "Zygote") != 0) { return 1; }
	}
	return 0;
}


static inline int is_Xs(const DRIVER_REGION_INFO * rInfo) {
	if (strcmp(rInfo->name, "[vdso]") == 0) { return 1; }

	if (is_r_xp(rInfo)) {
		if (strstr(rInfo->name, "dalvik") != 0) { return 1; } else if (memcmp(rInfo->name, "/system", 7) == 0) { return 1; }
	}
	return 0;
}


#endif //MEMSEARCHER_MAPREGIONTYPE_H
