#include "json_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <memory>

#ifndef WIN32
#define _fseeki64 fseeko64
#define _ftelli64 ftello64

#include "win32util.h"

static FILE* _wfopen(const wchar_t* path, const wchar_t* mode)
{
    return fopen(WcharToText(path).c_str(), WcharToText(mode).c_str());
}

#endif

json ReadJSONFile(const std::wstring& path)
{
    FILE* fJson = _wfopen(path.c_str(), L"r");
    if (!fJson)
        throw std::runtime_error("!ReadJSONFile");

    fseek(fJson, 0L, SEEK_END);
    long iSize = ftell(fJson);
    fseek(fJson, 0L, SEEK_SET);

    auto pszJson = std::make_unique<char[]>(iSize+1);
    fread(pszJson.get(), 1, iSize, fJson);
    fclose(fJson);

    return json::parse(pszJson.get());
}

void WriteJSONFile(const std::wstring& path, const json& value)
{
    FILE* fJson = _wfopen(path.c_str(), L"w");
    if (!fJson)
        throw std::runtime_error("!WriteJSONFile");

    std::string dump = value.dump(4);
    fwrite(dump.c_str(), 1, dump.size(), fJson);
    fclose(fJson);
}
