#include "certificates.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/version.hpp>

#include <boost/beast/http.hpp>

#include <boost/beast/ssl.hpp>

#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>

#include <exception>
#include <iostream>

#include <cstdlib>

#include "http_client_sync.hpp"

namespace fs = std::filesystem;
namespace asio = boost::asio;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

int main() {
  try {
   
    auto ctx = load_certificates_SSL_TLS();

    net::io_context ioc;

    http_client_sync client;

    client.establish_connection("pastebin.com", "https");

    // Create HTTP POST request
    http::request<http::string_body> request(http::verb::post, "/api/api_post.php", 11);
    request.set(http::field::content_type, "application/x-www-form-urlencoded");

    std::string body = "api_option=paste"
                       "&api_dev_key=<pastebin-api-dev-key>"
                       "&api_paste_code=wi w wi, uwa wa, wi wi wi, uyaya"
                       "&api_paste_expire_date=1D"
                       "&api_paste_private=1"
                       "&api_paste_format=text"
                       "&api_paste_name=nekko";

    request.set(http::field::content_length, std::to_string(body.size()));
    request.body() = body;
    request.prepare_payload();

    auto response = client.fetch(request);

    std::cout << "response :\n\n" << response << '\n';

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
  }

  return 0;
}
