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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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

void load_root_certificates(ssl::context &ctx) {
  // set verification mode to verify peer certificate
  ctx.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);

  // Check if the CA file exists.
  auto path_certificate = std::filesystem::current_path();
  path_certificate /= "cacert.pem";

  std::cout << "current path : " << path_certificate << '\n';

  std::ifstream file_certificate(path_certificate.string());
  if (file_certificate.good()) {
    std::cout << "Loading CA certificates from cacert.pem\n";
    ctx.load_verify_file(path_certificate.string());
  } else {
    std::cout << "cacert.pem not found. Using system default verification paths.\n";
    ctx.set_default_verify_paths();
  }

  // set custom verification callback
  ctx.set_verify_callback(verify_certificate);

  // set strong cipher list
  SSL_CTX_set_cipher_list(ctx.native_handle(), "ECDHE-ECDSA-AES256-GCM-SHA384:"
                                               "ECDHE-RSA-AES256-GCM-SHA384:"
                                               "ECDHE-ECDSA-CHACHA20-POLY1305:"
                                               "ECDHE-RSA-CHACHA20-POLY1305");
}

class https_client {
public:
  https_client(net::io_context &ioc, ssl::context &ctx) : resolver_(net::make_strand(ioc)), stream_(net::make_strand(ioc), ctx) {}

  void run(const std::string &host, const std::string &port, const std::string &path) {
    // set SNI hostname
    if (!SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str())) {
      throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()), "Failed to set SNI Hostname");
    }

    // set timeout for all operations
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // look up the domain name
    resolver_.async_resolve(host, port, [this, host, path](beast::error_code ec, tcp::resolver::results_type results) {
      if (ec) {
        return fail(ec, "resolve");
      }

      // connect to the endpoint
      beast::get_lowest_layer(stream_).async_connect(results, [this, host, path](beast::error_code ec, tcp::endpoint) {
        if (ec) {
          return fail(ec, "connect");
        }

        // perform SSL handshake
        stream_.async_handshake(ssl::stream_base::client, [this, host, path](beast::error_code ec) {
          if (ec) {
            return fail(ec, "handshake");
          }

          send_request(host, path);
        });
      });
    });
  }

private:
  void fail(beast::error_code ec, char const *what) { std::cerr << what << ": " << ec.message() << "\n"; }

  void send_request(const std::string &host, const std::string &path) {
    // set up HTTP request
    req_.version(11);
    req_.method(http::verb::get);
    req_.target(path);
    req_.set(http::field::host, host);
    req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // send the request
    http::async_write(stream_, req_, [this](beast::error_code ec, std::size_t) {
      if (ec) {
        return fail(ec, "write");
      }

      // receive the response
      http::async_read(stream_, buffer_, res_, [this](beast::error_code ec, std::size_t) {
        if (ec) {
          return fail(ec, "read");
        }

        // response headers
        std::cout << "Response Headers:\n";
        for (const auto &field : res_) {
          std::cout << field.name_string() << ": " << field.value() << "\n";
        }

        std::cout << "\n";

        // response body
        std::cout << "Response Body:\n";
        std::cout << res_.body() << "\n";

        // shutdown SSL connection
        stream_.async_shutdown([this](beast::error_code ec) {
          if (ec && ec != net::error::eof) {
            return fail(ec, "shutdown");
          }
        });
      });
    });
  }

  tcp::resolver resolver_;
  beast::ssl_stream<beast::tcp_stream> stream_;
  beast::flat_buffer buffer_;
  http::request<http::empty_body> req_;
  http::response<http::string_body> res_;
};

int main(int _, const char **argv) {
  std::cout << "program path : " << argv[0] << '\n';

  try {
    net::io_context ioc;

    // create SSL context with TLS 1.3 (fallback to 1.2 if not available)
    ssl::context ctx{ssl::context::tlsv13_client};
    load_root_certificates(ctx);

    https_client client(ioc, ctx);
    client.run("example.com", "443", "/");

    ioc.run();
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}