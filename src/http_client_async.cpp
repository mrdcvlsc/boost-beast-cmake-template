#include "http_client_async.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/system/error_code.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

https_client_async::https_client_async(net::io_context &ioc, ssl::context &ctx) : resolver(net::make_strand(ioc)), stream(net::make_strand(ioc), ctx) {}

void https_client_async::start_http_transaction(const std::string &host, const std::string &port, const std::string &path) {
  // set SNI hostname
  if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
    throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()), "Failed to set SNI Hostname");
  }

  // set timeout for all operations
  beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

  // look up the domain name
  resolver.async_resolve(host, port, [this, host, path](beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
      return fail(ec, "resolve");
    }

    // connect to the endpoint
    beast::get_lowest_layer(stream).async_connect(results, [this, host, path](beast::error_code ec, tcp::endpoint) {
      if (ec) {
        return fail(ec, "connect");
      }

      // perform SSL handshake
      stream.async_handshake(ssl::stream_base::client, [this, host, path](beast::error_code ec) {
        if (ec) {
          return fail(ec, "handshake");
        }

        send_request(host, path);
      });
    });
  });
}

void https_client_async::fail(beast::error_code ec, char const *what) { std::cerr << what << ": " << ec.message() << "\n"; }

void https_client_async::send_request(const std::string &host, const std::string &path) {
  // set up HTTP request
  request.version(11);
  request.method(http::verb::get);
  request.target(path);
  request.set(http::field::host, host);
  request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  // send the request
  http::async_write(stream, request, [this](beast::error_code ec, std::size_t) {
    if (ec) {
      return fail(ec, "write");
    }

    // receive the response
    http::async_read(stream, buffer, response, [this](beast::error_code ec, std::size_t) {
      if (ec) {
        return fail(ec, "read");
      }

      // response headers
      std::cout << "Response Headers:\n";
      for (const auto &field : response) {
        std::cout << field.name_string() << ": " << field.value() << "\n";
      }

      std::cout << "\n";

      // response body
      std::cout << "Response Body:\n";
      std::cout << response.body() << "\n";

      // shutdown SSL connection
      stream.async_shutdown([this](beast::error_code ec) {
        if (ec && ec != net::error::eof && ec != beast::errc::not_connected) {
          return fail(ec, "shutdown");
        }
      });
    });
  });
}
