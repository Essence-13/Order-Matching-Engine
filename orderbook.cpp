#include "OrderBook.h"
#include <iostream>
#include <algorithm> // for std::max

OrderBook::OrderBook(std::shared_ptr<Logger> logger) : logger(logger) {
    persistence = std::make_unique<PersistenceManager>("buy_orders.csv", "sell_orders.csv", "trades.csv");
    matchingEngine = std::make_unique<MatchingEngine>();

    logger->log("System", "Order book initializing...");
    
    // Load existing orders and reconstruct the state
    persistence->loadOrders(buyOrders, sellOrders);
    
    // Populate the allOrders map from the loaded books
    for(const auto& pair : buyOrders) {
        auto q = pair.second;
        while(!q.empty()) {
            Order o = q.front();
            allOrders[o.id] = o;
            nextOrderId = std::max(nextOrderId, o.id + 1);
            q.pop();
        }
    }
    for(const auto& pair : sellOrders) {
        auto q = pair.second;
        while(!q.empty()) {
            Order o = q.front();
            allOrders[o.id] = o;
            nextOrderId = std::max(nextOrderId, o.id + 1);
            q.pop();
        }
    }
    
    logger->log("System", "Order book initialized successfully.");
}

OrderBook::~OrderBook() {
    logger->log("System", "Order book shutting down. Exporting active orders...");
    persistence->exportActiveOrders(buyOrders, sellOrders);
    logger->log("System", "Export complete.");
}

void OrderBook::placeOrder(OrderType type, int price, int quantity) {
    if (price <= 0 || quantity <= 0) {
        logger->log("Error", "Invalid order parameters: price and quantity must be positive.");
        throw std::invalid_argument("Price and quantity must be positive");
    }

    Order order{
        nextOrderId++,
        type,
        price,
        quantity,
        0, // filled_quantity
        getCurrentTimestamp()
    };

    allOrders[order.id] = order;
    
    logger->log("Order", "Placing " + typeToStr(type) + " order ID " + 
                std::to_string(order.id) + " for " + std::to_string(quantity) + 
                " @ " + std::to_string(price));

    std::vector<Trade> trades;
    if (type == OrderType::BUY) {
        trades = matchingEngine->matchBuyOrder(allOrders.at(order.id), sellOrders, nextTradeId);
    } else {
        trades = matchingEngine->matchSellOrder(allOrders.at(order.id), buyOrders, nextTradeId);
    }

    processTrades(trades);

    // If the order is not fully filled, add it to the book.
    if (!allOrders.at(order.id).is_filled()) {
        if (order.type == OrderType::BUY) {
            buyOrders[order.price].push(allOrders.at(order.id));
        } else {
            sellOrders[order.price].push(allOrders.at(order.id));
        }
    }
    
    // Persist changes after the operation
    persistence->exportActiveOrders(buyOrders, sellOrders);
}

void OrderBook::processTrades(const std::vector<Trade>& trades) {
    if(trades.empty()) return;

    for (const auto& trade : trades) {
        logger->log("Trade", "Matched " + std::to_string(trade.quantity) + 
                   " units at price " + std::to_string(trade.price) + 
                   " (Buy:" + std::to_string(trade.buyOrderId) + 
                   " Sell:" + std::to_string(trade.sellOrderId) + ")");
        
        std::cout << "TRADE: " << trade.quantity << " @ " << trade.price << std::endl;

        persistence->logTrade(trade);

        // Update the master list of orders
        // allOrders.at(trade.buyOrderId).filled_quantity += trade.quantity;
        // allOrders.at(trade.sellOrderId).filled_quantity += trade.quantity;
    }

    // Clean up filled orders from the master list
    for (const auto& trade : trades) {
        if (allOrders.at(trade.buyOrderId).is_filled()) {
            logger->log("Order", "Buy order " + std::to_string(trade.buyOrderId) + " is fully FILLED.");
        }
        if (allOrders.at(trade.sellOrderId).is_filled()) {
            logger->log("Order", "Sell order " + std::to_string(trade.sellOrderId) + " is fully FILLED.");
        }
    }
}


void OrderBook::cancelOrder(int id) {
    if (allOrders.find(id) == allOrders.end()) {
        logger->log("Error", "Cancel failed - order ID " + std::to_string(id) + " not found");
        throw std::runtime_error("Order ID not found");
    }

    Order& order_to_cancel = allOrders.at(id);
    
    if (order_to_cancel.is_filled()) {
        logger->log("Error", "Cannot cancel already filled order ID " + std::to_string(id));
        throw std::runtime_error("Cannot cancel a filled order.");
    }

    bool removed = false;
    auto type = order_to_cancel.type;
    auto price = order_to_cancel.price;

    auto removeFromQueue = [&](std::queue<Order>& q) {
        std::queue<Order> newQ;
        while (!q.empty()) {
            Order o = q.front();
            q.pop();
            if (o.id != id) {
                newQ.push(o);
            } else {
                removed = true;
            }
        }
        q = newQ;
    };

    if (type == OrderType::BUY) {
        if(buyOrders.count(price)) {
            removeFromQueue(buyOrders[price]);
            if (buyOrders[price].empty()) buyOrders.erase(price);
        }
    } else { // SELL
        if(sellOrders.count(price)) {
            removeFromQueue(sellOrders[price]);
            if (sellOrders[price].empty()) sellOrders.erase(price);
        }
    }

    if (removed) {
        allOrders.erase(id);
        logger->log("Order", "Cancelled order ID " + std::to_string(id));
        persistence->exportActiveOrders(buyOrders, sellOrders);
    } else {
        logger->log("Error", "Order ID " + std::to_string(id) + " not found in active book (might be filled).");
        throw std::runtime_error("Order ID not found in active order book.");
    }
}


void OrderBook::showBook() const {
    std::cout << "\n--- ORDER BOOK ---\n";

    if (!sellOrders.empty()) {
        auto it = sellOrders.rbegin();
        std::cout << "Top Sell: " << it->second.front().remaining() << " @ " << it->first << std::endl;
    } else {
        std::cout << "Top Sell: <empty>\n";
    }

    if (!buyOrders.empty()) {
        auto it = buyOrders.begin();
        std::cout << "Top Buy:  " << it->second.front().remaining() << " @ " << it->first << std::endl;
    } else {
         std::cout << "Top Buy:  <empty>\n";
    }

    std::cout << "------------------\n" << std::endl;
}


time_t OrderBook::getCurrentTimestamp() const {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}
