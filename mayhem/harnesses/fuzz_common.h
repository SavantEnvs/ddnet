// Shared helpers for the ddnet fuzz harnesses (mayhem integration).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

// Write the input to a per-process scratch file under $TMPDIR (default /tmp) and return its path.
// File-based loaders (IMap::Load, CDemoPlayer::Load) take a path + IStorage, not a buffer.
static inline std::string FuzzWriteTemp(const uint8_t *pData, size_t Size, const char *pSuffix)
{
	const char *pTmp = getenv("TMPDIR");
	if(!pTmp || !*pTmp)
		pTmp = "/tmp";
	std::string Path = std::string(pTmp) + "/ddnet_fuzz_" + std::to_string(getpid()) + pSuffix;
	FILE *f = fopen(Path.c_str(), "wb");
	if(!f)
		abort();
	if(Size && fwrite(pData, 1, Size, f) != Size)
		abort();
	fclose(f);
	return Path;
}
