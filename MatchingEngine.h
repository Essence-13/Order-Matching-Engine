#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include "OrderBookTypes.h" // <-- Includes our new types
#include <vector>

// Contains the core logic for matching buy and sell orders.
class MatchingEngine {
public:
    // Matches a new buy order against the existing sell book.
    std::vector<Trade> matchBuyOrder(Order& newBuyOrder,
                                     SellBookType& sellOrders,
                                     SellIterMap& sell_order_iterators,
                                     int& tradeId);

    // Matches a new sell order against the existing buy book.
    std::vector<Trade> matchSellOrder(Order& newSellOrder,
                                      BuyBookType& buyOrders,
                                      BuyIterMap& buy_order_iterators,
                                      int& tradeId);
private:
    time_t getCurrentTimestamp() const;
};

#endif // MATCHING_ENGINE_H

