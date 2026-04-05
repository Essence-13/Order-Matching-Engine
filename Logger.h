#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Raw trade data — no strings, no heap allocation
struct TradeEntry {
    int qty;
    int price;
    int buyId;
    int sellId;
    int tradeId;
};

struct OrderEntry {
    int  id;
    int  price;
    int  qty;
    bool isBuy;
    bool isFilled;  // true = "fully filled", false = "placing"
};

// General log entry for non-hot-path messages
struct LogEntry {
    std::string category;
    std::string message;
};

class Logger {
public:
    Logger(const std::string& log_file, const std::string& trades_file = "");
    ~Logger();

    // General logging — called for system/order/error events (not hot path)
    void log(const std::string& category, const std::string& message);

    // Trade logging — called on the hot path, zero heap allocation on caller thread
    void logTrade(int tradeId, int buyId, int sellId, int price, int qty);

    // In class body:
    void logOrder(int id, int price, int qty, bool isBuy, bool isFilled);

    // Queue:
    std::queue<OrderEntry> order_queue_;

    // Update queueDepth():
    size_t queueDepth() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return log_queue_.size() + trade_queue_.size() + order_queue_.size();
    }


private:
    std::ofstream eventLog_;
    std::ofstream tradesLog_;
    bool persist_trades_{false};

    std::queue<LogEntry>   log_queue_;
    std::queue<TradeEntry> trade_queue_;

    mutable std::mutex      mutex_;
    std::condition_variable cv_;
    std::atomic<bool>       running_{true};

    std::thread   worker_;

    void processQueue();
};

// Macros:
#ifdef BENCHMARK_MODE
    #define LOG(logger, cat, msg)
    #define LOG_TRADE(logger, tid, bid, sid, price, qty)
    #define LOG_ORDER(logger, id, price, qty, isBuy, isFilled)
#else
    #define LOG(logger, cat, msg) \
            (logger)->log(cat, msg)
    #define LOG_TRADE(logger, tid, bid, sid, price, qty) \
            (logger)->logTrade(tid, bid, sid, price, qty)
    #define LOG_ORDER(logger, id, price, qty, isBuy, isFilled) \
            (logger)->logOrder(id, price, qty, isBuy, isFilled)
#endif

#endif // LOGGER_H