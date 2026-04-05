#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "OrderBookTypes.h" // <-- Includes our new types
#include <string>
#include <fstream> // <-- THE FIX IS HERE

// Manages loading and saving data to/from CSV files.
class PersistenceManager {
public:
    PersistenceManager(const std::string& buy_file, const std::string& sell_file, const std::string& trades_file);
    ~PersistenceManager();

    // Loads orders from files into the two book types.
    void loadOrders(BuyBookType& buyOrders, SellBookType& sellOrders);

    // Exports the current state of active orders to their respective files.
    void exportActiveOrders(const BuyBookType& buyOrders, const SellBookType& sellOrders);
    
    // Appends a completed trade to the trades log file.
    void logTrade(const Trade& trade);

private:
    std::string buy_orders_file;
    std::string sell_orders_file;
    std::string trades_file;
    std::ofstream trades_log_stream; // <-- This line now has its definition

    // Template helper to load orders into either a BuyBookType or SellBookType
    template<typename TMap>
    void loadOrderType(const std::string& filename, OrderType type, TMap& orders);
};

#endif // PERSISTENCE_H

