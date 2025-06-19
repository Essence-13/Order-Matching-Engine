# Order Matching Engine Simulator

A high-performance C++ based order matching engine that simulates a stock exchange environment. It supports live order book matching, order status tracking, persistent trade logs, and an interactive real-time dashboard built with Streamlit.

## Features

- C++ core matching engine with:
  - Price-time priority
  - Support for partial fills and cancellations
  - Order lifecycle tracking (OPEN, PARTIAL, FILLED, CANCELLED)

- Real-time dashboard (Python/Streamlit) for:
  - Market depth charts (Buy/Sell)
  - Price movement visualization
  - Live trade ticker
  - Order status distribution
  - Grouped trade summary

- Persistent state using CSV files:
  - Active orders, trades, and order statuses are logged to disk and restored on restart

- Random order simulator to generate realistic test data for benchmarking and stress testing

## Project Structure

```
order-matching-engine/
├── engine.cpp                    # C++ core matching engine
├── random_order_generator.cpp    # C++ utility for generating random test orders
├── dashboard.py                  # Real-time visual dashboard (Streamlit)
├── buy_orders.csv                # Active buy orders (persistent)
├── sell_orders.csv               # Active sell orders (persistent)
├── trades.csv                    # Trade execution log
├── order_status.csv              # Tracks the status of all orders
└── README.md                     # Project documentation
```

## Getting Started

### 1. Build and Run the Matching Engine

```bash
g++ -std=c++17 engine.cpp -o engine
./engine
```

Interactive commands:
- `buy` / `sell` – Place an order
- `cancel` – Cancel an order by ID
- `book` – Display top of order book
- `log` – View recent trade summary
- `exit` – Exit and save state

### 2. Generate Random Orders (Optional)

```bash
g++ -std=c++17 random_order_generator.cpp -o generator
./generator
./engine < input_orders.txt or cmd /c "engine.exe < input_orders.txt" #(PowerShell)

```

### 3. Launch the Dashboard

```bash
pip install streamlit #(if not installed)
streamlit run dashboard.py
```

## Dashboard Overview

- Market Depth: Bar charts for current buy and sell orders
- Price Movement: Line chart showing recent price trends
- Order Status: Pie chart of all order statuses
- Trade Log: Table and volume aggregation by price
- Live Ticker: Displays the most recent trade (price, quantity, time)

## Concepts Demonstrated

- Efficient order book design using C++ STL (maps, queues)
- Modular architecture: separation of matching logic, data persistence, and UI
- Real-time data pipelines and visualization
- Persistent state management using CSV I/O
- Integration between C++ and Python components

## Sanpshots

![Screenshot 2025-06-19 182721](https://github.com/user-attachments/assets/91048a8e-20b0-4378-98e7-b3cdefc88e42)

![Screenshot 2025-06-19 182746](https://github.com/user-attachments/assets/2ca8de64-d0a2-4a67-a942-766296cc3c9a)

![Screenshot 2025-06-19 182807](https://github.com/user-attachments/assets/3226ad6e-a706-492b-886d-f6f63129f772)

![image](https://github.com/user-attachments/assets/7f52060d-840b-47d5-86ea-fa5bfb6144f9)




## Author

This project was developed as part of a systems development initiative aimed at exploring real-time market simulation and software engineering concepts for trading systems.

## License

This project is licensed under the MIT License.
