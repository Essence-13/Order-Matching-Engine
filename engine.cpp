// Enhanced Order Matching Engine with Status Tracking (C++)
#include <iostream>
#include <map>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <memory>
#include <limits>
#include <iomanip>
#include <chrono>
using namespace std;

enum class OrderType { BUY, SELL };
enum class OrderStatus { OPEN, PARTIAL, FILLED, CANCELLED };

// Utility functions
string statusToStr(OrderStatus status) {
    switch (status) {
        case OrderStatus::OPEN: return "OPEN";
        case OrderStatus::PARTIAL: return "PARTIAL";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

string typeToStr(OrderType type) {
    return type == OrderType::BUY ? "BUY" : "SELL";
}

struct Order {
    int id;
    OrderType type;
    int price;
    int quantity;
    int filled_quantity = 0;
    time_t timestamp;
    
    int remaining() const { return quantity - filled_quantity; }
    bool is_filled() const { return remaining() == 0; }
};

class OrderBook {
private:
    int orderId = 0;
    time_t time = 0;
    int tradeId = 1;
    
    // Order books - price => queue of orders
    map<int, queue<Order>, greater<int>> buyOrders;  // descending for buys
    map<int, queue<Order>> sellOrders;              // ascending for sells
    
    // Tracking structures
    unordered_map<int, pair<int, OrderType>> idToPriceAndType;
    unordered_map<int, OrderStatus> orderStatus;
    unordered_map<int, queue<Order>*> idToOrderQueue;  // For O(1) cancellation
    
    // File handles
    ofstream logFile;
    ofstream eventLog;
    
    // Constants
    const string BUY_ORDERS_FILE = "buy_orders.csv";
    const string SELL_ORDERS_FILE = "sell_orders.csv";
    const string TRADES_FILE = "trades.csv";
    const string STATUS_FILE = "order_status.csv";
    const string EVENT_LOG_FILE = "events.log";

public:
    OrderBook() {
        // Initialize with proper error handling
        try {
            loadOrders(BUY_ORDERS_FILE, OrderType::BUY);
            loadOrders(SELL_ORDERS_FILE, OrderType::SELL);
            
            // Open log files with proper headers if empty
            logFile.open(TRADES_FILE, ios::app);
            if (logFile.tellp() == 0) {
                logFile << "TradeID,BuyOrderID,SellOrderID,Price,Quantity,Timestamp\n";
            }
            
            eventLog.open(EVENT_LOG_FILE, ios::app);
            logEvent("System", "Order book initialized");
        } catch (const exception& e) {
            cerr << "Initialization error: " << e.what() << endl;
            throw;
        }
    }

    ~OrderBook() {
        logFile.close();
        eventLog.close();
        exportActiveOrders();
        exportOrderStatus();
        logEvent("System", "Order book shutdown");
    }

    void placeOrder(OrderType type, int price, int quantity) {
        if (price <= 0 || quantity <= 0) {
            logEvent("Error", "Invalid order parameters");
            throw invalid_argument("Price and quantity must be positive");
        }

        Order order{
            ++orderId,
            type,
            price,
            quantity,
            0,  // filled_quantity
            getCurrentTimestamp()
        };

        idToPriceAndType[order.id] = {price, type};
        orderStatus[order.id] = OrderStatus::OPEN;
        
        logEvent("Order", "Placing " + typeToStr(type) + " order ID " + 
                to_string(order.id) + " for " + to_string(quantity) + 
                " @ " + to_string(price));

        try {
            if (type == OrderType::BUY) {
                matchBuy(order);
            } else {
                matchSell(order);
            }
        } catch (...) {
            orderStatus[order.id] = OrderStatus::CANCELLED;
            throw;
        }

        exportActiveOrders();
        exportOrderStatus();
    }

    void cancelOrder(int id) {
        if (idToPriceAndType.count(id) == 0) {
            logEvent("Error", "Cancel failed - order ID " + to_string(id) + " not found");
            throw runtime_error("Order ID not found");
        }

        auto price = idToPriceAndType[id].first;
        auto type = idToPriceAndType[id].second;
        bool removed = false;

        if (type == OrderType::BUY) {
            auto& q = buyOrders[price];
            removed = removeOrderFromQueue(q, id);
            if (q.empty()) buyOrders.erase(price);
        } else {
            auto& q = sellOrders[price];
            removed = removeOrderFromQueue(q, id);
            if (q.empty()) sellOrders.erase(price);
        }

        if (removed) {
            orderStatus[id] = OrderStatus::CANCELLED;
            idToPriceAndType.erase(id);
            idToOrderQueue.erase(id);
            logEvent("Order", "Cancelled order ID " + to_string(id));
            exportActiveOrders();
            exportOrderStatus();
        } else {
            logEvent("Error", "Order ID " + to_string(id) + " not found in queue");
            throw runtime_error("Order ID not found in queue");
        }
    }

private:
    // Matching engine core functions
    void matchBuy(Order& buy) {
        int originalQty = buy.quantity;
        
        for (auto it = sellOrders.begin(); it != sellOrders.end() && !buy.is_filled();) {
            if (buy.price < it->first) break;
            
            auto& q = it->second;
            while (!q.empty() && !buy.is_filled()) {
                Order& sell = q.front();
                int tradedQty = min(buy.remaining(), sell.remaining());
                
                // Execute trade
                executeTrade(buy, sell, sell.price, tradedQty);
                
                // Update quantities
                buy.filled_quantity += tradedQty;
                sell.filled_quantity += tradedQty;
                
                // Handle sell order status
                if (sell.is_filled()) {
                    orderStatus[sell.id] = OrderStatus::FILLED;
                    q.pop();
                } else {
                    orderStatus[sell.id] = OrderStatus::PARTIAL;
                }
            }
            
            if (q.empty()) {
                it = sellOrders.erase(it);
            } else {
                ++it;
            }
        }
        
        // Handle remaining buy order
        if (!buy.is_filled()) {
            orderStatus[buy.id] = buy.filled_quantity > 0 ? 
                OrderStatus::PARTIAL : OrderStatus::OPEN;
            buyOrders[buy.price].push(buy);
            idToOrderQueue[buy.id] = &buyOrders[buy.price];
        } else {
            orderStatus[buy.id] = OrderStatus::FILLED;
            idToPriceAndType.erase(buy.id);
        }
    }

    void matchSell(Order& sell) {
        int originalQty = sell.quantity;
        
        for (auto it = buyOrders.begin(); it != buyOrders.end() && !sell.is_filled();) {
            if (sell.price > it->first) break;
            
            auto& q = it->second;
            while (!q.empty() && !sell.is_filled()) {
                Order& buy = q.front();
                int tradedQty = min(sell.remaining(), buy.remaining());
                
                // Execute trade
                executeTrade(buy, sell, buy.price, tradedQty);
                
                // Update quantities
                sell.filled_quantity += tradedQty;
                buy.filled_quantity += tradedQty;
                
                // Handle buy order status
                if (buy.is_filled()) {
                    orderStatus[buy.id] = OrderStatus::FILLED;
                    q.pop();
                } else {
                    orderStatus[buy.id] = OrderStatus::PARTIAL;
                }
            }
            
            if (q.empty()) {
                it = buyOrders.erase(it);
            } else {
                ++it;
            }
        }
        
        // Handle remaining sell order
        if (!sell.is_filled()) {
            orderStatus[sell.id] = sell.filled_quantity > 0 ? 
                OrderStatus::PARTIAL : OrderStatus::OPEN;
            sellOrders[sell.price].push(sell);
            idToOrderQueue[sell.id] = &sellOrders[sell.price];
        } else {
            orderStatus[sell.id] = OrderStatus::FILLED;
            idToPriceAndType.erase(sell.id);
        }
    }

    void executeTrade(Order& buy, Order& sell, int price, int quantity) {
        // Log the trade
        logFile << tradeId++ << ","
                << buy.id << ","
                << sell.id << ","
                << price << ","
                << quantity << ","
                << getCurrentTimestamp() << "\n";
        logFile.flush();
        
        // Log event
        logEvent("Trade", "Matched " + to_string(quantity) + 
                " units at price " + to_string(price) + 
                " (Buy:" + to_string(buy.id) + 
                " Sell:" + to_string(sell.id) + ")");
        
        cout << "Matched " << quantity << " units at price " << price << endl;
    }

    // Utility functions
    bool removeOrderFromQueue(queue<Order>& q, int id) {
        queue<Order> newQ;
        bool found = false;
        
        while (!q.empty()) {
            Order o = q.front();
            q.pop();
            if (o.id != id) {
                newQ.push(o);
            } else {
                found = true;
            }
        }
        
        if (!newQ.empty()) {
            q = move(newQ);
        }
        
        return found;
    }

    time_t getCurrentTimestamp() {
        return chrono::system_clock::to_time_t(chrono::system_clock::now());
    }

    void logEvent(const string& category, const string& message) {
        time_t now = getCurrentTimestamp();
        eventLog << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S") << " ["
                << category << "] " << message << "\n";
        eventLog.flush();
    }

    // Persistence functions
    void exportOrderStatus() {
        ofstream statusOut(STATUS_FILE);
        statusOut << "OrderID,Status,FilledQuantity,TotalQuantity\n";
        
        for (auto it = orderStatus.begin(); it != orderStatus.end();it++) {
            auto id = it->first;
            auto status = it->second;

            if (idToPriceAndType.count(id)) {
                auto price = idToPriceAndType[id].first;
                auto type = idToPriceAndType[id].second;
                int filled = 0;
                int total = 0;
                
                // Find the order to get filled quantity
                if (type == OrderType::BUY && buyOrders.count(price)) {
                    auto q = buyOrders[price];
                    while (!q.empty()) {
                        auto o = q.front();
                        if (o.id == id) {
                            filled = o.filled_quantity;
                            total = o.quantity;
                            break;
                        }
                        q.pop();
                    }
                } else if (sellOrders.count(price)) {
                    auto q = sellOrders[price];
                    while (!q.empty()) {
                        auto o = q.front();
                        if (o.id == id) {
                            filled = o.filled_quantity;
                            total = o.quantity;
                            break;
                        }
                        q.pop();
                    }
                }
                
                statusOut << id << "," << statusToStr(status) << "," 
                         << filled << "," << total << "\n";
            }
        }
    }

    void exportActiveOrders() {
        ofstream buyOut(BUY_ORDERS_FILE);
        ofstream sellOut(SELL_ORDERS_FILE);
        
        buyOut << "OrderID,Price,Quantity,FilledQuantity,Timestamp\n";
        sellOut << "OrderID,Price,Quantity,FilledQuantity,Timestamp\n";

        for (auto it = buyOrders.begin(); it != buyOrders.end();it++) {
            auto price = it->first;
            auto q = it->second;
            auto tempQ = q;
            while (!tempQ.empty()) {
                const auto& o = tempQ.front();
                buyOut << o.id << "," << o.price << "," << o.quantity << ","
                       << o.filled_quantity << "," << o.timestamp << "\n";
                tempQ.pop();
            }
        }

        for (auto it = sellOrders.begin(); it != sellOrders.end();it++) {
            auto price = it->first;
            auto q = it->second;
            auto tempQ = q;
            while (!tempQ.empty()) {
                const auto& o = tempQ.front();
                sellOut << o.id << "," << o.price << "," << o.quantity << ","
                        << o.filled_quantity << "," << o.timestamp << "\n";
                tempQ.pop();
            }
        }
    }

    void loadOrders(const string& filename, OrderType type) {
        ifstream in(filename);
        if (!in.is_open()) {
            logEvent("Warning", "Could not open " + filename + " for loading");
            return;
        }

        string line;
        getline(in, line); // skip header
        
        while (getline(in, line)) {
            try {
                stringstream ss(line);
                string token;
                Order o;
                
                getline(ss, token, ','); o.id = stoi(token);
                getline(ss, token, ','); o.price = stoi(token);
                getline(ss, token, ','); o.quantity = stoi(token);
                getline(ss, token, ','); o.filled_quantity = stoi(token);
                getline(ss, token, ','); o.timestamp = stol(token);
                o.type = type;
                
                if (type == OrderType::BUY) {
                    buyOrders[o.price].push(o);
                } else {
                    sellOrders[o.price].push(o);
                }
                
                idToPriceAndType[o.id] = {o.price, type};
                orderStatus[o.id] = o.is_filled() ? OrderStatus::FILLED : 
                    (o.filled_quantity > 0 ? OrderStatus::PARTIAL : OrderStatus::OPEN);
                
                orderId = max(orderId, o.id);
                time = max(time, o.timestamp);
                
                // Track for O(1) cancellation
                idToOrderQueue[o.id] = type == OrderType::BUY ? 
                    &buyOrders[o.price] : &sellOrders[o.price];
                    
            } catch (const exception& e) {
                logEvent("Error", "Failed to parse line in " + filename + ": " + line);
                continue;
            }
        }
        
        logEvent("System", "Loaded " + to_string(orderId) + " orders from " + filename);
    }

public:
    // Display functions
    void showBook() const {
        cout << "\nTop of Order Book:\n";
        
        if (!buyOrders.empty()) {
            auto price = buyOrders.begin()->first;
            auto q = buyOrders.begin()->second;
            if (!q.empty()) {
                cout << "Top Buy: " << q.front().remaining() 
                     << " @ " << price << endl;
            }
        }
        
        if (!sellOrders.empty()) {
            auto price = sellOrders.begin()->first;
            auto q = sellOrders.begin()->second;
            if (!q.empty()) {
                cout << "Top Sell: " << q.front().remaining() 
                     << " @ " << price << endl;
            }
        }
        
        cout << endl;
    }

    void showTradeLog() const {
        cout << "\nTrade log saved in '" << TRADES_FILE 
             << "'. Open it with Excel or a text editor to view.\n";
    }
};

int main() {
    try {
        OrderBook ob;
        cout << "Order Matching Engine (Enter 'exit' to quit)\n";

        string cmd;
        while (true) {
            cout << "\nCommand (buy/sell/cancel/book/log/exit): ";
            cin >> cmd;
            
            if (cmd == "exit") break;
            else if (cmd == "buy" || cmd == "sell") {
                try {
                    int price, quantity;
                    cout << "Enter price and quantity: ";
                    cin >> price >> quantity;
                    
                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        throw invalid_argument("Invalid input");
                    }
                    
                    ob.placeOrder(cmd == "buy" ? OrderType::BUY : OrderType::SELL, 
                                price, quantity);
                } catch (const exception& e) {
                    cerr << "Error: " << e.what() << endl;
                }
            } else if (cmd == "cancel") {
                try {
                    int id;
                    cout << "Enter Order ID to cancel: ";
                    cin >> id;
                    ob.cancelOrder(id);
                } catch (const exception& e) {
                    cerr << "Error: " << e.what() << endl;
                }
            } else if (cmd == "book") {
                ob.showBook();
            } else if (cmd == "log") {
                ob.showTradeLog();
            } else {
                cout << "Unknown command.\n";
            }
        }
    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}