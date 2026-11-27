#pragma once

#include <json/json.h>

#include <filesystem>
#include <initializer_list>
#include <string_view>

namespace manifest_input {

Json::Value read_json(const std::filesystem::path &path);
std::filesystem::path resolve_path(const std::filesystem::path &directory,
                                   const Json::Value &value, const char *field);
bool is_unsigned_integer(const Json::Value &value);
void check_fields(const Json::Value &object, std::initializer_list<std::string_view> fields,
                  const std::string &location);
std::string read_string(const Json::Value &value, const std::string &location);
double read_number(const Json::Value &value, const std::string &location);

}
