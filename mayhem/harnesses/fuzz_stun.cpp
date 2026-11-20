// StunMessageParse over an untrusted STUN reply (header, attribute walk, (XOR-)MAPPED-ADDRESS v4/v6).
// The request secret comes from StunMessagePrepare like the client's real request; the first 12
// input bytes, when present, override it so replies can match the transaction id.
#include <cstdint>
#include <cstddef>
#include <base/types.h>
#include <engine/shared/stun.h>

#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	CStunData StunData;
	unsigned char aReq[64];
	StunMessagePrepare(aReq, sizeof(aReq), &StunData);
	if(Size < sizeof(StunData.m_aSecret))
		return -1;
	memcpy(StunData.m_aSecret, pData, sizeof(StunData.m_aSecret));
	pData += sizeof(StunData.m_aSecret);
	Size -= sizeof(StunData.m_aSecret);
	bool Success = false;
	NETADDR Addr = {};
	StunMessageParse(pData, Size, &StunData, &Success, &Addr);
	return 0;
}
