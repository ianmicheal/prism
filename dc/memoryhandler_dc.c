#include "../include/memoryhandler.h"

#include "../include/math.h"

int getAvailableTextureMemory() {
	return pvr_mem_available();
}
