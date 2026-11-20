// CSaveTeam::FromString — the /save code string that the server stores in the score database and
// parses back on /load (team stats, switcher states, then one CSaveTee::FromString line per tee).
// A parsed team is serialised again with GetString, as the server does when re-saving.
#include <cstddef>
#include <cstdint>

#include <game/server/save.h>

#include <string>

// Normally defined by the server's main.cpp (signal-interrupt flag polled by CServer::Run, which
// this harness never calls); upstream's gameworld_test.cpp provides the same stub.
bool IsInterrupted()
{
	return false;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	std::string Str(reinterpret_cast<const char *>(pData), Size); // NUL-terminated, like a DB text column
	CSaveTeam Team;
	if(Team.FromString(Str.c_str()) == 0)
	{
		const char *pOut = Team.GetString();
		if(pOut)
		{
			CSaveTeam Again;
			(void)Again.FromString(pOut);
		}
	}
	return 0;
}
