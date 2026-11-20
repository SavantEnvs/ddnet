// CConsole::ExecuteLine — the parser behind rcon, votes, cfg files and chat commands: line and
// command-chain splitting, quoting/escaping, and argument parsing for every parameter format
// (i, f, s, r, c, v, ? optional). Only the commands registered below are executable: they carry
// CFGFLAG_GAME and lines run with ExecuteLineFlag(CFGFLAG_GAME), the console's own flag filter, so
// built-ins such as `exec` (reads files) and `access_level` (mutates state) are never run.
// A fresh console is built for every input, so no state survives between inputs.
#include <cstdint>
#include <cstddef>
#include <base/color.h>
#include <engine/console.h>
#include <engine/shared/config.h>

#include <string>
#include <vector>

// Read every argument through every accessor. Unsigned accumulation (defined wrap-around) and
// floats stored as floats: the sink itself must never be UB, whatever values the input parses to.
static void Consume(IConsole::IResult *pResult, void *)
{
	volatile unsigned Sink = 0;
	volatile float FloatSink = 0.0f;
	for(int i = 0; i < pResult->NumArguments(); i++)
	{
		Sink += (unsigned)pResult->GetInteger(i);
		FloatSink = pResult->GetFloat(i);
		Sink += (unsigned char)pResult->GetString(i)[0];
		FloatSink = pResult->GetColor(i, 0.5f).h;
	}
	(void)Sink;
	(void)FloatSink;
}

static void ConsumeVictim(IConsole::IResult *pResult, void *)
{
	volatile int Sink = pResult->NumArguments() ? pResult->GetVictim(0) : 0;
	(void)Sink;
}

static std::optional<std::vector<int>> Victims(int, const char *pVictim, void *)
{
	return std::vector<int>{pVictim[0] & 63};
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	std::string Line(reinterpret_cast<const char *>(pData), Size); // NUL-terminated copy
	std::unique_ptr<IConsole> pConsole = CreateConsole(CFGFLAG_SERVER | CFGFLAG_GAME);
	pConsole->SetGetVictimsCommandCallback(Victims, nullptr);
	pConsole->Register("t_int", "i[a] ?i[b]", CFGFLAG_GAME, Consume, nullptr, "");
	pConsole->Register("t_float", "f[a] ?f[b] ?f[c]", CFGFLAG_GAME, Consume, nullptr, "");
	pConsole->Register("t_str", "s[a] ?s[b] ?r[rest]", CFGFLAG_GAME, Consume, nullptr, "");
	pConsole->Register("t_rest", "r[text]", CFGFLAG_GAME, Consume, nullptr, "");
	pConsole->Register("t_mix", "i[a] f[b] s[c] ?i[d] ?r[e]", CFGFLAG_GAME, Consume, nullptr, "");
	pConsole->Register("t_victim", "v[id] ?r[reason]", CFGFLAG_GAME, ConsumeVictim, nullptr, "");
	pConsole->Register("t_any", "?s[a] ?s[b] ?s[c] ?s[d] ?s[e] ?s[f]", CFGFLAG_GAME, Consume, nullptr, "");
	pConsole->ExecuteLineFlag(Line.c_str(), CFGFLAG_GAME, IConsole::CLIENT_ID_UNSPECIFIED, true);
	return 0;
}
