#include <iostream>

#include "certificates.hpp"
#include "http_client_async.hpp"

int main() {
  try {
    auto ctx1 = load_certificates();
    auto ctx2 = load_certificates();

    // IO context required for all I/O
    net::io_context ioc;

    https_client_async client1(ioc, ctx1);
    https_client_async client2(ioc, ctx2);

    client1.start_http_transaction("example.com", "443", "/");
    client2.start_http_transaction("api.restful-api.dev", "443", "/objects");

    ioc.run();

  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}