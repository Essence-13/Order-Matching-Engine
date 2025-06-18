#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using namespace std;

// Random order generator
string randomOrderType() {
    return (rand() % 2 == 0) ? "buy" : "sell";
}

int randomPrice() {
    return 750 + rand() % 551;  // Prices between 950 and 1049
}

int randomQuantity() {
    return 10 + rand() % 91;    // Quantities between 10 and 100
}

int main() {
    srand(time(0));

    int numOrders;
    cout << "Enter number of random orders to generate: ";
    cin >> numOrders;

    ofstream out("input_orders.txt");
    if (!out.is_open()) {
        cerr << "Failed to open output file.\n";
        return 1;
    }

    out << "book\n"; // Optional: show book initially

    for (int i = 0; i < numOrders; ++i) {
        string type = randomOrderType();
        int price = randomPrice();
        int qty = randomQuantity();
        out << type << " "  << price << " " << qty << "\n";
    }

    out << "book\n";   // Optional: show book again
    out << "log\n";    // View trades
    out << "exit\n";   // Exit the engine cleanly
    out.close();

    cout << "✅ Generated " << numOrders << " orders in input_orders.txt\n";
    cout << "📦 To run with your engine:\n";
    cout << "./engine < input_orders.txt\n";

    return 0;
}
