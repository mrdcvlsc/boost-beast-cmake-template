#include <iostream>

#include "certificates.hpp"
#include "http_client_sync.hpp"

int main() {
  try {
    // SSL context for HTTPS
    auto ctx = load_certificates();

    // IO context required for all I/O
    net::io_context ioc;

    http_client_sync client(ioc, ctx);
    client.establish_connection("example.com", "443");
    client.send_request(http::verb::get, "/");
    std::cout << client.received_response() << '\n';

  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}