// CImageLoader::LoadPng over an in-memory PNG (maps embed images; skins/assets are shipped as
// PNG), then the processing the dilate tool and the client apply: ConvertToRgba, DilateImage and
// a SavePng round trip.
#include <cstdint>
#include <cstddef>
#include <engine/gfx/image_loader.h>
#include <engine/gfx/image_manipulation.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	CByteBufferReader Reader(pData, Size);
	CImageInfo Image;
	int PngliteIncompatible = 0;
	if(!CImageLoader::LoadPng(Reader, "fuzz", Image, PngliteIncompatible))
		return 0;
	if(ConvertToRgba(Image))
	{
		DilateImage(Image);
		CByteBufferWriter Writer;
		CImageLoader::SavePng(Writer, Image);
	}
	Image.Free();
	return 0;
}
