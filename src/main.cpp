#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

// For root certificates
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/asio/ssl/context.hpp>

#include <cstdlib>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Function to load root certificates into an ssl::context
void load_root_certificates(ssl::context& ctx) {
    // Unfortunately, we have to trust the host's certificate store
    // You might want to implement proper certificate verification here
    ctx.set_verify_mode(ssl::verify_none);
}

int main() {
    try {
        // IO context required for all I/O
        net::io_context ioc;

        // SSL context for HTTPS
        ssl::context ctx{ssl::context::tlsv12_client};
        load_root_certificates(ctx);

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(!SSL_set_tlsext_host_name(stream.native_handle(), "example.com")) {
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        }

        // Look up the domain name
        auto const results = resolver.resolve("example.com", "443");

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, "/", 11};
        req.set(http::field::host, "example.com");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the response to standard out
        std::cout << res << std::endl;

        // Gracefully close the stream
        beast::error_code ec;
        stream.shutdown(ec);

        // not_connected happens sometimes so don't bother reporting it
        if(ec && ec != beast::errc::not_connected) {
            throw beast::system_error{ec};
        }
    }
    catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}