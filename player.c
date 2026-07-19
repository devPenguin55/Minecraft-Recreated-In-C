#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "chunks.h"
#include "chunkLoaderManager.h"
#include "input.h"

Chunk *chunkAtPosition(int voxelX, int voxelY, int voxelZ) {
    int playerChunkX = (int)floor(player.position.x / (ChunkWidthX * BlockWidthX));
    int playerChunkZ = (int)floor(player.position.z / (ChunkLengthZ * BlockLengthZ));

    int chunkX = (voxelX >= 0)
        ? voxelX / ChunkWidthX
        : (voxelX - (ChunkWidthX - 1)) / ChunkWidthX;

    int chunkZ = (voxelZ >= 0)
        ? voxelZ / ChunkLengthZ
        : (voxelZ - (ChunkLengthZ - 1)) / ChunkLengthZ;

    if (
        (chunkX > (playerChunkX + CHUNK_PRELOAD_RADIUS) || (chunkX < (playerChunkX - CHUNK_PRELOAD_RADIUS))) ||
        (chunkZ > (playerChunkZ + CHUNK_PRELOAD_RADIUS) || (chunkZ < (playerChunkZ - CHUNK_PRELOAD_RADIUS))))
    { 
        return NULL;
    }

    
    uint64_t chunkKey = packChunkKey(chunkX, chunkZ);
    BucketEntry* result = getHashmapEntry(chunkKey);

    if (result == NULL) {
        return NULL;
    }

    Chunk* chunk = result->chunkEntry;
    return chunk;
}

Block *blockAtPosition(int voxelX, int voxelY, int voxelZ) {
    int chunkX = (voxelX >= 0)
        ? voxelX / ChunkWidthX
        : (voxelX - (ChunkWidthX - 1)) / ChunkWidthX;

    int chunkZ = (voxelZ >= 0)
        ? voxelZ / ChunkLengthZ
        : (voxelZ - (ChunkLengthZ - 1)) / ChunkLengthZ;

    int localX = voxelX - chunkX * ChunkWidthX;
    int localY = voxelY;
    int localZ = voxelZ - chunkZ * ChunkLengthZ;

    uint64_t chunkKey = packChunkKey(chunkX, chunkZ);
    BucketEntry* result = getHashmapEntry(chunkKey);

    if (result == NULL) {
        return NULL;
    }

    Chunk* chunk = result->chunkEntry;

    int index =
    localX +
    ChunkWidthX * localZ +
    (ChunkWidthX * ChunkLengthZ) * localY;
    
    if (index < 0 ||
        index >= ChunkWidthX * ChunkLengthZ * ChunkHeightY)
        {
            return NULL;
        }
        
    return &chunk->blocks[index];
}

int isSolidVoxel(int voxelX, int voxelY, int voxelZ)
{
    Block* block = blockAtPosition(voxelX, voxelY, voxelZ);

    if (block == NULL) { return 0; }

    return (
        blockRegistry[block->blockType].isPhysicsSolid &&
        !block->isAir
    );
}

float playerHalfWidth(Player* player)
{
    return 0;
    // return player->width * 0.5f;
}

int playerCollides(Player* player)
{
    float minX = player->position.x - playerHalfWidth(player);
    float maxX = player->position.x + playerHalfWidth(player);

    float minY = player->position.y;
    float maxY = player->position.y + player->height;
 
    float minZ = player->position.z - playerHalfWidth(player);
    float maxZ = player->position.z + playerHalfWidth(player);

    int voxelMinX = (int)floor(minX);
    int voxelMaxX = (int)floor(maxX);

    int voxelMinY = (int)floor(minY);
    int voxelMaxY = (int)floor(maxY);

    int voxelMinZ = (int)floor(minZ);
    int voxelMaxZ = (int)floor(maxZ);


    for (int x = voxelMinX; x <= voxelMaxX; x++)
    {
        for (int y = voxelMinY; y <= voxelMaxY; y++)
        {
            for (int z = voxelMinZ; z <= voxelMaxZ; z++)
            {
                if (isSolidVoxel(x, y, z))
                {
                    Block *block = blockAtPosition(x, y, z);
                    if (block->isSlope == 0) { return 1; }

                    float localWithinBlockX = player->position.x-(float)x;
                    float localWithinBlockY = player->position.y-(float)y;
                    float localWithinBlockZ = player->position.z-(float)z;
                    
                    float height;
                    switch (block->isSlope) {
                        case 1:
                            height = 1-localWithinBlockZ;
                            break;
                        case 4:
                            height = localWithinBlockX;
                            break;
                        case 3:
                            height = localWithinBlockZ;
                            break;
                        case 2:
                            height = 1-localWithinBlockX;
                            break;
                    }
        
                    height += y;
                    if (player->position.y < height) {
                        player->position.y = height + DELTA_TIME;
                        return 0;
                    }
 
                    return 0;
                }

            }
        }
    }

    return 0;
}

void updatePlayerPhysics(Player* player)
{
    const float gravity = (player->isInWater) ? (5.0f) : (20.0f);


    player->velocity.y -= gravity * DELTA_TIME;

    player->isOnGround = 0;

  
    player->position.x += player->velocity.x * DELTA_TIME;
    if (playerCollides(player))
    {
        player->position.x -= player->velocity.x * DELTA_TIME;
        player->velocity.x = 0;
    }

    player->position.y += player->velocity.y * DELTA_TIME;
    if (playerCollides(player))
    {
        if (player->velocity.y > 0)
        {
            while (playerCollides(player))
            {
                player->position.y -= 0.001f;
            }
        }
        else if (player->velocity.y < 0)
        {
            while (playerCollides(player))
            {
                player->position.y += 0.001f;
            }

            player->isOnGround = 1;
        }

        player->velocity.y = 0;
    }

    player->position.z += player->velocity.z * DELTA_TIME;

    if (playerCollides(player))
    {
        player->position.z -= player->velocity.z * DELTA_TIME;
        player->velocity.z = 0;
    }
}

