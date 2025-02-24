#ifndef BOOST_BEAST_CMAKE_PROJECT_TEMPLATE_HTTP_CLIENT_ASYNC_HPP
#define BOOST_BEAST_CMAKE_PROJECT_TEMPLATE_HTTP_CLIENT_ASYNC_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/system/error_code.hpp>

#include <cstdlib>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class https_client_async {

  http::request<http::empty_body> request;
  http::response<http::string_body> response;

  // these objects perform our I/O
  tcp::resolver resolver;
  beast::ssl_stream<beast::tcp_stream> stream;
  beast::flat_buffer buffer;

public:
  https_client_async(net::io_context &ioc, ssl::context &ctx);

  void start_http_transaction(const std::string &host, const std::string &port, const std::string &path);

private:
  void fail(beast::error_code ec, char const *what);
  void send_request(const std::string &host, const std::string &path);
  auto received_response();
};

#endif