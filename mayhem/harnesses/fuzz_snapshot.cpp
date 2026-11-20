// Snapshot delta decoding as the client does on NETMSG_SNAP*: CVariableInt::Decompress of the
// packet payload, CSnapshotDelta::UnpackDelta against the empty snapshot, then IsValid/Crc and item
// lookups on the result; the remaining input is applied as a second delta on top of the first.
#include <cstdint>
#include <cstddef>
#include <engine/shared/compression.h>
#include <engine/shared/snapshot.h>
#include <generated/protocol.h>

#include <memory>

static CSnapshotDelta *g_pDelta;

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	g_pDelta = new CSnapshotDelta();
	CNetObjHandler Handler;
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		g_pDelta->SetStaticsize(i, Handler.GetObjSize(i));
	return 0;
}

static bool Apply(const CSnapshot *pFrom, CSnapshotBuffer *pTo, const uint8_t *pData, size_t Size)
{
	static int aInts[CSnapshot::MAX_SIZE / sizeof(int)];
	const long IntSize = CVariableInt::Decompress(pData, (int)Size, aInts, sizeof(aInts));
	if(IntSize < 0)
		return false;
	const int SnapSize = g_pDelta->UnpackDelta(pFrom, pTo, aInts, (int)IntSize);
	if(SnapSize < 0)
		return false;
	const CSnapshot *pSnap = pTo->AsSnapshot();
	if(!pSnap->IsValid(SnapSize))
		return false;
	volatile unsigned Crc = pSnap->Crc();
	(void)Crc;
	for(int i = 0; i < pSnap->NumItems(); i++)
	{
		(void)pSnap->GetItemSize(i);
		(void)pSnap->GetItemType(i);
		(void)pSnap->GetItemIndex(pSnap->GetItem(i)->Key());
	}
	return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	if(Size < 2)
		return -1;
	// First byte: split point between the two deltas (scaled to the input length).
	const size_t Split = 1 + (size_t)pData[0] * (Size - 1) / 255;
	auto pFirst = std::make_unique<CSnapshotBuffer>();
	if(!Apply(CSnapshot::EmptySnapshot(), pFirst.get(), pData + 1, Split - 1))
		return 0;
	auto pSecond = std::make_unique<CSnapshotBuffer>();
	Apply(pFirst->AsSnapshot(), pSecond.get(), pData + Split, Size - Split);
	return 0;
}
