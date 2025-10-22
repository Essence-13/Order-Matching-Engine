#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include "Order.h"
#include <map>
#include <queue>
#include <vector>

// Contains the core logic for matching buy and sell orders.
class MatchingEngine {
public:
    // Matches a new buy order against the existing sell book.
    std::vector<Trade> matchBuyOrder(Order& newBuyOrder, 
                                     std::map<int, std::queue<Order>>& sellOrders,
                                     int& tradeId);

    // Matches a new sell order against the existing buy book.
    std::vector<Trade> matchSellOrder(Order& newSellOrder, 
                                      std::map<int, std::queue<Order>, std::greater<int>>& buyOrders,
                                      int& tradeId);
private:
    time_t getCurrentTimestamp() const;
};

#endif // MATCHING_ENGINE_H
