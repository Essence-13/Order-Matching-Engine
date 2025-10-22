#include "OrderBook.h"
#include "Logger.h"
#include <iostream>
#include <string>
#include <limits>
#include <memory>

// The UI is now handled in a separate function.
void run_console_ui(OrderBook& ob) {
    std::cout << "Order Matching Engine (Enter 'help' for commands, 'exit' to quit)\n";
    std::string cmd;

    while (true) {
        std::cout << "> ";
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


int main() {
    try {
        // A shared pointer allows multiple objects to share ownership of the logger.
        auto logger = std::make_shared<Logger>("events.log");
        
        // The main application logic is now encapsulated in the OrderBook class.
        OrderBook ob(logger);
        
        // The user interface is cleanly separated from the core logic.
        run_console_ui(ob);

    } catch (const std::exception& e) {
        std::cerr << "A fatal error occurred: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Exiting gracefully." << std::endl;
    return 0;
}
