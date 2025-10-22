#include "Persistence.h"
#include <fstream>
#include <sstream>
#include <iostream>

PersistenceManager::PersistenceManager(const std::string& buy_file, const std::string& sell_file, const std::string& trades_file)
    : buy_orders_file(buy_file), sell_orders_file(sell_file), trades_file(trades_file) {
    
    trades_log_stream.open(trades_file, std::ios::app);
    if (trades_log_stream.tellp() == 0) {
        trades_log_stream << "TradeID,BuyOrderID,SellOrderID,Price,Quantity,Timestamp\n";
    }
}

void PersistenceManager::loadOrders(std::map<int, std::queue<Order>, std::greater<int>>& buyOrders, std::map<int, std::queue<Order>>& sellOrders) {
    loadOrderType(buy_orders_file, OrderType::BUY, buyOrders);
    loadOrderType(sell_orders_file, OrderType::SELL, sellOrders);
}


void PersistenceManager::logTrade(const Trade& trade) {
    if (trades_log_stream.is_open()) {
        trades_log_stream << trade.tradeId << ","
                          << trade.buyOrderId << ","
                          << trade.sellOrderId << ","
                          << trade.price << ","
                          << trade.quantity << ","
                          << trade.timestamp << "\n";
        trades_log_stream.flush();
    }
}


template<typename TMap>
void PersistenceManager::loadOrderType(const std::string& filename, OrderType type, TMap& orders) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        // It's okay if files don't exist on first run.
        std::cerr << "Warning: Could not open " << filename << " for loading. Starting fresh." << std::endl;
        return;
    }

    std::string line;
    getline(in, line); // skip header

    while (getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        Order o;
        try {
            getline(ss, token, ','); o.id = stoi(token);
            getline(ss, token, ','); o.price = stoi(token);
            getline(ss, token, ','); o.quantity = stoi(token);
            getline(ss, token, ','); o.filled_quantity = stoi(token);
            getline(ss, token, ','); o.timestamp = stol(token);
            o.type = type;

            orders[o.price].push(o);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line in " << filename << ": " << line << " - " << e.what() << std::endl;
        }
    }
}

// Explicit instantiations for the map types we use
template void PersistenceManager::loadOrderType<std::map<int, std::queue<Order>, std::greater<int>>>(const std::string&, OrderType, std::map<int, std::queue<Order>, std::greater<int>>&);
template void PersistenceManager::loadOrderType<std::map<int, std::queue<Order>>>(const std::string&, OrderType, std::map<int, std::queue<Order>>&);


void PersistenceManager::exportActiveOrders(const std::map<int, std::queue<Order>, std::greater<int>>& buyOrders, const std::map<int, std::queue<Order>>& sellOrders) {
    std::ofstream buyOut(buy_orders_file);
    buyOut << "OrderID,Price,Quantity,FilledQuantity,Timestamp\n";
    for (const auto& pair : buyOrders) {
        auto q = pair.second; // Make a copy to iterate
        while (!q.empty()) {
            const auto& o = q.front();
            buyOut << o.id << "," << o.price << "," << o.quantity << ","
                   << o.filled_quantity << "," << o.timestamp << "\n";
            q.pop();
        }
    }

    std::ofstream sellOut(sell_orders_file);
    sellOut << "OrderID,Price,Quantity,FilledQuantity,Timestamp\n";
    for (const auto& pair : sellOrders) {
        auto q = pair.second; // Make a copy to iterate
        while (!q.empty()) {
            const auto& o = q.front();
            sellOut << o.id << "," << o.price << "," << o.quantity << ","
                    << o.filled_quantity << "," << o.timestamp << "\n";
            q.pop();
        }
    }
}
