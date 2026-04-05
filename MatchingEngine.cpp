#include "MatchingEngine.h"
#include "OrderBookTypes.h" // <-- Include the new types
#include <algorithm> // For std::min

// --- matchBuyOrder ---
// Matches a new BUY order against the sell book
std::vector<Trade> MatchingEngine::matchBuyOrder(Order& newBuyOrder,
                                                 SellBookType& sellOrders,
                                                 SellIterMap& sell_order_iterators,
                                                 int& tradeId) {
    std::vector<Trade> trades;

    // Iterate through sell orders, from lowest price (begin()) upwards.
    // The loop continues as long as 'it' is valid AND the newBuyOrder is not filled.
    // We do not increment 'it' here; we do it manually inside the loop.
    for (auto it = sellOrders.begin(); it != sellOrders.end() && !newBuyOrder.is_filled(); /* no increment */) {

        // Get a pointer to the list of orders at this price level.
        // We use a pointer because 'it' might be erased, invalidating a reference.
        std::list<Order>* price_level_list = &it->second;

        if (newBuyOrder.price < it->first) {
            // Buyer's price is too low to match even the best sell price. Stop.
            break;
        }

        // --- Inner Loop: Iterate through orders at this price level ---
        // Continue as long as the list is not empty AND the newBuyOrder is not filled.
        while (!price_level_list->empty() && !newBuyOrder.is_filled()) {
            
            // Get a reference to the oldest order (front of the list)
            Order& sell_order = price_level_list->front();

            // Calculate traded quantity
            int tradedQty = std::min(newBuyOrder.remaining(), sell_order.remaining());

            if (tradedQty > 0) {
                // Update quantities for both orders
                newBuyOrder.filled_quantity += tradedQty;
                sell_order.filled_quantity += tradedQty;

                // Create the trade record
                trades.push_back({tradeId++, newBuyOrder.id, sell_order.id, sell_order.price, tradedQty, getCurrentTimestamp()});
            }

            // Check if the resting order (sell_order) is now fully filled
            if (sell_order.is_filled()) {
                // It's full. Remove it.
                // 1. Erase from the lookup map
                sell_order_iterators.erase(sell_order.id);
                // 2. Erase from the list
                price_level_list->pop_front();
            } else {
                // sell_order is NOT filled, which means newBuyOrder MUST be filled.
                // Stop matching at this price level and exit the function.
                return trades;
            }
        } // --- End of Inner Loop ---

        // We are outside the inner loop.
        // This means either newBuyOrder is full (which the 'for' loop condition will catch)
        // OR the price_level_list is now empty.

        if (price_level_list->empty()) {
            // The list is empty. Erase this price level from the map.
            // 'erase(it)' returns an iterator to the *next* price level.
            it = sellOrders.erase(it);
        } else {
            // The list is not empty (meaning newBuyOrder got filled)
            // Manually advance to the next price level.
            ++it;
        }
    } // --- End of Outer Loop ---

    return trades;
}


// --- matchSellOrder ---
// Matches a new SELL order against the buy book
std::vector<Trade> MatchingEngine::matchSellOrder(Order& newSellOrder,
                                                  BuyBookType& buyOrders,
                                                  BuyIterMap& buy_order_iterators,
                                                  int& tradeId) {
    std::vector<Trade> trades;

    // Iterate through buy orders, from highest price (begin()) downwards.
    for (auto it = buyOrders.begin(); it != buyOrders.end() && !newSellOrder.is_filled(); /* no increment */) {

        std::list<Order>* price_level_list = &it->second;

        if (newSellOrder.price > it->first) {
            // Seller's price is too high to match even the best buy price. Stop.
            break;
        }

        // --- Inner Loop: Iterate through orders at this price level ---
        while (!price_level_list->empty() && !newSellOrder.is_filled()) {
            
            Order& buy_order = price_level_list->front();

            int tradedQty = std::min(newSellOrder.remaining(), buy_order.remaining());

            if (tradedQty > 0) {
                newSellOrder.filled_quantity += tradedQty;
                buy_order.filled_quantity += tradedQty;
                trades.push_back({tradeId++, buy_order.id, newSellOrder.id, buy_order.price, tradedQty, getCurrentTimestamp()});
            }

            if (buy_order.is_filled()) {
                // It's full. Remove it.
                // 1. Erase from lookup map
                buy_order_iterators.erase(buy_order.id);
                // 2. Erase from list
                price_level_list->pop_front();
            } else {
                // buy_order is NOT filled, which means newSellOrder MUST be filled.
                // Stop matching and exit.
                return trades;
            }
        } // --- End of Inner Loop ---

        if (price_level_list->empty()) {
            // The list is empty. Erase this price level and advance 'it'
            it = buyOrders.erase(it);
        } else {
            // The list is not empty (newSellOrder is full). Advance 'it' normally
            ++it;
        }
    } // --- End of Outer Loop ---

    return trades;
}

time_t MatchingEngine::getCurrentTimestamp() const {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

