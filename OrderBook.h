#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "OrderBookTypes.h" // <-- Includes our new types
#include "Logger.h"
#include "Persistence.h"
#include "MatchingEngine.h"
#include <memory>

// The central class that orchestrates the entire process.
class OrderBook {
public:
    OrderBook(std::shared_ptr<Logger> logger);
    ~OrderBook();

    // Places a new order and attempts to match it.
    void placeOrder(OrderType type, int price, int quantity);

    // Cancels an existing order.
    bool cancelOrder(int id);
    
    // Displays the top of the buy and sell books.
    void showBook() const;

private:
    int nextOrderId = 1;
    int nextTradeId = 1;

    // The two order books
    BuyBookType buyOrders;
    SellBookType sellOrders;

    // The two iterator maps for O(1) cancels
    BuyIterMap buy_order_iterators;
    SellIterMap sell_order_iterators;

    // Member pointers to our helper classes
    std::shared_ptr<Logger> logger;
    std::unique_ptr<PersistenceManager> persistence;
    std::unique_ptr<MatchingEngine> matchingEngine;

    // Private helper functions
    void processTrades(const std::vector<Trade>& trades);
    time_t getCurrentTimestamp() const;
};

#endif // ORDER_BOOK_H

