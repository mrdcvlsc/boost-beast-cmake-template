#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

// for root certificates
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>

#include <cstdlib>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Custom verification callback
bool verify_certificate(bool preverified, ssl::verify_context &ctx) {
  char subject_name[256];
  X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
  X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

  // log certificate information
  std::cout << "Verifying " << subject_name << "\n";

  // TO LEARN:
  // here you can implement additional custom verification
  // for example, certificate pinning or specific organizational checks

  return preverified;
}

// function to load root certificates into an ssl::context
void load_root_certificates(ssl::context &ctx) {
  ctx.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);

  // set default verification paths (system's root CA certificates)
  ctx.set_default_verify_paths();

  // set custom verification callback
  ctx.set_verify_callback(verify_certificate);

  // set strong cipher list
  SSL_CTX_set_cipher_list(ctx.native_handle(), "ECDHE-ECDSA-AES256-GCM-SHA384:"
                                               "ECDHE-RSA-AES256-GCM-SHA384:"
                                               "ECDHE-ECDSA-CHACHA20-POLY1305:"
                                               "ECDHE-RSA-CHACHA20-POLY1305");
}

int main() {
  try {
    // IO context required for all I/O
    net::io_context ioc;

    // SSL context for HTTPS
    ssl::context ctx{ssl::context::tlsv12_client};
    load_root_certificates(ctx);

    // these objects perform our I/O
    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    // set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle(), "example.com")) {
      throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()), "Failed to set SNI Hostname");
    }

    // look up the domain name
    auto const results = resolver.resolve("example.com", "443");

    // make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(stream).connect(results);

    // perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    // set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, "/", 11};
    req.set(http::field::host, "example.com");
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // send the HTTP request to the remote host
    http::write(stream, req);

    // this buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // declare a container to hold the response
    http::response<http::dynamic_body> res;

    // receive the HTTP response
    http::read(stream, buffer, res);

    // write the response to standard out
    std::cout << res << '\n';

    // gracefully close the stream
    beast::error_code ec;
    stream.shutdown(ec);

    // not_connected happens sometimes so don't bother reporting it
    if (ec && ec != beast::errc::not_connected) {
      throw beast::system_error{ec};
    }
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}