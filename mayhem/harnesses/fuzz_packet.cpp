// CNetBase::UnpackPacket on a raw UDP datagram (0.6 and 0.7/sixup headers, connless packets,
// security tokens, huffman decompression), then split the payload into chunks with
// CNetChunkHeader::Unpack bounded the same way CNetRecvUnpacker::FetchChunk bounds it.
#include <cstdint>
#include <cstddef>
#include <engine/shared/network.h>

#include <cstring>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	CNetBase::Init();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	if(Size > NET_MAX_PACKETSIZE) // a UDP read never yields more (CNetBase::Recv buffer size)
		return -1;
	unsigned char aBuf[NET_MAX_PACKETSIZE];
	memcpy(aBuf, pData, Size);
	static CNetPacketConstruct Packet;
	bool Sixup = false;
	SECURITY_TOKEN Token, ResponseToken;
	if(CNetBase::UnpackPacket(aBuf, (int)Size, &Packet, Sixup, true, &Token, &ResponseToken) != 0)
		return 0;
	if(Packet.m_Flags & NET_PACKETFLAG_CONNLESS)
		return 0;
	unsigned char *pEnd = Packet.m_aChunkData + Packet.m_DataSize;
	unsigned char *pCur = Packet.m_aChunkData;
	const int HeaderSplit = Sixup ? 6 : 4;
	for(int i = 0; i < Packet.m_NumChunks; i++)
	{
		// the chunk header is two bytes, three for vital chunks (FetchChunk's bound)
		if(pCur + 2 > pEnd)
			break;
		CNetChunkHeader Header;
		unsigned char *pChunk = Header.Unpack(pCur, HeaderSplit);
		if(pChunk + Header.m_Size > pEnd)
			break;
		pCur = pChunk + Header.m_Size;
	}
	return 0;
}
