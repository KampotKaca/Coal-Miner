#include "terrain.h"

typedef struct BlockData
{
	float chance;
	unsigned char spawnedBlock;
}BlockData;

static void GenerateDirt(float caveValue, unsigned char yIndex, unsigned char maxHeight);
static void GenerateStone(float caveValue, unsigned char yIndex, unsigned char maxHeight);

BlockData blockData[BLOCK_GEN_COUNT] =
{
	(BlockData)
	{
		0.85f,
		BLOCK_STONE,
	},
	(BlockData)
	{
		0.1f,
		BLOCK_DIRT,
	},
	(BlockData)
	{
		0.05f,
		BLOCK_IRON,
	}
};

void load_block_types()
{

}

unsigned char get_block_type(float caveValue)
{
	float currentValue = 0;
	for (int i = 0; i < BLOCK_GEN_COUNT; ++i)
	{
		currentValue += blockData[i].chance;
		if(currentValue >= caveValue) return blockData[i].spawnedBlock;
	}
	return blockData[BLOCK_GEN_COUNT - 1].spawnedBlock;
}