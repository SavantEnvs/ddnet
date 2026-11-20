// CHuffman::Decompress (the network/demo huffman decoder), with a compress/decompress round trip
// of the decoded bytes checked for equality.
#include <cstdint>
#include <cstddef>
#include <engine/shared/huffman.h>

#include <cstdlib>
#include <cstring>

static CHuffman g_Huffman;

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	g_Huffman.Init();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	static unsigned char aOut[4096], aRe[8192], aBack[4096];
	const int n = g_Huffman.Decompress(pData, (int)Size, aOut, sizeof(aOut));
	if(n < 0)
		return 0;
	const int c = g_Huffman.Compress(aOut, n, aRe, sizeof(aRe));
	if(c < 0)
		return 0;
	const int m = g_Huffman.Decompress(aRe, c, aBack, sizeof(aBack));
	if(m != n || memcmp(aOut, aBack, n) != 0)
		abort(); // round trip must be lossless
	return 0;
}
