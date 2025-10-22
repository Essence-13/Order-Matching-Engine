#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <chrono>

// Enums define the possible states and types for orders.
enum class OrderType { BUY, SELL };
enum class OrderStatus { OPEN, PARTIAL, FILLED, CANCELLED };

// Utility functions to convert enums to human-readable strings.
inline std::string statusToStr(OrderStatus status) {
    switch (status) {
        case OrderStatus::OPEN: return "OPEN";
        case OrderStatus::PARTIAL: return "PARTIAL";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

inline std::string typeToStr(OrderType type) {
    return type == OrderType::BUY ? "BUY" : "SELL";
}

// Represents a single order in the book.
struct Order {
    int id;
    OrderType type;
    int price;
    int quantity;
    int filled_quantity = 0;
    time_t timestamp;

    // Calculates the remaining quantity to be filled.
    int remaining() const { return quantity - filled_quantity; }

    // Checks if the order has been completely filled.
    bool is_filled() const { return remaining() == 0; }
};

// Represents a completed trade between a buy and a sell order.
struct Trade {
    int tradeId;
    int buyOrderId;
    int sellOrderId;
    int price;
    int quantity;
    time_t timestamp;
};

#endif // ORDER_H
