#ifndef WORLD_DISK_STORAGE
#define WORLD_DISK_STORAGE
#include "chunks.h"

void initWorldDiskStorage();
void saveChunkToDisk(Chunk *chunk);
void fetchChunkFromDisk(int chunkX, int chunkZ, Chunk *writeChunk);

#endif