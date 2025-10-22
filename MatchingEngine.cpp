#include "MatchingEngine.h"
#include <algorithm> // For std::min

std::vector<Trade> MatchingEngine::matchBuyOrder(Order& buy, std::map<int, std::queue<Order>>& sellOrders, int& tradeId) {
    std::vector<Trade> trades;
    
    // Iterate through sell orders, from lowest price upwards.
    for (auto it = sellOrders.begin(); it != sellOrders.end() && !buy.is_filled();) {
        if (buy.price < it->first) {
            // The buyer's price is lower than the best seller's price, no more matches possible.
            break;
        }

        auto& q = it->second;
        while (!q.empty() && !buy.is_filled()) {
            Order& sell = q.front();
            int tradedQty = std::min(buy.remaining(), sell.remaining());
            
            // Create a trade record
            trades.push_back({tradeId++, buy.id, sell.id, sell.price, tradedQty, getCurrentTimestamp()});
            
            buy.filled_quantity += tradedQty;
            sell.filled_quantity += tradedQty;
            
            if (sell.is_filled()) {
                q.pop(); // This sell order is completely filled
            }
        }
        
        if (q.empty()) {
            // Erase the price level if no more orders exist there.
            it = sellOrders.erase(it);
        } else {
            ++it;
        }
    }
    return trades;
}

std::vector<Trade> MatchingEngine::matchSellOrder(Order& sell, std::map<int, std::queue<Order>, std::greater<int>>& buyOrders, int& tradeId) {
    std::vector<Trade> trades;
    
    // Iterate through buy orders, from highest price downwards.
    for (auto it = buyOrders.begin(); it != buyOrders.end() && !sell.is_filled();) {
        if (sell.price > it->first) {
            // The seller's price is higher than the best buyer's price, no more matches possible.
            break;
        }

        auto& q = it->second;
        while (!q.empty() && !sell.is_filled()) {
            Order& buy = q.front();
            int tradedQty = std::min(sell.remaining(), buy.remaining());
            
            // Create a trade record
            trades.push_back({tradeId++, buy.id, sell.id, buy.price, tradedQty, getCurrentTimestamp()});
            
            sell.filled_quantity += tradedQty;
            buy.filled_quantity += tradedQty;
            
            if (buy.is_filled()) {
                q.pop(); // This buy order is completely filled
            }
        }
        
        if (q.empty()) {
            // Erase the price level if no more orders exist there.
            it = buyOrders.erase(it);
        } else {
            ++it;
        }
    }
    return trades;
}

time_t MatchingEngine::getCurrentTimestamp() const {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}
