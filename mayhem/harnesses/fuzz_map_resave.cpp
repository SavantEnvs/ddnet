// The map_resave path (also behind map_optimize/map_convert_07): load an untrusted map with
// IMap::Load, copy every item and data block into a CDataFileWriter exactly as
// src/tools/map_resave.cpp does (ext item type mapping, recompression), Finish(), then load the
// written file back with CDataFileReader and walk it.
#include <cstddef>
#include <cstdint>
#include "fuzz_common.h"

#include <engine/map.h>
#include <engine/shared/datafile.h>
#include <engine/storage.h>

static IStorage *g_pStorage;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	if(!g_pStorage)
		g_pStorage = CreateLocalStorage().release();
	std::string Path = FuzzWriteTemp(pData, Size, ".map");
	std::string OutPath = Path + ".out.map";
	std::unique_ptr<IMap> pMap = CreateMap();
	if(pMap->Load(g_pStorage, Path.c_str(), IStorage::TYPE_ABSOLUTE))
	{
		CDataFileWriter Writer;
		if(Writer.Open(g_pStorage, OutPath.c_str(), IStorage::TYPE_ABSOLUTE))
		{
			for(int i = 0; i < pMap->NumItems(); i++)
			{
				int Type, Id;
				CUuid Uuid;
				const void *pItem = pMap->GetItem(i, &Type, &Id, &Uuid);
				if(Type == ITEMTYPE_EX) // re-added automatically, as in map_resave
					continue;
				Writer.AddItem(Type, Id, pMap->GetItemSize(i), pItem, &Uuid);
			}
			for(int i = 0; i < pMap->NumData(); i++)
			{
				// CDataFileWriter::AddData's contract (dbg_assert, datafile.cpp) requires a non-null,
				// non-empty block; GetData returns nullptr for an erroneous block (map_test reports
				// it as "Data erroneous"), so such blocks are not passed on.
				const void *pBlock = pMap->GetData(i);
				const int BlockSize = pMap->GetDataSize(i);
				if(pBlock && BlockSize > 0)
					Writer.AddData(BlockSize, pBlock);
			}
			pMap->Unload();
			Writer.Finish();

			CDataFileReader Reader;
			if(Reader.Open(g_pStorage, OutPath.c_str(), IStorage::TYPE_ABSOLUTE))
			{
				for(int i = 0; i < Reader.NumItems(); i++)
				{
					int Type, Id;
					CUuid Uuid;
					(void)Reader.GetItem(i, &Type, &Id, &Uuid);
				}
				for(int i = 0; i < Reader.NumData(); i++)
				{
					(void)Reader.GetData(i);
					Reader.UnloadData(i);
				}
				Reader.Close();
			}
		}
		else
			pMap->Unload();
	}
	unlink(OutPath.c_str());
	unlink(Path.c_str());
	return 0;
}
