#include <stdio.h>
#include "player.h"
#include "chunks.h"
#include "chunkLoaderManager.h"
#include "input.h"

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

    if (!result) {
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
    return player->width * 0.5f;
}

int playerCollides(Player* player)
{
    float minX = player->position.x - playerHalfWidth(player);
    float maxX = player->position.x + playerHalfWidth(player);

    float minY = player->position.y;
    float maxY = player->position.y + player->height;

    float minZ = player->position.z - playerHalfWidth(player);
    float maxZ = player->position.z + playerHalfWidth(player);

    int voxelMinX = (int)round(minX);
    int voxelMaxX = (int)round(maxX);

    int voxelMinY = (int)round(minY);
    int voxelMaxY = (int)round(maxY);

    int voxelMinZ = (int)round(minZ);
    int voxelMaxZ = (int)round(maxZ);

    for (int x = voxelMinX; x <= voxelMaxX; x++)
    {
        for (int y = voxelMinY; y <= voxelMaxY; y++)
        {
            for (int z = voxelMinZ; z <= voxelMaxZ; z++)
            {
                if (isSolidVoxel(x, y, z))
                {
                    return 1;
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

