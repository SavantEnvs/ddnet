// IMap::Load (CDataFileReader) over an untrusted .map file, then walk every item and data block
// exactly like src/tools/map_test.cpp. Reaches datafile.cpp (header/offset/zlib/UUID item types)
// and map.cpp (version item validation).
#include <cstdint>
#include <cstddef>
#include "fuzz_common.h"

#include <base/hash.h>
#include <engine/map.h>
#include <engine/shared/uuid_manager.h>
#include <engine/storage.h>

static IStorage *g_pStorage;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	if(!g_pStorage)
		g_pStorage = CreateLocalStorage().release();
	std::string Path = FuzzWriteTemp(pData, Size, ".map");
	std::unique_ptr<IMap> pMap = CreateMap();
	if(pMap->Load(g_pStorage, Path.c_str(), IStorage::TYPE_ABSOLUTE))
	{
		volatile unsigned Sink = pMap->Crc();
		for(int i = 0; i < pMap->NumItems(); i++)
		{
			int Type, Id;
			CUuid Uuid;
			const void *pItem = pMap->GetItem(i, &Type, &Id, &Uuid);
			const int ItemSize = pMap->GetItemSize(i);
			if(pItem && ItemSize > 0)
				Sink += sha256(pItem, ItemSize).data[0];
		}
		for(int i = 0; i < pMap->NumData(); i++)
		{
			const void *pBlock = pMap->GetData(i);
			const int DataSize = pMap->GetDataSize(i);
			if(pBlock && DataSize > 0)
				Sink += sha256(pBlock, DataSize).data[0];
			pMap->UnloadData(i);
		}
		(void)Sink;
		pMap->Unload();
	}
	unlink(Path.c_str());
	return 0;
}
