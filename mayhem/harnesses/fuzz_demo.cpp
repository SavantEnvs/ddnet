// CDemoPlayer::Load + full playback of an untrusted .demo file, as src/tools/demo_extract_chat.cpp
// does: header/version checks, timeline markers, embedded map, chunk parsing, huffman + varint
// decompression and snapshot delta application; the listener decodes every net message with
// UnpackMessageId + SecureUnpackMsg like the tool.
#include <cstddef>
#include <cstdint>
#include "fuzz_common.h"

#include <engine/shared/demo.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/protocol_ex.h>
#include <engine/shared/snapshot.h>
#include <engine/storage.h>
#include <generated/protocol.h>
#include <generated/protocol7.h>

static IStorage *g_pStorage;
static CSnapshotDelta *g_pDelta, *g_pDeltaSixup;

class CListener : public CDemoPlayer::IListener
{
public:
	void OnDemoPlayerSnapshot(void *pData, int Size) override
	{
		const CSnapshot *pSnap = (const CSnapshot *)pData;
		if(pSnap->IsValid(Size))
			for(int i = 0; i < pSnap->NumItems(); i++)
				(void)pSnap->GetItemSize(i);
	}
	void OnDemoPlayerMessage(void *pData, int Size) override
	{
		CUnpacker Unpacker;
		Unpacker.Reset(pData, Size);
		CMsgPacker Packer(NETMSG_EX, true);
		int Msg;
		bool Sys;
		if(UnpackMessageId(&Msg, &Sys, &Unpacker, &Packer) == UNPACKMESSAGE_ERROR || Sys)
			return;
		CNetObjHandler Handler;
		(void)Handler.SecureUnpackMsg(Msg, &Unpacker);
	}
};

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	CNetBase::Init();
	g_pStorage = CreateLocalStorage().release();
	g_pDelta = new CSnapshotDelta();
	CNetObjHandler Handler;
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		g_pDelta->SetStaticsize(i, Handler.GetObjSize(i));
	g_pDeltaSixup = new CSnapshotDelta();
	protocol7::CNetObjHandler Handler7;
	static const int OLD_NUM_NETOBJTYPES = 23; // as demo_extract_chat's CreateSnapshotDeltaSixup
	for(int i = 0; i < OLD_NUM_NETOBJTYPES; i++)
		g_pDeltaSixup->SetStaticsize(i, Handler7.GetObjSize(i));
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	std::string Path = FuzzWriteTemp(pData, Size, ".demo");
	{
		CDemoPlayer Player(g_pDelta, g_pDeltaSixup, false);
		if(Player.Load(g_pStorage, Path.c_str(), IStorage::TYPE_ABSOLUTE) != -1)
		{
			CListener Listener;
			Player.SetListener(&Listener);
			Player.Play();
			while(Player.IsPlaying())
			{
				Player.Update(false);
				if(Player.Info()->m_Info.m_Paused)
					break;
			}
			Player.Stop();
		}
	}
	unlink(Path.c_str());
	return 0;
}
