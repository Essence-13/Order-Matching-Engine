#include "Logger.h"
#include <stdexcept>
#include <sstream>

Logger::Logger(const std::string& log_file, const std::string& trades_file) {
    eventLog_.open(log_file, std::ios::app);
    if (!eventLog_.is_open())
        throw std::runtime_error("Failed to open log file: " + log_file);

    if (!trades_file.empty()) {
    tradesLog_.open(trades_file, std::ios::app);
    if (!tradesLog_.is_open())
        throw std::runtime_error("Failed to open trades file: " + trades_file);

    // Write header if new file
    bool isNew = false;
    { std::ifstream check(trades_file); isNew = check.peek() == EOF; }
    if (isNew)
        tradesLog_ << "TradeID,BuyOrderID,SellOrderID,Price,Quantity\n";

    persist_trades_ = true;
}

    worker_ = std::thread(&Logger::processQueue, this);
    log("System", "Logger initialized.");
}

Logger::~Logger() {
    log("System", "Logger shutting down.");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_one();
    worker_.join();
    if (eventLog_.is_open())
        eventLog_.close();
}

// Called on matching thread — just pushes string, no formatting
void Logger::log(const std::string& category, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        log_queue_.push({category, message});
    }
    cv_.notify_one();
}

// Called on matching thread — pushes raw ints, zero heap allocation
void Logger::logTrade(int tradeId, int buyId, int sellId, int price, int qty) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        trade_queue_.push({qty, price, buyId, sellId, tradeId});
    }
    cv_.notify_one();
}

void Logger::logOrder(int id, int price, int qty, bool isBuy, bool isFilled) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        order_queue_.push({id, price, qty, isBuy, isFilled});
    }
    cv_.notify_one();
}

// Runs on logger thread — all formatting happens here, off the hot path
void Logger::processQueue() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return !log_queue_.empty() || !trade_queue_.empty() || !running_;
        });

        // Drain general log queue
        while (!log_queue_.empty()) {
            const auto& entry = log_queue_.front();

            // Timestamp formatting happens here — not on matching thread
            time_t now = std::chrono::system_clock::to_time_t(
                             std::chrono::system_clock::now());
            std::tm tm{};
            localtime_r(&now, &tm);
            char timebuf[32];
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);

            eventLog_ << timebuf << " ["
                      << entry.category << "] "
                      << entry.message  << "\n";
            log_queue_.pop();
        }

        // Drain trade queue — all string building happens here
        while (!trade_queue_.empty()) {
            const auto& t = trade_queue_.front();

            eventLog_ << "[Trade] id="     << t.tradeId
                      << " buy="           << t.buyId
                      << " sell="          << t.sellId
                      << " price="         << t.price
                      << " qty="           << t.qty  << "\n";

                      if (persist_trades_ && tradesLog_.is_open()) {
        tradesLog_ << t.tradeId  << ","
                   << t.buyId    << ","
                   << t.sellId   << ","
                   << t.price    << ","
                   << t.qty      << "\n";
    }

            trade_queue_.pop();
        }

        

    while (!order_queue_.empty()) {
    const auto& o = order_queue_.front();
    if (o.isFilled) {
        eventLog_ << "[Order] id=" << o.id << " fully filled immediately\n";
    } else {
        eventLog_ << "[Order] placing "
                  << (o.isBuy ? "BUY" : "SELL")
                  << " id="    << o.id
                  << " qty="   << o.qty
                  << " price=" << o.price << "\n";
    }
    order_queue_.pop();
}


        eventLog_.flush();
if (persist_trades_) tradesLog_.flush();
        if (!running_ && log_queue_.empty() && trade_queue_.empty())
            break;
    }
}