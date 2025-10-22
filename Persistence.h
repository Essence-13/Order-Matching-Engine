#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "Order.h"
#include <map>
#include <queue>
#include <vector>
#include <string>
#include<fstream>

// Manages loading and saving data to/from CSV files.
class PersistenceManager {
public:
    PersistenceManager(const std::string& buy_file, const std::string& sell_file, const std::string& trades_file);

    // Loads orders from a file into a map data structure.
    void loadOrders(std::map<int, std::queue<Order>, std::greater<int>>& buyOrders,
                    std::map<int, std::queue<Order>>& sellOrders);

    // Exports the current state of active orders to their respective files.
    void exportActiveOrders(const std::map<int, std::queue<Order>, std::greater<int>>& buyOrders,
                            const std::map<int, std::queue<Order>>& sellOrders);
    
    // Appends a completed trade to the trades log file.
    void logTrade(const Trade& trade);

private:
    std::string buy_orders_file;
    std::string sell_orders_file;
    std::string trades_file;
    std::ofstream trades_log_stream;

    // Helper to load orders of a specific type
    template<typename TMap>
       void loadOrderType(const std::string& filename, OrderType type, TMap& orders);
};

#endif // PERSISTENCE_H
