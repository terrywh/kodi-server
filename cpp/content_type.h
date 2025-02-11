#ifndef CONTENT_TYPE_H
#define CONTENT_TYPE_H
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/file_status.hpp>
#include <string>
#include <string_view>

std::string format_size(std::size_t size);
std::string_view parse_type(const boost::filesystem::path& path, const boost::filesystem::file_status& stat);
std::string_view parse_icon(const boost::filesystem::path& path, const boost::filesystem::file_status& stat);

#endif // CONTENT_TYPE_H