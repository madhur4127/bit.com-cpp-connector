//
// Created by madhur on 31/12/2021.
//

#include "connector.h"

net::awaitable<void> start(std::string_view url, std::string_view pair) {
  pt::bit_connector connector(co_await net::this_coro::executor,
                              net::ssl::context{net::ssl::context::tls_client});
  co_await connector.connect(url);
  fmt::print("Subscribing for {}\n", pair);
  co_await connector.subscribe(pair);
  for (;;) {
    // TODO: change to generator
    const pt::order_book &book = *(co_await connector.book());
    fmt::print("\033c");
    fmt::print("Received book update\n"); // magic spell
    for (int i = 0, lim = std::min(book.asks.size(), book.bids.size()); i < lim;
         ++i) {
      fmt::print("{:.6f}@{:.6f}\t{:.6f}@{:.6f}\n", book.bids[i].qty,
                 book.bids[i].px, book.asks[i].qty, book.asks[i].px);
    }
  }
}

int main(int argc, char **argv) {
  std::string_view url = "spot-ws.bit.com";
  std::string_view pair = argv[1];
  net::io_context ioc;
  net::co_spawn(ioc.get_executor(), start(url, pair), net::detached);
  ioc.run();
}
