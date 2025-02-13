#include <boost/asio.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>
#include <boost/url.hpp>
#include <boost/charconv/from_chars.hpp>

#include <format>
#include <string>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#ifdef __linux__
#include <sys/sendfile.h>
#endif // linux

#include "content_type.h"

template <class T>
class callback {
    std::shared_ptr<T> ref_;

public:
    explicit callback(std::enable_shared_from_this<T>* self)
    : ref_(self->shared_from_this()) {}

    void operator()(const boost::system::error_code& error = {}, std::size_t size = 0) {
        ref_->operator()(error, size);
    }
};

struct do_sendfile_impl: public boost::asio::coroutine {
    boost::asio::ip::tcp::socket& socket_;
    boost::asio::stream_file&       file_;
    const boost::filesystem::path&  path_;
    std::size_t                     size_;
    boost::asio::mutable_buffer   buffer_;
    ssize_t         start_, end_, remain_;
 
    explicit do_sendfile_impl(boost::asio::ip::tcp::socket& socket,
        boost::asio::mutable_buffer   buffer,
        boost::asio::stream_file& file,
        const boost::filesystem::path&  path,
        std::size_t                     size,
        std::size_t start, std::size_t   end)
    : socket_(socket)
    , file_(file)
    , path_(path)
    , size_(size)
    , buffer_(buffer)
    , start_(start)
    , end_(end)
    , remain_(end - start + 1) {}

    do_sendfile_impl(const do_sendfile_impl& self) = default;
    do_sendfile_impl(do_sendfile_impl&& self) = default;

    template <class Self>
    void operator()(Self& self, boost::system::error_code error = {}, std::size_t size = 0) { BOOST_ASIO_CORO_REENTER(this) {
        size = std::format_to(static_cast<char*>(buffer_.data()), // std::counted_iterator{static_cast<char*>(buffer_.data), buffer_.size()},
            "HTTP/1.1 206 Partial Content\r\n" 
            "Content-Type: {}\r\n"
            "Content-Length: {}\r\n"
            "Content-Range: bytes {}-{}/{}\r\n\r\n", parse_type(path_), end_ - start_ + 1, start_, end_, size_) - static_cast<char*>(buffer_.data());

        BOOST_ASIO_CORO_YIELD boost::asio::async_write(socket_, boost::asio::buffer(buffer_, size), std::move(self));
        if (error) {
            self.complete(error, size);
            return;
        }

#ifdef __linux__
        while (remain_ > 0 && !error) {
            // 注意：sendfile 调用后 start_ 会被修改
            size = ::sendfile(socket_.native_handle(), file_.native_handle(), &start_, end_ - start_ + 1);
            if (size < 0) {
                self.complete(boost::system::error_code{errno, boost::system::system_category()}, 0);
                return;
            }
            remain_ -= size;
            BOOST_ASIO_CORO_YIELD socket_.async_wait(boost::asio::socket_base::wait_write, std::move(self));
        }

#elif defined(BOOST_ASIO_WINDOWS)
        // 参考：https://www.boost.io/doc/libs/latest/doc/html/boost_asio/example/cpp11/windows/transmit_file.cpp
        
#else // NOT __linux__
        file_.seek(start_, boost::asio::file_base::seek_set);

        while (remain_ > 0) {    
            BOOST_ASIO_CORO_YIELD file_.async_read_some(buffer_, std::move(self));
            if (error) {
                self.complete(error, size);
                return;
            }
            remain_ -= size;
            BOOST_ASIO_CORO_YIELD boost::asio::async_write(socket_, boost::asio::buffer(buffer_, size), std::move(self));
            if (error) {
                self.complete(error, size);
            }
        };

#endif // NOT __linux__

        self.complete(error, size);
        // socket_.async_wait()
    }}
};

template <class MutableBuffer, class CompleteToken>
auto do_sendfile(boost::asio::ip::tcp::socket& socket, MutableBuffer buffer, boost::asio::stream_file& file,
        const boost::filesystem::path& path, std::size_t size, std::pair<std::size_t, std::size_t> range, CompleteToken&& token) 
    -> decltype(boost::asio::async_compose<CompleteToken, void (boost::system::error_code, std::size_t)>(
        std::declval<do_sendfile_impl>(), token, socket)) {
    
    return boost::asio::async_compose<CompleteToken, void (boost::system::error_code, std::size_t)>(
        do_sendfile_impl{socket, buffer, file, path, size, range.first, range.second}, token, socket);
}

class handler: public boost::asio::coroutine, public std::enable_shared_from_this<handler> {
    boost::asio::io_context& io_;
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::ip::tcp::endpoint address_;

    boost::beast::http::request<boost::beast::http::empty_body>    req_;
    boost::url url_;
    boost::beast::flat_buffer buf_;
    boost::beast::http::response<boost::beast::http::string_body> rsp_;
    boost::beast::http::response<boost::beast::http::file_body>   rsp_file_;

    boost::filesystem::path path_;
    boost::filesystem::file_status stat_;
    std::size_t size_;
    boost::asio::stream_file file_;
    char buffer_[256 * 1024];

public:
    explicit handler(boost::asio::io_context& io, boost::asio::ip::tcp::socket&& socket,
        boost::asio::ip::tcp::endpoint address)
    : io_(io)
    , socket_(std::move(socket))
    , file_(io) {}

    void operator()(boost::system::error_code error = {}, std::size_t = 0) { BOOST_ASIO_CORO_REENTER(this) {
        do {
            BOOST_ASIO_CORO_YIELD boost::beast::http::async_read(socket_, buf_, req_, callback(this));
            if (error) break;

            url_ = boost::url(req_.target());
            path_ = "." / boost::filesystem::path(url_.path()).lexically_normal();
            stat_ = boost::filesystem::status(path_, error);
            BOOST_LOG_TRIVIAL(debug) << req_.method_string() << " " << path_;
            if (error) break;
            size_ = boost::filesystem::file_size(path_, error);

            if (stat_.type() == boost::filesystem::directory_file) {
                rsp_.set("content-type", "text/html");
                rsp_.body() = build_directory(path_);
                rsp_.prepare_payload();
                BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(socket_, rsp_, callback(this));
            } else if (req_.count("range") > 0) {
                file_.close(error);
                file_.open(path_.string(), boost::asio::file_base::read_only, error);
                if (error) break;
                BOOST_ASIO_CORO_YIELD do_sendfile(socket_, boost::asio::buffer(buffer_), file_, path_, size_,
                    parse_range(req_.at("range")), callback(this));
            } else if (req_.method() == boost::beast::http::verb::head) {
                rsp_.body().clear();
                rsp_.prepare_payload();

                rsp_.set("content-type", parse_type(path_, false));
                rsp_.set("content-length", std::to_string(size_));
                BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(socket_, rsp_, callback(this));
            } else {
                rsp_file_.body().close();
                rsp_file_.body().open(path_.c_str(), boost::beast::file_mode::read, error);
                if (error) break;
                rsp_file_.prepare_payload();
                rsp_file_.set("content-type", parse_type(path_, false));
                BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(socket_, rsp_file_, callback(this));
            }
            
        } while(!error && req_.keep_alive() && !req_.need_eof());

        if (error) 
            if (error == boost::system::errc::no_such_file_or_directory ) {
                rsp_.set("content-type", "text/plain");
                rsp_.body() = "resource not found";
                rsp_.prepare_payload();
                // rsp_.set("content-type", "text/html");
                BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(socket_, rsp_, callback(this));
            } else if (error != boost::system::errc::connection_reset && error != boost::system::errc::broken_pipe) {
                BOOST_LOG_TRIVIAL(error) << "error = (" << error << ") " << error.message(); 
            }
    }}

    std::string build_directory(const boost::filesystem::path& path) {
        boost::system::error_code error;
        std::stringstream ss;
        ss << R"HTML(<html>
<head>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css" crossorigin="anonymous">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.4.1/dist/css/bootstrap.min.css" crossorigin="anonymous">
</head>
<body>
<div class="container">

<div class="row pt-3 pb-2"><div class="col-12">
    <h5>当前路径：<code>)HTML" << path.string() << R"HTML(</code></h5>
</div></div>

<div class="row"><div class="col-12">

<table class="table">
	<thead>
		<tr>
			<th>名称</th>
            <th style="width: 14em;">时间</th>
            <th style="width: 5em;">大小</th>
		</tr>
	</thead>
	<tbody>)HTML";
        
        if (path.has_parent_path())
		    ss << R"HTML(<tr><td><i class="bi bi-box-arrow-in-up"></i> <a href="../">上一层</a></td><td>&nbsp;</td><td>&nbsp;</td></tr>)HTML";

        for (auto i = boost::filesystem::directory_iterator(path); i!= boost::filesystem::directory_iterator(); i++) {
            if (i->path().filename().string()[0] == '.' || i->path().filename() == "node_modules")
                continue;

            std::size_t size = boost::filesystem::file_size(i->path(), error);
            ss << "\t\t<tr>";
                
                
            if (i->status().type() == boost::filesystem::file_type::directory_file) {
                ss 
                    << "\t\t\t<td>" << parse_icon(i->path(), true) << " <a href=\""
                    << i->path().filename().string() << "/\">" << i->path().filename().string() << "</a></td>"
                    << "\t\t\t<td> - </td>\n"
                    << "\t\t\t<td> - </td>\n";
            } else {
                ss 
                    << "\t\t\t<td>" << parse_icon(i->path(), false) << " <a href=\""
                    << i->path().filename().string() << "\">" << i->path().filename().string() << "</a></td>"
                    << "\t\t\t<td class=\"text-secondary\"><i class=\"bi bi-calendar2-day\"></i> " << 
                        format_date(boost::filesystem::last_write_time(i->path())) << "</td>\n"
                    << "\t\t\t<td class=\"text-secondary\">" << format_size(size) << "</td>\n";
            }
            ss << "\t\t</tr>\n";

        }
        ss << R"HTML(
	</tbody>
</table>

</div></div>

</div>
</body>
</html>)HTML";

        return ss.str();
    }

    std::pair<std::size_t, std::size_t> parse_range(std::string_view range) {
        std::size_t eq = range.find_first_of('=');
        std::size_t sl = range.find_first_of('-', eq);
        
        std::size_t start, end;
        boost::charconv::from_chars(range.substr(eq+1, sl-eq), start);
        boost::charconv::from_chars(range.substr(sl+1), end);
        if (end == 0 || end >= size_) end = size_ - 1;
        return std::make_pair(start, end);
    }

    friend class acceptor;
};

class acceptor: public boost::asio::coroutine {
    boost::asio::io_context& io_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket     socket_;
    boost::asio::ip::tcp::endpoint  address_;

public:
    acceptor(boost::asio::io_context& io)
    : io_(io)
    , acceptor_(io_, boost::asio::ip::tcp::endpoint{boost::asio::ip::make_address("::"), 3000})
    , socket_(io_) {}

    void operator()(const boost::system::error_code& error = {}) { BOOST_ASIO_CORO_REENTER(this) {
        do {
            BOOST_ASIO_CORO_YIELD acceptor_.async_accept(socket_, address_, std::ref(*this));
            if (error) break;

            std::make_shared<handler>(io_, std::move(socket_), address_)->operator()();
        } while(!error);
    }}
};

class server {
    boost::asio::io_context& io_;
    int size_;
public:
    explicit server(boost::asio::io_context& io, int size)
    : io_(io), size_(size) {}

    void run() {
        acceptor acc{io_};
        acc.operator()();

        std::vector<boost::thread> workers;
        for (int i=0;i<size_;i++) {
            workers.emplace_back([this] () {
                io_.run();
            });
        }
        for (int i=0;i<size_;i++) {
            workers[i].join();
        }
    }
};

int main(int argc, char* argv[]) {
#ifdef __linux__
    signal(SIGPIPE, SIG_IGN);
#endif // __linux_
    int size = boost::thread::hardware_concurrency() / 2;
    boost::asio::io_context io { size };

    server(io, size).run();
    return 0;
}