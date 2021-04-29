#include "json_file.h"
#include <win32util.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <memory>

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
