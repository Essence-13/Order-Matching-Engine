#include "OrderBook.h"
#include "Logger.h"
#include <iostream>
#include <string>
#include <limits>
#include <memory>
#include <chrono>   // For benchmark timer
#include <random>   // For benchmark data
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>

/**
 * @brief Runs the interactive console UI for manual order entry.
 * @param ob The OrderBook instance.
 */
void run_console_ui(OrderBook& ob) {
    std::cout << "Order Matching Engine (Enter 'help' for commands, 'exit' to quit)\n";
    std::string cmd;

    while (true) {
        std::cout << "\n> ";
        std::cin >> cmd;

        if (cmd == "exit") {
            break;
        } else if (cmd == "buy" || cmd == "sell") {
            try {
                int price, quantity;
                std::cout << "Enter price and quantity: ";
                std::cin >> price >> quantity;

                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    throw std::invalid_argument("Invalid input. Please enter numbers.");
                }

                ob.placeOrder(cmd == "buy" ? OrderType::BUY : OrderType::SELL, price, quantity);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        } else if (cmd == "cancel") {
            try {
                int id;
                std::cout << "Enter Order ID to cancel: ";
                std::cin >> id;
                 if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    throw std::invalid_argument("Invalid input. Please enter a number.");
                }
                ob.cancelOrder(id);
                std::cout << "Order " << id << " cancellation request processed.\n";
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        } else if (cmd == "book") {
            ob.showBook();
        } else if (cmd == "help") {
             std::cout << "\nAvailable Commands:\n"
                  << "  buy      - Place a new buy order.\n"
                  << "  sell     - Place a new sell order.\n"
                  << "  cancel   - Cancel an existing order by ID.\n"
                  << "  book     - Show the top of the order book.\n"
                  << "  exit     - Save state and exit the application.\n\n";
        } else {
            std::cout << "Unknown command. Type 'help' for a list of commands.\n";
        }
    }
}




void run_engine_benchmark(OrderBook& ob, long long num_orders) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> price_dist(95, 105);
    std::uniform_int_distribution<int> quant_dist(1, 100);
    std::uniform_int_distribution<int> type_dist(0, 1);

    // Pre-generate orders so RNG cost is excluded from timing
    struct RawOrder { OrderType type; int price; int qty; };
    std::vector<RawOrder> orders;
    orders.reserve(num_orders);
    for (long long i = 0; i < num_orders; ++i)
        orders.push_back({type_dist(rng)==0 ? OrderType::BUY : OrderType::SELL,
                          price_dist(rng), quant_dist(rng)});

    std::cout << "\n=== ENGINE BENCHMARK (" << num_orders << " orders) ===\n";

    // -- THROUGHPUT --
    auto t0 = std::chrono::high_resolution_clock::now();
    for (const auto& o : orders)
        ob.placeOrder(o.type, o.price, o.qty);
    auto t1 = std::chrono::high_resolution_clock::now();

    double secs = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Throughput : " << std::fixed << std::setprecision(0)
              << (num_orders / secs) << " OPS  (" << secs << "s)\n";

    // -- LATENCY (10k individual samples) --
    std::vector<long long> lat;
    lat.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        OrderType type = type_dist(rng)==0 ? OrderType::BUY : OrderType::SELL;
        auto l0 = std::chrono::high_resolution_clock::now();
        ob.placeOrder(type, price_dist(rng), quant_dist(rng));
        auto l1 = std::chrono::high_resolution_clock::now();
        lat.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(l1-l0).count());
    }
    std::sort(lat.begin(), lat.end());
    std::cout << "Latency    : p50=" << lat[5000] << "ns"
              << "  p99="  << lat[9900] << "ns"
              << "  p999=" << lat[9990] << "ns"
              << "  max="  << lat.back() << "ns\n";
}

void run_logger_benchmark(Logger& logger, long long num_messages) {
    std::cout << "\n=== LOGGER BENCHMARK (" << num_messages << " messages) ===\n";

    // -- PUSH THROUGHPUT --
    auto t0 = std::chrono::high_resolution_clock::now();
    for (long long i = 0; i < num_messages; ++i)
        logger.log("Bench", "msg " + std::to_string(i));
    auto t1 = std::chrono::high_resolution_clock::now();

    double secs = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Push throughput : " << std::fixed << std::setprecision(0)
              << (num_messages / secs) << " msg/s\n";

    // -- PUSH LATENCY (10k samples) --
    std::vector<long long> lat;
    lat.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        auto l0 = std::chrono::high_resolution_clock::now();
        logger.log("Bench", "latency sample");
        auto l1 = std::chrono::high_resolution_clock::now();
        lat.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(l1-l0).count());
    }
    std::sort(lat.begin(), lat.end());
    std::cout << "Push latency    : p50=" << lat[5000] << "ns"
              << "  p99="  << lat[9900] << "ns"
              << "  p999=" << lat[9990] << "ns"
              << "  max="  << lat.back() << "ns\n";

    // -- QUEUE DEPTH --
    for (int i = 0; i < 1000; ++i)
        logger.log("Bench", "burst " + std::to_string(i));
    std::cout << "Queue depth after 1k burst : "
              << logger.queueDepth() << " msgs pending\n";
}
/**
 * @brief Main application entry point.
 */
int main(int argc, char* argv[]) {
    try {
        // A shared pointer allows multiple objects to share ownership of the logger.
        auto logger = std::make_shared<Logger>("events.log", "trades.csv");
        
        // The main application logic is now encapsulated in the OrderBook class.
        OrderBook ob(logger);
        
        // Check if we are in benchmark mode
        if (argc == 2 && std::string(argv[1]) == "--benchmark") {
            
            long long num_orders = 0;
            std::cout << "Enter Numer of Orders to Benchmark: ";
            std::cin >> num_orders;

            // --- BENCHMARK MODE ---
            // Run with 2 million orders
            // Assumes std::cout in processTrades is commented out!
                run_engine_benchmark(ob, num_orders);
                run_logger_benchmark(*logger, num_orders/2);

        } else {
            // --- INTERACTIVE MODE ---
            run_console_ui(ob);
        }

    } catch (const std::exception& e) {
        std::cerr << "A fatal error occurred: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Exiting gracefully." << std::endl;
    return 0;
}

