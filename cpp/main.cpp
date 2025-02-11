#include <boost/asio.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>
#include <boost/url.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

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

class handler: public boost::asio::coroutine, public std::enable_shared_from_this<handler> {
    boost::asio::io_context& io_;
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::ip::tcp::endpoint address_;

    boost::beast::http::request<boost::beast::http::empty_body>    req_;
    boost::url url_;
    boost::beast::flat_buffer buf_;
    boost::beast::http::response<boost::beast::http::string_body>  rsp_;
    boost::beast::http::response<boost::beast::http::file_body>   file_;

    boost::filesystem::path path_;
    boost::filesystem::file_status stat_;

public:
    explicit handler(boost::asio::io_context& io, boost::asio::ip::tcp::socket&& socket,
        boost::asio::ip::tcp::endpoint address)
    : io_(io)
    , socket_(std::move(socket)) {}

    void operator()(boost::system::error_code error = {}, std::size_t = 0) { BOOST_ASIO_CORO_REENTER(this) {
        do {
            BOOST_ASIO_CORO_YIELD boost::beast::http::async_read(socket_, buf_, req_, callback(this));
            if (error) break;

            url_ = boost::url(req_.target());
            path_ = "." / boost::filesystem::path(url_.path()).lexically_normal();
            stat_ = boost::filesystem::status(path_, error);
            BOOST_LOG_TRIVIAL(debug) << req_.method_string() << " " << path_;
            if (error) break;
            if (stat_.type() == boost::filesystem::directory_file) {
                rsp_.set("content-type", "text/html");
                rsp_.body() = build_directory(path_);
                rsp_.prepare_payload();
                BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(socket_, rsp_, callback(this));
            } else if (req_.count("Range")) {

            } else {
                file_.body().open(path_.c_str(), boost::beast::file_mode::read, error);
                file_.prepare_payload();
                if (error) break;
                BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(socket_, file_, callback(this));
            }
            
        } while(req_.keep_alive() && !req_.need_eof());

        if (error) 
            if (error != boost::system::errc::no_such_file_or_directory)
                BOOST_LOG_TRIVIAL(error) << "error = (" << error << ") " << error.message(); 
    }}

    std::string build_directory(const boost::filesystem::path& path) {
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
	<tbody>
		<tr><td><i class="bi bi-box-arrow-in-up"></i> <a href="../">上一层</a></td><td>&nbsp;</td><td>&nbsp;</td></tr>
    )HTML";
        for (auto i = boost::filesystem::directory_iterator(path); i!= boost::filesystem::directory_iterator(); i++) {
            if (i->path().filename().string()[0] == '.' || i->path().filename() == "node_modules")
                continue;

            ss << "\t\t<tr>"
                << "\t\t\t<td>" << parse_icon(i->path(), i->status()) << " <a href=\"";
                
            if (i->status().type() == boost::filesystem::file_type::directory_file) {
                ss << i->path().filename().string() << "/\">" << i->path().filename().string() << "</a></td>"
                    << "\t\t\t<td> - </td>\n"
                    << "\t\t\t<td> - </td>\n";
            } else {
                ss << i->path().filename().string() << "\">" << i->path().filename().string() << "</a></td>"
                    << "\t\t\t<td class=\"text-secondary\"><i class=\"bi bi-calendar2-day\"></i> " << "date" << "</td>\n"
                    << "\t\t\t<td class=\"text-secondary\">" << format_size(i->path().size()) << "</td>\n";
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
    int size = boost::thread::hardware_concurrency() / 2;
    boost::asio::io_context io { size };

    server(io, size).run();
    return 0;
}