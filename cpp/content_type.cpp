#include "content_type.h"
#include <boost/beast/core/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/file_status.hpp>
#include <format>

std::string format_size(std::size_t size) {
	if (size > 1.1*1024*1024*1024)
		return std::format("{:.1f}G", double(size)/1024/1024/1024);
	else if (size > 1.1*1024*1024)
		return std::format("{:.1f}M", double(size)/1024/1024);
	else
		return std::format("{:.1f}K", double(size)/1024);
}

std::string_view parse_type(const boost::filesystem::path& path, bool is_dir) {
	if (auto ext = path.extension().string(); boost::beast::iequals(ext, ".mp4"))
        return "video/mp4";
    else if (boost::beast::iequals(ext, ".mkv"))
        return "video/mkv";
    else if (boost::beast::iequals(ext, ".webm")) 
        return "video/webm";
	else if (boost::beast::iequals(ext, ".css"))
		return "text/stylesheet";
	else if (boost::beast::iequals(ext, ".js"))
		return "text/javascript";
	else if (boost::beast::iequals(ext, ".json"))
		return "text/json";
	else if (boost::beast::iequals(ext, ".yaml"))
		return "text/yaml";
	else if (boost::beast::iequals(ext, ".toml"))
		return "text/toml";
	else if (boost::beast::iequals(ext, ".html"))
		return "text/html";
	else if (boost::beast::iequals(ext, ".php"))
		return "text/php";
	else if (boost::beast::iequals(ext, ".cpp"))
		return "text/cpp";
	else if (boost::beast::iequals(ext, ".go"))
		return "text/go";
	else if (boost::beast::iequals(ext, ".rs"))
		return "test/rs";
	else if (boost::beast::iequals(ext, ".flac"))
        return "audio/flac";
	else if (boost::beast::iequals(ext, ".wma"))
        return "audio/wma";
	else if (boost::beast::iequals(ext, ".mp3"))
        return "audio/mp3";
	else if (boost::beast::iequals(ext, ".wav"))
        return "audio/wav";
	else if (boost::beast::iequals(ext, ".ogg"))
        return "audio/ogg";
	else if (boost::beast::iequals(ext, ".gif"))
        return "image/gif";
	else if (boost::beast::iequals(ext, ".bmp"))
        return "image/bmp";
	else if (boost::beast::iequals(ext, ".ico"))
        return "image/ico";
	else if (boost::beast::iequals(ext, ".jpg"))
        return "image/jpeg";
	else if (boost::beast::iequals(ext, ".jpeg"))
        return "image/jpeg";
	else if (boost::beast::iequals(ext, ".heic"))
        return "image/heic";
	else if (boost::beast::iequals(ext, ".webp"))
        return "image/webp";
	else if (boost::beast::iequals(ext, ".txt"))
		return "text/plain";
	else if (boost::beast::iequals(ext, ".md"))
		return "text/markdown";
	else if (is_dir)
        return "";
    else
        return "application/octet-stream";
}

std::string_view parse_icon(const boost::filesystem::path& path, bool is_dir) {
	if (auto ext = path.extension().string(); boost::beast::iequals(ext, ".mp4"))
		return R"HTML(<i class="bi bi-film"></i>)HTML";
	else if (boost::beast::iequals(ext, ".mkv"))
        return R"HTML(<i class="bi bi-film"></i>)HTML";
	else if (boost::beast::iequals(ext, ".webm"))
        return R"HTML(<i class="bi bi-film"></i>)HTML";
	else if (boost::beast::iequals(ext, ".css"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".js"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".json"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".yaml"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".toml"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".html"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".php"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".cpp"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".go"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".rs"))
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (boost::beast::iequals(ext, ".flac"))
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (boost::beast::iequals(ext, ".wma"))
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (boost::beast::iequals(ext, ".mp3"))
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (boost::beast::iequals(ext, ".wav"))
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (boost::beast::iequals(ext, ".ogg"))
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (boost::beast::iequals(ext, ".gif"))
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (boost::beast::iequals(ext, ".bmp"))
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (boost::beast::iequals(ext, ".jpg"))
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (boost::beast::iequals(ext, ".jpeg"))
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (boost::beast::iequals(ext, ".heic"))
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (boost::beast::iequals(ext, ".webp"))
		return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (boost::beast::iequals(ext, ".txt"))
        return R"HTML(<i class="bi bi-file-text"></i>)HTML";
	else if (boost::beast::iequals(ext, ".md"))
		return R"HTML(<i class="bi bi-file-text"></i>)HTML";
	else if (is_dir)
        return R"HTML(<i class="bi bi-folder"></i>)HTML";
    else
		return "";
}
