#ifndef CONTENT_TYPE_H
#define CONTENT_TYPE_H
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/file_status.hpp>
#include <string>
#include <string_view>

std::string format_size(std::size_t size);
std::string_view parse_type(const boost::filesystem::path& path, bool is_dir = false);
std::string_view parse_icon(const boost::filesystem::path& path, bool is_dir = false);

#endif // CONTENT_TYPE_H