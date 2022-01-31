#include <cstdio>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <thread>
#include <sstream>
#include <cinttypes>


int g_value = 0;

void AddValueThread() {
	while (1) {
		g_value++;
		printf("pid:%d, g_value addr:0x%llx, g_value: %d\n", getpid(), &g_value, g_value);
		fflush(stdout);
		sleep(5);
	}
	return;
}


int main(int argc, char *argv[]) {
	//启动数值增加线程
	std::thread tdValue(AddValueThread);
	tdValue.detach();

	while (1) {
		sleep(0);
	}
	return 0;
}