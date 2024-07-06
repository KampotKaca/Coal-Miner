#include "terrain.h"
#include "terrainConfig.h"

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
		0.5f,
		BLOCK_STONE,
	},
	(BlockData)
	{
		0.3f,
		BLOCK_DIRT,
	},
	(BlockData)
	{
		0.2f,
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

//region noise
fnl_state get_cave_noise()
{
	fnl_state noise = fnlCreateState();
	noise.seed = rand() % 10000000;
	noise.noise_type = TERRAIN_CAVE_NOISE;
	noise.fractal_type = TERRAIN_CAVE_FRACTAL;
	noise.frequency = TERRAIN_CAVE_FREQUENCY;
	noise.octaves = TERRAIN_CAVE_OCTAVES;
	noise.gain = TERRAIN_CAVE_GAIN;
	noise.lacunarity = TERRAIN_CAVE_LACUNARITY;
	noise.ping_pong_strength = TERRAIN_CAVE_PING_PONG_STRENGTH;
	return noise;
}

fnl_state get_biome_flat_noise()
{
	fnl_state noise = fnlCreateState();
	noise.seed = rand() % 10000000;
	noise.noise_type = TERRAIN_FLAT_NOISE;
	noise.fractal_type = TERRAIN_FLAT_FRACTAL;
	noise.frequency = TERRAIN_FLAT_FREQUENCY;
	noise.octaves = TERRAIN_FLAT_OCTAVES;
	noise.gain = TERRAIN_FLAT_GAIN;
	noise.lacunarity = TERRAIN_FLAT_LACUNARITY;
	return noise;
}

fnl_state get_biome_small_hill_noise()
{
	fnl_state noise = fnlCreateState();
	noise.seed = rand() % 10000000;
	noise.noise_type = TERRAIN_FLAT_NOISE;
	noise.fractal_type = TERRAIN_FLAT_FRACTAL;
	noise.frequency = TERRAIN_FLAT_FREQUENCY;
	noise.octaves = TERRAIN_FLAT_OCTAVES;
	noise.gain = TERRAIN_FLAT_GAIN;
	noise.lacunarity = TERRAIN_FLAT_LACUNARITY;
	return noise;
}

fnl_state get_biome_hill_noise()
{
	fnl_state noise = fnlCreateState();
	noise.seed = rand() % 10000000;
	noise.noise_type = TERRAIN_SMALL_HILL_NOISE;
	noise.fractal_type = TERRAIN_SMALL_HILL_FRACTAL;
	noise.frequency = TERRAIN_SMALL_HILL_FREQUENCY;
	noise.octaves = TERRAIN_SMALL_HILL_OCTAVES;
	noise.gain = TERRAIN_SMALL_HILL_GAIN;
	noise.lacunarity = TERRAIN_SMALL_HILL_LACUNARITY;
	return noise;
}

fnl_state get_biome_mountain_noise()
{
	fnl_state noise = fnlCreateState();
	noise.seed = rand() % 10000000;
	noise.noise_type = TERRAIN_MOUNTAIN_NOISE;
	noise.fractal_type = TERRAIN_MOUNTAIN_FRACTAL;
	noise.frequency = TERRAIN_MOUNTAIN_FREQUENCY;
	noise.octaves = TERRAIN_MOUNTAIN_OCTAVES;
	noise.gain = TERRAIN_MOUNTAIN_GAIN;
	noise.lacunarity = TERRAIN_MOUNTAIN_LACUNARITY;
	return noise;
}

fnl_state get_biome_high_mountain_noise()
{
	fnl_state noise = fnlCreateState();
	noise.seed = rand() % 10000000;
	noise.noise_type = TERRAIN_HIGH_MOUNTAIN_NOISE;
	noise.fractal_type = TERRAIN_HIGH_MOUNTAIN_FRACTAL;
	noise.frequency = TERRAIN_HIGH_MOUNTAIN_FREQUENCY;
	noise.octaves = TERRAIN_HIGH_MOUNTAIN_OCTAVES;
	noise.gain = TERRAIN_HIGH_MOUNTAIN_GAIN;
	noise.lacunarity = TERRAIN_HIGH_MOUNTAIN_LACUNARITY;
	return noise;
}
//endregion
