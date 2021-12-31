//
// Created by madhur on 31/12/2021.
//
#pragma once

#include "config.h"

#include <boost/beast/ssl/ssl_stream.hpp>

#include <simdjson.h>

#include "fmt/ranges.h"

#include <iostream>
#include <string_view>

namespace pt {

struct depth_level {
  double px;
  double qty;
};
using depth_levels = std::vector<depth_level>;

struct order_book {
  depth_levels bids;
  depth_levels asks;
};

struct bit_connector {
  bit_connector(const net::any_io_executor &exec, net::ssl::context ctx)
      : ws_(std::move(exec), ctx) {
    ws_.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::client));
  }

  net::awaitable<void> connect(std::string_view url);
  net::awaitable<void> subscribe(std::string_view pair);
  net::awaitable<const order_book *> book();

private:
  websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
  beast::flat_buffer buf_;
  simdjson::ondemand::parser parser_;
  simdjson::padded_string padded_str_;
  std::uint64_t seq_num_ = 0;
  order_book book_;
};

} // namespace pt