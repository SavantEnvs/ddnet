// Game/system net message decoding as the server and client do it: UnpackMessageId (incl. UUID
// "ex" messages) followed by CNetObjHandler::SecureUnpackMsg for the 0.6 protocol and
// protocol7::CNetObjHandler::SecureUnpackMsg for 0.7, plus 0.6 SecureUnpackObj for snapshot objects.
// The first input byte selects which decoder runs.
#include <cstdint>
#include <cstddef>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/protocol_ex.h>
#include <generated/protocol.h>
#include <generated/protocol7.h>

#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	if(Size < 1)
		return -1;
	const uint8_t Mode = pData[0] % 3;
	// CUnpacker sanitizes strings in place (as on the server's receive buffer), so work on a copy.
	std::vector<uint8_t> Buf(pData + 1, pData + Size);
	CUnpacker Unpacker;
	Unpacker.Reset(Buf.data(), (int)Buf.size());
	if(Mode == 0)
	{
		int Id;
		bool Sys;
		CMsgPacker Packer(NETMSG_EX, true);
		if(UnpackMessageId(&Id, &Sys, &Unpacker, &Packer) != UNPACKMESSAGE_OK)
			return 0;
		if(!Sys)
		{
			CNetObjHandler Handler;
			if(Handler.SecureUnpackMsg(Id, &Unpacker))
				(void)Handler.GetMsgName(Id);
		}
	}
	else if(Mode == 1)
	{
		const int Id = Unpacker.GetInt();
		if(Unpacker.Error())
			return 0;
		protocol7::CNetObjHandler Handler;
		if(Handler.SecureUnpackMsg(Id >> 1, &Unpacker))
			(void)Handler.GetMsgName(Id >> 1);
	}
	else if(Mode == 2)
	{
		const int Type = Unpacker.GetInt();
		if(Unpacker.Error())
			return 0;
		CNetObjHandler Handler;
		(void)Handler.SecureUnpackObj(Type, &Unpacker);
	}
	return 0;
}
