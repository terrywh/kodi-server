#include "content_type.h"
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

std::string_view parse_type(const boost::filesystem::path& path, const boost::filesystem::file_status& stat) {
	if (auto ext = path.extension(); ext == ".mp4")
        return "video/mp4";
    else if (ext == ".mkv")
        return "video/mkv";
    else if (ext == ".webm") 
        return "video/webm";
	else if (ext == ".css")
		return "text/stylesheet";
	else if (ext == ".js")
		return "text/javascript";
	else if (ext == ".json")
		return "text/json";
	else if (ext == ".yaml")
		return "text/yaml";
	else if (ext == ".toml")
		return "text/toml";
	else if (ext == ".html")
		return "text/html";
	else if (ext == ".php")
		return "text/php";
	else if (ext == ".cpp")
		return "text/cpp";
	else if (ext == ".go")
		return "text/go";
	else if (ext == ".rs")
		return "test/rs";
	else if (ext == ".flac")
        return "audio/flac";
	else if (ext == ".wma")
        return "audio/wma";
	else if (ext == ".mp3")
        return "audio/mp3";
	else if (ext == ".wav")
        return "audio/wav";
	else if (ext == ".ogg")
        return "audio/ogg";
	else if (ext == ".gif")
        return "image/gif";
	else if (ext == ".bmp")
        return "image/bmp";
	else if (ext == ".ico")
        return "image/ico";
	else if (ext == ".jpg")
        return "image/jpeg";
	else if (ext == ".jpeg")
        return "image/jpeg";
	else if (ext == ".heic")
        return "image/heic";
	else if (ext == ".webp")
        return "image/webp";
	else if (ext == ".txt")
		return "text/plain";
	else if (ext == ".md")
		return "text/markdown";
	else if (stat.type() == boost::filesystem::file_type::directory_file)
        return "";
    else
        return "application/octet-stream";
}

std::string_view parse_icon(const boost::filesystem::path& path, const boost::filesystem::file_status& stat) {
	if (auto ext = path.extension(); ext == ".mp4")
		return R"HTML(<i class="bi bi-film"></i>)HTML";
	else if (ext == ".mkv")
        return R"HTML(<i class="bi bi-film"></i>)HTML";
	else if (ext == ".webm")
        return R"HTML(<i class="bi bi-film"></i>)HTML";
	else if (ext == ".css")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".js")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".json")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".yaml")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".toml")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".html")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".php")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".cpp")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".go")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".rs")
        return R"HTML(<i class="bi bi-file-code"></i>)HTML";
	else if (ext == ".flac")
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (ext == ".wma")
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (ext == ".mp3")
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (ext == ".wav")
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (ext == ".ogg")
        return R"HTML(<i class="bi bi-file-music"></i>)HTML";
	else if (ext == ".gif")
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (ext == ".bmp")
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (ext == ".jpg")
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (ext == ".jpeg")
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (ext == ".heic")
        return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (ext == ".webp")
		return R"HTML(<i class="bi bi-file-image"></i>)HTML";
	else if (ext == ".txt")
        return R"HTML(<i class="bi bi-file-text"></i>)HTML";
	else if (ext == ".md")
		return R"HTML(<i class="bi bi-file-text"></i>)HTML";
	else if (stat.type() == boost::filesystem::file_type::directory_file)
        return R"HTML(<i class="bi bi-folder"></i>)HTML";
    else
		return "";
}
