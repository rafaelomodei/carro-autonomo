#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace traffic_sign_service::vision {

bool isValidBase64(std::string_view value);
std::vector<unsigned char> decodeBase64(std::string_view value, std::string *error = nullptr);
std::string encodeBase64(const std::vector<unsigned char> &input);

} // namespace traffic_sign_service::vision
