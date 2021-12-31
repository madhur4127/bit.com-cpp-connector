//
// Created by madhur on 31/12/2021.
//

#include "connector.h"

namespace pt {

namespace {

void parse_depth_levels(depth_levels &out, simdjson::ondemand::array levels) {
  out.clear();
  out.reserve(levels.count_elements());
  for (simdjson::ondemand::array level : levels) {
    depth_level &cur = out.emplace_back();
    auto it = level.begin();
    cur.px = (*it).get_double_in_string();
    cur.qty = (*(++it)).get_double_in_string();
  }
}

net::awaitable<net::ip::tcp::resolver::results_type>
resolve(std::string_view host, std::string_view port) {
  net::ip::tcp::resolver resolver(co_await net::this_coro::executor);
  auto results =
      co_await resolver.async_resolve(host, port, boost::asio::use_awaitable);
  fmt::print("resolved {}:{}\n", host, port);
  co_return results;
}

void set_sni_(beast::ssl_stream<beast::tcp_stream> &ws, std::string_view url) {
  // credit:
  //   https://github.com/boostorg/beast/blob/develop/example/websocket/client/sync-ssl/websocket_client_sync_ssl.cpp

  // Set SNI Hostname (many hosts need this to handshake successfully)
  if (!SSL_set_tlsext_host_name(ws.native_handle(), url.data())) {
    throw std::system_error(boost::system::error_code(
        (int)(::ERR_get_error()), boost::asio::error::get_ssl_category()));
  }
}

} // namespace

net::awaitable<void> bit_connector::connect(std::string_view url) {
  auto results = co_await resolve(url, "443");

  fmt::print("Connecting TCP socket\n");
  auto &stream = beast::get_lowest_layer(ws_);
  stream.expires_after(std::chrono::seconds{3});
  co_await stream.async_connect(results, boost::asio::use_awaitable);

  fmt::print("Performing SSL handshake\n");
  auto &ssl_stream = ws_.next_layer();
  set_sni_(ssl_stream, url);
  co_await ssl_stream.async_handshake(net::ssl::stream_base::client,
                                      net::use_awaitable);

  fmt::print("Performing Websocket Handshake\n");
  beast::websocket::response_type response;
  co_await ws_.async_handshake(response, url.data(), "/", net::use_awaitable);
  fmt::print("Connected!\n");

  stream.expires_never();
}

net::awaitable<void> bit_connector::subscribe(std::string_view pair) {
  // TODO: allow other subscription too in future
  std::string msg = fmt::format(
      R"({{"type":"subscribe","pairs":["{}"],"channels":["order_book.10.10"],"interval":"100ms"}})",
      pair);
  net::const_buffer buf(msg.data(), msg.size());
  beast::error_code ec;
  co_await ws_.async_write(buf, net::use_awaitable);
  std::cout << "[TX] " << beast::make_printable(buf) << "\n";

  co_await ws_.async_read(buf_, net::use_awaitable);
  std::cout << "[RX] " << beast::make_printable(buf_.cdata()) << "\n";
  // TODO: check for actual subscription here
  buf_.consume(buf_.size());
}

net::awaitable<const order_book *> bit_connector::book() {
  // TODO: check for subscription failure
  co_await ws_.async_read(buf_, net::use_awaitable);
  const net::const_buffer &cbuf = buf_.cdata();
  std::cout << "[RX] " << beast::make_printable(cbuf) << "\n";
  padded_str_ = simdjson::padded_string{(const char *)cbuf.data(), cbuf.size()};
  simdjson::ondemand::document doc = parser_.iterate(padded_str_);
  simdjson::ondemand::object data = doc["data"];
  if (std::uint64_t seq = data["sequence"]; seq < seq_num_) {
    fmt::print("ERROR: ooo msgs\n");
    throw std::runtime_error("ooo msgs");
  } else {
    seq_num_ = seq;
  }
  parse_depth_levels(book_.bids, data["bids"]);
  parse_depth_levels(book_.asks, data["asks"]);
  buf_.consume(buf_.size());
  co_return &book_;
}

} // namespace pt