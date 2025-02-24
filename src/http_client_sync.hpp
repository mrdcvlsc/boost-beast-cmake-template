#ifndef BOOST_BEAST_CMAKE_PROJECT_TEMPLATE_HTTP_CLIENT_SYNC_HPP
#define BOOST_BEAST_CMAKE_PROJECT_TEMPLATE_HTTP_CLIENT_SYNC_HPP

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>

#include <boost/beast/http.hpp>

#include <boost/beast/ssl.hpp>

#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>

#include <cstdlib>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class http_client_sync {

  std::string host;
  std::string port;

  tcp::resolver resolver;
  beast::ssl_stream<beast::tcp_stream> stream;
  http::response<http::dynamic_body> response;

public:
  // : resolver(ioc) ?
  http_client_sync(net::io_context &ioc, ssl::context &ctx);

  void establish_connection(std::string_view host, std::string_view port);

  void send_request(http::verb, const std::string &path);

  http::response<http::dynamic_body> received_response();
};

#endif