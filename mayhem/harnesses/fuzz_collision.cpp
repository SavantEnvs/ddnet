// The server/client map load path: IMap::Load, then CLayers::Init (groups, layers, tilemaps,
// tele/speedup/switch/tune layers, tilemap skip) and CCollision::Init plus a few collision queries.
#include <cstdint>
#include <cstddef>
#include "fuzz_common.h"

#include <engine/map.h>
#include <engine/storage.h>
#include <game/collision.h>
#include <game/layers.h>

static IStorage *g_pStorage;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	if(!g_pStorage)
		g_pStorage = CreateLocalStorage().release();
	std::string Path = FuzzWriteTemp(pData, Size, ".map");
	std::unique_ptr<IMap> pMap = CreateMap();
	if(pMap->Load(g_pStorage, Path.c_str(), IStorage::TYPE_ABSOLUTE))
	{
		CLayers Layers;
		Layers.Init(pMap.get(), false, true);
		if(Layers.GameLayer())
		{
			CCollision Collision;
			Collision.Init(&Layers);
			vec2 Out, Before;
			Collision.IntersectLine(vec2(0, 0), vec2(Collision.GetWidth() * 32.0f, Collision.GetHeight() * 32.0f), &Out, &Before);
			for(int i = 0; i < 16; i++)
				(void)Collision.GetTileIndex(Collision.GetPureMapIndex(vec2(i * 37.0f, i * 53.0f)));
			Collision.Unload();
		}
		pMap->Unload();
	}
	unlink(Path.c_str());
	return 0;
}
