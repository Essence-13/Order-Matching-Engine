#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "Order.h"
#include "Logger.h"
#include "Persistence.h"
#include "MatchingEngine.h"

#include <map>
#include <queue>
#include <unordered_map>
#include <memory>

// The central class that orchestrates the entire process.
class OrderBook {
public:
    OrderBook(std::shared_ptr<Logger> logger);
    ~OrderBook();

    // Places a new order and attempts to match it.
    void placeOrder(OrderType type, int price, int quantity);

    // Cancels an existing order.
    void cancelOrder(int id);
    
    // Displays the top of the buy and sell books.
    void showBook() const;

private:
    int nextOrderId = 1;
    int nextTradeId = 1;

    std::map<int, std::queue<Order>, std::greater<int>> buyOrders; // Buys, sorted high to low
    std::map<int, std::queue<Order>> sellOrders;                   // Sells, sorted low to high

    // For fast lookups and status tracking
    std::unordered_map<int, Order> allOrders;

    std::shared_ptr<Logger> logger;
    std::unique_ptr<PersistenceManager> persistence;
    std::unique_ptr<MatchingEngine> matchingEngine;

    void processTrades(const std::vector<Trade>& trades);
    time_t getCurrentTimestamp() const;
    void updateOrderStatus(int orderId);
};

#endif // ORDER_BOOK_H
