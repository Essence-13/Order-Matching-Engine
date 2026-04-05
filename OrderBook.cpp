#include "OrderBook.h"
#include "OrderBookTypes.h" // <-- Include the new types
#include <iostream>
#include <algorithm> // for std::max

// --- Constructor ---
OrderBook::OrderBook(std::shared_ptr<Logger> logger) : logger(logger) {
    #ifndef BENCHMARK_MODE
    // Initialize components
    persistence = std::make_unique<PersistenceManager>("buy_orders.csv", "sell_orders.csv", "trades.csv");
    // matchingEngine = std::make_unique<MatchingEngine>();

    LOG(logger, "System", "Order book initializing...");
    
    // 1. Load existing orders from files into the buy/sell maps
    persistence->loadOrders(buyOrders, sellOrders);
    
    // --- THIS IS THE CRITICAL FIX ---
    // 2. Build the iterator lookup maps from the loaded orders

    LOG(logger, "System", "Building BUY order iterator map...");
    // Iterate through each price level in the buy book
    for (auto& price_level_pair : buyOrders) {
        // Iterate through each order in the list at this price level
        for (auto it = price_level_pair.second.begin(); it != price_level_pair.second.end(); ++it) {
            // Store a "bookmark" (iterator) to this order, keyed by its ID
            buy_order_iterators[it->id] = it;
            // Update nextOrderId to be higher than any loaded ID
            nextOrderId = std::max(nextOrderId, it->id + 1);
        }
    }

    LOG(logger, "System", "Building SELL order iterator map...");
    // Iterate through each price level in the sell book
    for (auto& price_level_pair : sellOrders) {
        // Iterate through each order in the list at this price level
        for (auto it = price_level_pair.second.begin(); it != price_level_pair.second.end(); ++it) {
            // Store a "bookmark" (iterator) to this order, keyed by its ID
            sell_order_iterators[it->id] = it;
            // Update nextOrderId to be higher than any loaded ID
            nextOrderId = std::max(nextOrderId, it->id + 1);
        }
    }
    // --- END CRITICAL FIX ---

    // Ensure tradeId is at least 1
    nextTradeId = 1; 
    #endif
    LOG(logger, "System", "Order book initialized successfully. Next OrderID: " + std::to_string(nextOrderId));
}

// --- Destructor ---
OrderBook::~OrderBook() {
    #ifndef BENCHMARK_MODE
    LOG(logger, "System", "Order book shutting down. Exporting active orders...");
    // Export all active orders on shutdown
    if(persistence) {
        persistence->exportActiveOrders(buyOrders, sellOrders);
    }
    LOG(logger, "System", "Export complete.");
    #endif
}

// --- placeOrder ---
void OrderBook::placeOrder(OrderType type, int price, int quantity) {
    if (price <= 0 || quantity <= 0) {
        LOG(logger, "Error", "Invalid order parameters: price and quantity must be positive.");
        throw std::invalid_argument("Price and quantity must be positive");
    }

    // Create the new order object
    Order order{
        nextOrderId++,
        type,
        price,
        quantity,
        0, // filled_quantity
        getCurrentTimestamp()
    };

 LOG_ORDER(logger, order.id, price, quantity, type == OrderType::BUY, false);

    std::vector<Trade> trades;
    
    // --- Match the order ---
    if (type == OrderType::BUY) {
        trades = matchingEngine->matchBuyOrder(order, sellOrders, sell_order_iterators, nextTradeId);
    } else {
        trades = matchingEngine->matchSellOrder(order, buyOrders, buy_order_iterators, nextTradeId);
    }

    // --- Log any trades that occurred ---
    if (!trades.empty()) {
        processTrades(trades);
    }

    // --- Add to book if not fully filled ---
    if (!order.is_filled()) {
        if (order.type == OrderType::BUY) {
            // 1. Add the order to the end of the list at its price
            buyOrders[order.price].push_back(order);
            
            // 2. Get an iterator to the new order
            //    (std::prev(list.end()) points to the last element)
            auto newOrderIt = std::prev(buyOrders[order.price].end());
            
            // 3. Store the "bookmark" (iterator) in the lookup map
            buy_order_iterators[order.id] = newOrderIt;

        } else { // OrderType::SELL
            // 1. Add the order to the end of the list
            sellOrders[order.price].push_back(order);
            
            // 2. Get an iterator to the new order
            auto newOrderIt = std::prev(sellOrders[order.price].end());
            
            // 3. Store the "bookmark" in the lookup map
            sell_order_iterators[order.id] = newOrderIt;
        }
    } else {
LOG_ORDER(logger, order.id, price, quantity, type == OrderType::BUY, true);    }
    
    // Note: We no longer call exportActiveOrders() here for performance.
    // It will be called once on shutdown in the destructor.
}

// --- processTrades ---
// This function is now ONLY for logging.
void OrderBook::processTrades(const std::vector<Trade>& trades) {
    if(trades.empty()) return;

    for (const auto& trade : trades) {
        // 1. Log to the event log
        LOG_TRADE(logger,
                  trade.tradeId,
                  trade.buyOrderId,
                  trade.sellOrderId,
                  trade.price,
                  trade.quantity);
        // 2. Log to the console (for interactive mode)
        // We will comment this out in the benchmark, but it's fine for now.
        // std::cout << "TRADE: " << trade.quantity << " @ " << trade.price << std::endl;

        // #ifndef BENCHMARK_MODE
        // // 3. Log to the trades.csv file
        // persistence->logTrade(trade);
        // #endif
    }
}

// --- cancelOrder ---
bool OrderBook::cancelOrder(int id) {
    
    // 1. Check the BUY lookup map
    if (buy_order_iterators.count(id)) {
        
        // Get the iterator from the BUY lookup map
        auto it = buy_order_iterators.at(id);
        int price = it->price;

        // Erase the order from the BUY list (O(1) operation)
        buyOrders[price].erase(it);

        // If that list is now empty, erase the price level
        if (buyOrders[price].empty()) {
            buyOrders.erase(price);
        }

        // Erase the order from the BUY lookup map
        buy_order_iterators.erase(id);
        
        LOG(logger, "Order", "Cancelled BUY order ID " + std::to_string(id));
    return true;
    // 2. Check the SELL lookup map
    } else if (sell_order_iterators.count(id)) {
        
        // Get the iterator from the SELL lookup map
        auto it = sell_order_iterators.at(id);
        int price = it->price;

        // Erase the order from the SELL list (O(1) operation)
        sellOrders[price].erase(it);

        // If that list is now empty, erase the price level
        if (sellOrders[price].empty()) {
            sellOrders.erase(price);
        }

        // Erase the order from the SELL lookup map
        sell_order_iterators.erase(id);

        LOG(logger, "Order", "Cancelled SELL order ID " + std::to_string(id));
        return true;
    } else {
        // 3. Not found in either map
        LOG(logger, "Error", "Cancel failed - order ID " + std::to_string(id) + " not found");
        return false;
        // throw std::runtime_error("Order ID not found");
    }
    return false;
}

// --- showBook ---
void OrderBook::showBook() const {
    std::cout << "\n--- ORDER BOOK ---\n";

    if (!sellOrders.empty()) {
        // sellOrders.begin() is the lowest price
        auto it = sellOrders.begin();
        std::cout << "Top Sell (Ask): " << it->second.front().remaining() 
                  << " @ " << it->first << std::endl;
    } else {
        std::cout << "Top Sell (Ask): <empty>\n";
    }

    if (!buyOrders.empty()) {
        // buyOrders.begin() is the highest price
        auto it = buyOrders.begin();
        std::cout << "Top Buy  (Bid):  " << it->second.front().remaining() 
                  << " @ " << it->first << std::endl;
    } else {
         std::cout << "Top Buy  (Bid):  <empty>\n";
    }

    std::cout << "------------------\n" << std::endl;
}

// --- getCurrentTimestamp ---
time_t OrderBook::getCurrentTimestamp() const {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

