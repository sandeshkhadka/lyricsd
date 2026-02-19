#include "simplejson.hpp"
#include <string>
std::string ExtractJsonString(const std::string& json,
                                     const std::string& key) {
  std::string search_key = "\"" + key + "\"";
  size_t key_post = json.find(search_key);
  if (key_post == std::string::npos)
    return "";

  size_t colon_pos = json.find(':', key_post);
  if (colon_pos == std::string::npos)
    return "";

  size_t value_start = json.find_first_not_of(" \t\n\r", colon_pos + 1);
  if (value_start == std::string::npos)
    return "";

  if (json[value_start] == 'n') {
    return "";
  }

  if (json[value_start] != '"')
    return "";
  value_start++;

  std::string result;
  for (size_t i = value_start; i < json.length(); i++) {
    if (json[i] == '\\' && i + 1 < json.length()) {
      char next = json[i + 1];
      if (next == 'n') {
        result += '\n';
        i++;
      } else if (next == 't') {
        result += '\t';
        i++;
      } else if (next == '"') {
        result += '"';
        i++;
      } else if (next == '\\') {
        result += '\\';
        i++;
      } else {
        result += json[i];
      }
    } else if (json[i] == '"') {
      break;
    } else {
      result += json[i];
    }
  }

  return result;
}
