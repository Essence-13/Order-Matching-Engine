#include "Persistence.h"
#include "OrderBookTypes.h" // Include the new types file
#include <fstream>
#include <sstream>
#include <iostream>

PersistenceManager::PersistenceManager(const std::string& buy_file, const std::string& sell_file, const std::string& trades_file)
    : buy_orders_file(buy_file), sell_orders_file(sell_file), trades_file(trades_file) {
    
    // Open the trades log in append mode
    trades_log_stream.open(trades_file, std::ios::app);
    if (!trades_log_stream.is_open()) {
        throw std::runtime_error("Failed to open trades file: " + trades_file);
    }

    // Write header only if the file is new/empty
    if (trades_log_stream.tellp() == 0) {
        trades_log_stream << "TradeID,BuyOrderID,SellOrderID,Price,Quantity,Timestamp\n";
        trades_log_stream.flush();
    }
}

// Destructor to ensure the trades file is closed
PersistenceManager::~PersistenceManager() {
    if (trades_log_stream.is_open()) {
        trades_log_stream.close();
    }
}

void PersistenceManager::loadOrders(BuyBookType& buyOrders, SellBookType& sellOrders) {
    // Pass the maps to the templated helper function
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
        trades_log_stream.flush(); // Ensure trades are written immediately
    }
}

// Template implementation for loading orders
// This now works for both buy and sell maps (which use std::list)
template<typename TMap>
void PersistenceManager::loadOrderType(const std::string& filename, OrderType type, TMap& orders) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        // This is not a fatal error, just a warning. The book will start empty.
        std::cerr << "Warning: Could not open " << filename << " for loading. Starting with an empty book for this type." << std::endl;
        return;
    }

    std::string line;
    getline(in, line); // Skip header

    int count = 0;
    while (getline(in, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::stringstream ss(line);
        std::string token;
        Order o; // This uses the default initializers from Order.h

        try {
            // Parse CSV line
            getline(ss, token, ','); o.id = std::stoi(token);
            getline(ss, token, ','); o.price = std::stoi(token);
            getline(ss, token, ','); o.quantity = std::stoi(token);
            getline(ss, token, ','); o.filled_quantity = std::stoi(token);
            getline(ss, token, ','); o.timestamp = std::stol(token);
            o.type = type;

            // Use push_back for std::list
            if (!o.is_filled()) { // Only load orders that are still active
                orders[o.price].push_back(o);
                count++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line in " << filename << ": " << line << " | Error: " << e.what() << std::endl;
            // Continue to the next line
        }
    }
    std::cout << "Loaded " << count << " active orders from " << filename << std::endl;
}

// Explicit template instantiations are required when the definition is in a .cpp file
template void PersistenceManager::loadOrderType<BuyBookType>(const std::string&, OrderType, BuyBookType&);
template void PersistenceManager::loadOrderType<SellBookType>(const std::string&, OrderType, SellBookType&);


void PersistenceManager::exportActiveOrders(const BuyBookType& buyOrders, const SellBookType& sellOrders) {
    // Use a temporary stream to avoid corrupting the file on error
    std::ofstream buyOut(buy_orders_file, std::ios::trunc); // trunc = overwrite
    if (!buyOut.is_open()) {
        std::cerr << "Error: Could not open " << buy_orders_file << " for export." << std::endl;
        return;
    }
    
    buyOut << "OrderID,Price,Quantity,FilledQuantity,Timestamp\n";
    // Iterate through the map (price levels)
    for (const auto& pair : buyOrders) {
        // Iterate through the list (orders at that price)
        for (const auto& o : pair.second) {
            buyOut << o.id << "," << o.price << "," << o.quantity << ","
                   << o.filled_quantity << "," << o.timestamp << "\n";
        }
    }
    buyOut.close();


    std::ofstream sellOut(sell_orders_file, std::ios::trunc);
    if (!sellOut.is_open()) {
        std::cerr << "Error: Could not open " << sell_orders_file << " for export." << std::endl;
        return;
    }

    sellOut << "OrderID,Price,Quantity,FilledQuantity,Timestamp\n";
    for (const auto& pair : sellOrders) {
        for (const auto& o : pair.second) {
            sellOut << o.id << "," << o.price << "," << o.quantity << ","
                    << o.filled_quantity << "," << o.timestamp << "\n";
        }
    }
    sellOut.close();
}

