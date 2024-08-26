#include "terrain_meshing.h"
#include "coal_helper.h"

static void T_CreateTerrainChunkFaces(void* args);
static void T_TerrainChunkFacesCreationFinished(void* args);

static void T_TerrainCreateGroupFaces(void* args);
static void T_TerrainGroupFacesCreationFinished(void* args);

VoxelTerrain* m_terrain;

void setup_terrain_meshing(VoxelTerrain* terrain)
{
	m_terrain = terrain;
}

void send_terrain_face_creation_job(uint32_t x, uint32_t y, uint32_t z)
{
	TerrainChunk* chunk = &m_terrain->chunkGroups[x * TERRAIN_VIEW_RANGE + z].chunks[y];
	chunk->flags.state = CHUNK_CREATING_FACES;

	uint32_t * args = CM_MALLOC(3 * sizeof(uint32_t));
	args[0] = x;
	args[1] = y;
	args[2] = z;

	ThreadJob job = {0};
	job.args = args;
	job.job = T_CreateTerrainChunkFaces;
	job.callbackJob = T_TerrainChunkFacesCreationFinished;
	cm_submit_job(m_terrain->pool, job, true);
}

void send_terrain_group_face_creation_job(uint32_t x, uint32_t z)
{
	uint32_t* args = CM_MALLOC(2 * sizeof(uint32_t));
	args[0] = x;
	args[1] = z;

	ThreadJob job = {0};
	job.args = args;
	job.job = T_TerrainCreateGroupFaces;
	job.callbackJob = T_TerrainGroupFacesCreationFinished;

	TerrainChunkGroup* group = &m_terrain->chunkGroups[x * TERRAIN_VIEW_RANGE + z];

	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		group->chunks[y].flags.state = CHUNK_CREATING_FACES;

	cm_submit_job(m_terrain->pool, job, true);
}

//region thread callbacks
static void T_CreateTerrainChunkFaces(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	create_terrain_chunk_faces(cArgs[0], cArgs[1], cArgs[2]);
}

static void T_TerrainChunkFacesCreationFinished(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	m_terrain->chunkGroups[cArgs[0] * TERRAIN_VIEW_RANGE + cArgs[2]].chunks[cArgs[1]].flags.state = CHUNK_REQUIRES_UPLOAD;
}

static void T_TerrainCreateGroupFaces(void* args)
{
	uint32_t* cArgs = (uint32_t*)args;
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		create_terrain_chunk_faces(cArgs[0], y, cArgs[1]);
}

static void T_TerrainGroupFacesCreationFinished(void* args)
{
	uint32_t* cArgs = (uint32_t*)args;
	TerrainChunkGroup* group = &m_terrain->chunkGroups[cArgs[0] * TERRAIN_VIEW_RANGE + cArgs[1]];

	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		group->chunks[y].flags.state = CHUNK_REQUIRES_UPLOAD;
}
//endregion

//region faces

struct GreedySize
{
	uint32_t x;
	uint32_t y;
};

static inline uint32_t ToVoxelId(uint32_t x, uint32_t y, uint32_t z)
{
	return y * TERRAIN_CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE + z;
}

static inline void CreateFaceMask(const uint64_t* oMask, bool fVoxelExists, bool bVoxelExists,
                                  uint64_t* tf, uint64_t* tb, uint32_t id)
{
	uint64_t mask = oMask[id];
	tf[id] = (mask & ~(mask >> 1ull)) & ~((uint64_t)fVoxelExists << (TERRAIN_CHUNK_SIZE - 1ull));
	tb[id] = (mask & ~(mask << 1ull)) & ~(bVoxelExists);
}

static inline struct GreedySize GreedyMeshing
	(uint32_t x, uint32_t y, uint32_t offset,
	 uint64_t* currentFace)
{
	uint32_t sizeX = 1u, sizeY = 1u;
	uint64_t bitShift = 1llu << offset;

	uint32_t saEnd = glm_imin(x + TERRAIN_MAX_GREEDY_AXIS, TERRAIN_CHUNK_SIZE);
	for (uint32_t sa = x + 1u; sa < saEnd; ++sa)
	{
		uint32_t id = y * TERRAIN_CHUNK_SIZE + sa;
		uint64_t bitMap = currentFace[id];

		if(bitMap & bitShift)
		{
			sizeX++;
			currentFace[id] = bitMap & (~bitShift);
		}
		else break;
	}

	uint32_t laEnd = glm_imin(y + TERRAIN_MAX_GREEDY_AXIS, TERRAIN_CHUNK_SIZE);
	saEnd = x + sizeX;
	for (uint32_t la = y + 1u; la < laEnd; ++la)
	{
		for (uint32_t sa = x; sa < saEnd; ++sa)
		{
			if((currentFace[la * TERRAIN_CHUNK_SIZE + sa] & bitShift) == 0u)
				goto end;
		}

		for (uint32_t sa = x; sa < saEnd; ++sa)
			currentFace[la * TERRAIN_CHUNK_SIZE + sa] &= (~bitShift);

		sizeY++;
	}

	end:
	return (struct GreedySize){ .x = sizeX, .y = sizeY };
};

static inline void AddFace(List* list,
						   uint32_t x, uint32_t y, uint32_t z,
						   struct GreedySize size,
						   uint32_t faceId)
{
	uint32_t mainBlock = (x << 12u) | (y << 6u) | z;
	mainBlock <<= 12;
	mainBlock |= ((size.x - 1) << 6u) | (size.y - 1);
	mainBlock <<= 2;

	uint32_t faceBlock = faceId;
	faceBlock <<= 2;

	uint32_t buffer[TERRAIN_MEM_PRINT_SIZE] =
	{
		mainBlock,
		faceBlock | 0b00,
		mainBlock,
		faceBlock | 0b01,
		mainBlock,
		faceBlock | 0b11,
		mainBlock,
		faceBlock | 0b00,
		mainBlock,
		faceBlock | 0b11,
		mainBlock,
		faceBlock | 0b10
	};
	list_add(list, 32, buffer, sizeof(buffer));
}

void create_terrain_chunk_faces(uint32_t xId, uint32_t yId, uint32_t zId)
{
	uint32_t faceCount = 0;
	TerrainChunkGroup* group = &m_terrain->chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	TerrainChunk* chunk = &group->chunks[yId];
	list_reset(&chunk->buffer);

	//region MaskCreation
	uint8_t* voxels = chunk->voxels;
	uint64_t fbMask[TERRAIN_CHUNK_HORIZONTAL_SLICE],
		     rlMask[TERRAIN_CHUNK_HORIZONTAL_SLICE],
		     tbMask[TERRAIN_CHUNK_HORIZONTAL_SLICE];
	memset(fbMask, 0, TERRAIN_CHUNK_HORIZONTAL_SLICE * sizeof(uint64_t));
	memset(rlMask, 0, TERRAIN_CHUNK_HORIZONTAL_SLICE * sizeof(uint64_t));
	memset(tbMask, 0, TERRAIN_CHUNK_HORIZONTAL_SLICE * sizeof(uint64_t));

	for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
		{
			for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				if(voxels[ToVoxelId(x, y, z)] != BLOCK_EMPTY)
				{
					fbMask[y * TERRAIN_CHUNK_SIZE + x] |= 1llu << z;
					rlMask[z * TERRAIN_CHUNK_SIZE + y] |= 1llu << x;
					tbMask[x * TERRAIN_CHUNK_SIZE + z] |= 1llu << y;
				}
			}
		}
	}
	//endregion

	uint8_t *frontChunk = NULL, *backChunk = NULL,
		    *rightChunk = NULL, *leftChunk = NULL,
		    *topChunk = NULL, *bottomChunk = NULL;

	if(zId < TERRAIN_VIEW_RANGE - 1) frontChunk = m_terrain->chunkGroups[xId * TERRAIN_VIEW_RANGE + zId + 1].chunks[yId].voxels;
	if(zId > 0) backChunk = m_terrain->chunkGroups[xId * TERRAIN_VIEW_RANGE + zId - 1].chunks[yId].voxels;
	if(xId < TERRAIN_VIEW_RANGE - 1) rightChunk = m_terrain->chunkGroups[(xId + 1) * TERRAIN_VIEW_RANGE + zId].chunks[yId].voxels;
	if(xId > 0) leftChunk = m_terrain->chunkGroups[(xId - 1) * TERRAIN_VIEW_RANGE + zId].chunks[yId].voxels;
	if(yId < TERRAIN_HEIGHT - 1) topChunk = group->chunks[yId + 1].voxels;
	if(yId > 0) bottomChunk = group->chunks[yId - 1].voxels;

	uint64_t fFaces[TERRAIN_CHUNK_HORIZONTAL_SLICE];
	uint64_t bFaces[TERRAIN_CHUNK_HORIZONTAL_SLICE];
	uint64_t *faces[6] = { fFaces, bFaces, fFaces, bFaces, fFaces, bFaces };

	//region Front&Back
	for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
	{
		for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			uint32_t id = y * TERRAIN_CHUNK_SIZE + x;
			bool frontVoxelExists = frontChunk == NULL || frontChunk[ToVoxelId(x, y, 0)];
			bool backVoxelExists = backChunk == NULL || backChunk[ToVoxelId(x, y, (TERRAIN_CHUNK_SIZE - 1))];
			CreateFaceMask(fbMask, frontVoxelExists, backVoxelExists, fFaces, bFaces, id);
		}
	}

	for (uint32_t i = 0; i < 2; ++i)
	{
		uint64_t* currentFace = faces[i];

		for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
		{
			for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
			{
				uint64_t mask = currentFace[y * TERRAIN_CHUNK_SIZE + x];
				while(mask != 0llu)
				{
					uint32_t z = cm_trailing_zeros(mask);
					mask &= mask - 1u;

					struct GreedySize size = GreedyMeshing(x, y, z, currentFace);
					AddFace(&chunk->buffer, x, y, z, size, i);
					faceCount++;
				}
			}
		}
	}
	//endregion

	//region Right&Left
	for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
	{
		for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			uint32_t id = y * TERRAIN_CHUNK_SIZE + x;
			bool frontVoxelExists = rightChunk == NULL || rightChunk[ToVoxelId(0, x, y)];
			bool backVoxelExists = leftChunk == NULL || leftChunk[ToVoxelId((TERRAIN_CHUNK_SIZE - 1), x, y)];
			CreateFaceMask(rlMask, frontVoxelExists, backVoxelExists, fFaces, bFaces, id);
		}
	}

	for (uint32_t i = 2; i < 4; ++i)
	{
		uint64_t* currentFace = faces[i];

		for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
			{
				uint64_t mask = currentFace[z * TERRAIN_CHUNK_SIZE + y];
				while(mask != 0llu)
				{
					uint32_t x = cm_trailing_zeros(mask);
					mask &= mask - 1;

					struct GreedySize size = GreedyMeshing(y, z, x, currentFace);
					AddFace(&chunk->buffer, x, y, z, size, i);
					faceCount++;
				}
			}
		}
	}
	//endregion

	//region Top&Bottom
	for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
	{
		for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			uint32_t id = y * TERRAIN_CHUNK_SIZE + x;
			bool frontVoxelExists = topChunk == NULL || topChunk[ToVoxelId(y, 0, x)];
			bool backVoxelExists = bottomChunk == NULL || bottomChunk[ToVoxelId(y, (TERRAIN_CHUNK_SIZE - 1), x)];
			CreateFaceMask(tbMask, frontVoxelExists, backVoxelExists, fFaces, bFaces, id);
		}
	}

	for (uint32_t i = 4; i < 6; ++i)
	{
		uint64_t* currentFace = faces[i];

		for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				uint64_t mask = currentFace[x * TERRAIN_CHUNK_SIZE + z];
				while(mask != 0llu)
				{
					uint32_t y = cm_trailing_zeros(mask);
					mask &= mask - 1;

					struct GreedySize size = GreedyMeshing(z, x, y, currentFace);
					AddFace(&chunk->buffer, x, y, z, size, i);
					faceCount++;
				}
			}
		}
	}
	//endregion

#undef RECT_FACE
	chunk->flags.faceCount = faceCount;
	m_terrain->chunkVaos[group->ssboId * TERRAIN_HEIGHT + yId].vbo.vertexCount = faceCount * TERRAIN_MEM_PRINT_SIZE;

#undef BUFFER_CHECK
}

//endregion