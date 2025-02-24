#include "http_client_sync.hpp"

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

// : resolver(ioc) ?
http_client_sync::http_client_sync(net::io_context &ioc, ssl::context &ctx) : resolver(net::make_strand(ioc)), stream(net::make_strand(ioc), ctx) {}

void http_client_sync::establish_connection(std::string_view host, std::string_view port) {

  this->host = host;
  this->port = port;

  // set SNI Hostname (many hosts need this to handshake successfully)
  if (!SSL_set_tlsext_host_name(stream.native_handle(), this->host.data())) {
    throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()), "Failed to set SNI Hostname");
  }

  // look up the domain name
  auto const results = resolver.resolve(this->host, this->port);

  // make the connection on the IP address we get from a lookup
  beast::get_lowest_layer(stream).connect(results);

  // perform the SSL handshake
  stream.handshake(ssl::stream_base::client);
}

void http_client_sync::send_request(http::verb, const std::string &path) {

  // set up an HTTP GET request message
  http::request<http::string_body> request{http::verb::get, path, 11};
  request.set(http::field::host, host);
  request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  // send the HTTP request to the remote host
  http::write(stream, request);

  // this buffer is used for reading and must be persisted
  beast::flat_buffer buffer;

  // declare a container to hold the response
  response = http::response<http::dynamic_body>();

  // receive the HTTP response
  http::read(stream, buffer, response);

  // gracefully close the stream
  beast::error_code ec;
  stream.shutdown(ec);

  // not_connected happens sometimes so don't bother reporting it
  if (ec && ec != beast::errc::not_connected) {
    throw beast::system_error{ec};
  }
}

http::response<http::dynamic_body> http_client_sync::received_response() { return response; }