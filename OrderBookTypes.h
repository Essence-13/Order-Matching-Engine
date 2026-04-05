#ifndef ORDER_BOOK_TYPES_H
#define ORDER_BOOK_TYPES_H

#include "Order.h"
#include <map>
#include <list>
#include <unordered_map>

// --- Central Type Definitions ---

// A map of: Price -> List of Orders at that Price
// (Buy book is sorted high-to-low price)
using BuyBookType = std::map<int, std::list<Order>, std::greater<int>>;

// (Sell book is sorted low-to-high price)
using SellBookType = std::map<int, std::list<Order>>;

// A map of: OrderID -> "Bookmark" (Iterator) to the Order's
// location in the BuyBookType's list.
using BuyIterMap = std::unordered_map<int, BuyBookType::mapped_type::iterator>;

// A map of: OrderID -> "Bookmark" (Iterator) to the Order's
// location in the SellBookType's list.
using SellIterMap = std::unordered_map<int, SellBookType::mapped_type::iterator>;


#endif // ORDER_BOOK_TYPES_H

