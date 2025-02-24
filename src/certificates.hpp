#ifndef BOOST_BEAST_CMAKE_PROJECT_TEMPLATE_CERTIFICATES_HPP
#define BOOST_BEAST_CMAKE_PROJECT_TEMPLATE_CERTIFICATES_HPP

#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl.hpp>

#include <cstdlib>

namespace ssl = boost::asio::ssl;

bool verify_certificate(bool preverified, ssl::verify_context &ctx);
ssl::context load_certificates();

#endif