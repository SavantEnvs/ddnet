// Master-server / HTTP server-list entry parsing: json_parse, then CServerInfo2::FromJson,
// Validate and the conversion to CServerInfo the server browser performs.
#include <cstdint>
#include <cstddef>
#include <engine/external/json-parser/json.h>
#include <engine/serverbrowser.h>
#include <engine/shared/serverinfo.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *pData, size_t Size)
{
	json_value *pJson = json_parse((const json_char *)pData, Size);
	if(!pJson)
		return 0;
	CServerInfo2 Info;
	if(!CServerInfo2::FromJson(&Info, pJson)) // false = parsed and validated
	{
		CServerInfo Converted = Info;
		(void)Converted;
	}
	json_value_free(pJson);
	return 0;
}
