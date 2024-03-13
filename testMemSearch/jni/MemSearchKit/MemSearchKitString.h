#ifndef MEM_SEARCH_STRING_CORE_H_
#define MEM_SEARCH_STRING_CORE_H_
#include <stdio.h>
#include <string.h>
namespace MemorySearchKit {

	namespace String {

		static inline std::string& replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value) {
			for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
				if ((pos = str.find(old_value, pos)) != std::string::npos) {
					str.replace(pos, old_value.length(), new_value);
				} else {
					break;
				}
			}
			return str;
		}

	}

}
#endif /* MEM_SEARCH_STRING_CORE_H_ */

