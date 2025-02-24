#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>

#include <boost/beast/http.hpp>

#include <boost/beast/ssl.hpp>

#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "certificates.hpp"

namespace fs = std::filesystem;

// Custom verification callback
bool verify_certificate(bool preverified, ssl::verify_context &ctx) {
  X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

  if (!cert) {
    std::cerr << "No certificate available for verification.\n";
    return false;
  }

  char subject_name[256];
  X509_NAME_oneline(X509_get_subject_name(cert), subject_name, sizeof(subject_name));

  std::cout << "Verifying " << subject_name << "\n";

  // TO LEARN:
  // here you can implement additional custom verification
  // for example, certificate pinning or specific organizational checks

  return preverified;
}

// function to load root certificates into an ssl::context
ssl::context load_certificates() {
  ssl::context ctx{ssl::context::tlsv13_client};

  // ctx.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert); // for server
  ctx.set_verify_mode(ssl::verify_peer); // for client

  // check if mozilla CA bundle from curl exists
  auto curl_mozilla_ca = fs::current_path() / "certs" / "cacert.pem";
  std::cout << "curl mozilla ca path: " << curl_mozilla_ca << '\n';

  std::ifstream file_certificate(curl_mozilla_ca.string());
  if (file_certificate.good()) {
    std::cout << "Loading CA certificates from cacert.pem\n";
    ctx.load_verify_file(curl_mozilla_ca.string());
  } else {
    std::cout << "cacert.pem not found. Using system default verification paths.\n";
  }

  // set custom verification callback
  ctx.set_verify_callback(verify_certificate);

  // for servers:
  // SSL_CTX_set_cipher_list(ctx.native_handle(), "ECDHE-ECDSA-AES256-GCM-SHA384:"
  //                                              "ECDHE-RSA-AES256-GCM-SHA384:"
  //                                              "ECDHE-ECDSA-CHACHA20-POLY1305:"
  //                                              "ECDHE-RSA-CHACHA20-POLY1305");

  // for clients:
  SSL_CTX_set_ciphersuites(ctx.native_handle(), "TLS_AES_256_GCM_SHA384:"
                                                "TLS_CHACHA20_POLY1305_SHA256");

  return ctx;
}