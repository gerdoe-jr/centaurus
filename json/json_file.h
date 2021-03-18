#ifndef __JSON_FILE_H
#define __JSON_FILE_H

#include "json.hpp"
#include <string>

using json = nlohmann::json;

json ReadJSONFile(const std::wstring& path);
void WriteJSONFile(const std::wstring& path, const json& value);

#endif
