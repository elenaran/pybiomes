import pytest
from pybiomes import Finder, Generator, Pos
from pybiomes.biomes import plains
from pybiomes.structures import Village
from pybiomes.versions import MC_1_21_WD

@pytest.fixture
def finder():
    return Finder(version=MC_1_21_WD)

def test_set_attempt_seed(finder):
    seed = 1234567890
    cx, cz = 123, 456
    updated_seed = finder.set_attempt_seed(seed, cx, cz)
    assert isinstance(updated_seed, int)
    assert updated_seed == 206826366665763

def test_get_population_seed(finder):
    world_seed = 1234567890
    x, z = 123, 456
    population_seed = finder.get_population_seed(world_seed, x, z)
    assert isinstance(population_seed, int)
    assert population_seed == 9200318741546110857

def test_get_structure_config(finder):
    result = finder.get_structure_config(Village)
    assert isinstance(result, dict)
    assert 'salt' in result
    assert result['salt'] == 10387312
    assert 'regionSize' in result
    assert result['regionSize'] == 34
    assert 'chunkRange' in result
    assert result['chunkRange'] == 26
    assert 'structType' in result
    assert result['structType'] == Village
    assert 'dim' in result
    assert result['dim'] == 0
    assert 'rarity' in result
    assert result['rarity'] == 0
    

def test_is_stronghold_biome(finder):
    result = finder.is_stronghold_biome(plains)
    assert isinstance(result, bool)

def test_init_first_stronghold(finder):
    seed = 1234567890
    nextapprox, sh = finder.init_first_stronghold(seed)
    assert isinstance(nextapprox, Pos)
    assert nextapprox.x == -520 and nextapprox.z == -2600
    assert isinstance(sh, dict)
    assert sh['pos'].x == 0 and sh['pos'].z == 0
    assert sh['nextapprox'].x == -520 and sh['nextapprox'].z == -2600
    assert sh['index'] == 0
    assert sh['ringnum'] == 0
    assert sh['ringmax'] == 3
    assert sh['ringidx'] == 0
    assert sh['angle'] == 4.5127238872158175
    assert sh['dist'] == 166.02303278628128
    assert sh['rnds'] == 197462054985395

def test_get_spawn(finder):
    seed = 1234567890
    generator = Generator(MC_1_21_WD, seed)
    spawn_pos = finder.get_spawn(generator)
    
    assert isinstance(spawn_pos, Pos)
    assert spawn_pos.x == 8
    assert spawn_pos.z == 8

def test_next_stronghold(finder):
    sh = {
        'pos': Pos(x=0, z=0),
        'nextapprox': Pos(x=-520, z=-2600),
        'index': 0,
        'ringnum': 0,
        'ringmax': 3,
        'ringidx': 0,
        'angle': 4.5127238872158175,
        'dist': 166.02303278628128,
        'rnds': 197462054985395
    }
    generator = Generator(MC_1_21_WD, 0)
    is_valid, sh = finder.next_stronghold(sh, generator)
    assert isinstance(is_valid, bool)
    assert is_valid
    assert isinstance(sh, dict)
    assert sh['pos'].x == -524 and sh['pos'].z == -2604
    assert sh['nextapprox'].x == 1528 and sh['nextapprox'].z == 520
    assert sh['index'] == 1
    assert sh['ringnum'] == 0
    assert sh['ringmax'] == 3
    assert sh['ringidx'] == 1
    assert sh['angle'] == 6.607118989609013
    assert sh['dist'] == 99.97235323528389
    assert sh['rnds'] == 228792104649703

def test_chunk_generate_rnd(finder):
    seed = 1234567890
    chunkX = 0
    chunkZ = 0
    rnd = finder.chunk_generate_rnd(seed, chunkX, chunkZ)
    assert isinstance(rnd, int)
    assert rnd == 24016250047

def test_get_structure_pos(finder):
    structure = Village
    seed = 1234567890
    reg_x = 0
    reg_z = 0
    pos = finder.get_structure_pos(structure, seed, reg_x, reg_z)
    assert pos is None or isinstance(pos, Pos)
    assert pos.x == 256 and pos.z == 64

def test_get_variant(finder):
    struct_type = Village
    seed = 1234567890
    block_x, block_z = 288, 1984
    biome_id = plains
    
    variant = finder.get_variant(struct_type, seed, block_x, block_z, biome_id)
    
    assert isinstance(variant, dict)
    assert 'abandoned' in variant
    assert 'start' in variant
    assert 'rotation' in variant

    assert variant['abandoned'] == False
    assert variant['start'] == 0
    assert variant['rotation'] == 1
    
    invalid_biome_id = 999
    variant_none = finder.get_variant(struct_type, seed, block_x, block_z, invalid_biome_id)
    assert variant_none is None
