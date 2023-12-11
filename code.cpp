#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

// Assuming that we have a predefined list of valid instruments.
const std::vector<std::string> validInstruments = {"Rose"};

class Order {
public:
    std::string clientOrderID;
    std::string instrument;
    int side; // 1 for buy, 2 for sell
    int quantity;
    double price;

    Order(const std::string& clOrdID, const std::string& instr, int s, int qty, double pr)
        : clientOrderID(clOrdID), instrument(instr), side(s), quantity(qty), price(pr) {}

    // Validates the order fields
    bool isValid() {
        // Check required fields are present (assuming clientOrderID is also required)
        if (clientOrderID.empty() || instrument.empty() || side == 0 || quantity == 0 || price == 0) {
            return false;
        }
        // Check for valid instrument
        if (std::find(validInstruments.begin(), validInstruments.end(), instrument) == validInstruments.end()) {
            return false;
        }
        // Check for valid side
        if (side != 1 && side != 2) {
            return false;
        }
        // Check if price is greater than 0
        if (price <= 0) {
            return false;
        }
        // Check if quantity is a multiple of 10
        if (quantity % 10 != 0) {
            return false;
        }
        // Check if quantity is within valid range
        if (quantity < 10 || quantity > 1000) {
            return false;
        }
        return true;
    }
};

bool compareBuyOrders(const Order& a, const Order& b) {
    return a.price > b.price; // higher price first for buys
}

int main() {
    std::ifstream ordersFile("orders.csv");
    std::ofstream execFile("execution_rep.csv");
    std::string line;
    std::vector<Order> buyOrders, sellOrders;
    std::map<std::string, std::string> orderIDMap; // Map to track generated Order IDs

    // Read orders from CSV file
    while (std::getline(ordersFile, line)) {
        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> tokens;
        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }
        // Skip header or invalid lines
        if (tokens.size() != 5 || tokens[0] == "Cl. Ord.ID") continue;

        Order order(tokens[0], tokens[1], std::stoi(tokens[2]), std::stoi(tokens[3]), std::stod(tokens[4]));
        // Generate a unique Order ID for each Client Order ID
        std::string generatedOrderID = "ord" + std::to_string(orderIDMap.size() + 1);
        orderIDMap[order.clientOrderID] = generatedOrderID;

        // Validate the order before processing
        if (order.isValid()) {
            if (order.side == 1) { // Buy order
                buyOrders.push_back(order);
            } else { // Sell order
                sellOrders.push_back(order);
            }
        } else {
            // Write rejected order to the execution report
            execFile << generatedOrderID << "," << order.clientOrderID << ","
                     << order.instrument << "," << order.side << ",Rejected,"
                     << order.quantity << "," << order.price << "\n";
        }
    }
    ordersFile.close();

    // Sort buy orders by price in descending order
    std::sort(buyOrders.begin(), buyOrders.end(), compareBuyOrders);

    // Match sell orders with buy orders based on price priority
    for (Order& sellOrder : sellOrders) {
        for (Order& buyOrder : buyOrders) {
            if (buyOrder.quantity > 0 && sellOrder.quantity > 0 && buyOrder.price >= sellOrder.price) {
                int executedQuantity = std::min(sellOrder.quantity, buyOrder.quantity);
                std::string execStatus = (executedQuantity == sellOrder.quantity) ? "Fill" : "Pfill";
                sellOrder.quantity -= executedQuantity;
                buyOrder.quantity -= executedQuantity;

                // Write execution reports
                execFile << orderIDMap[buyOrder.clientOrderID] << "," << buyOrder.clientOrderID << ","
                         << buyOrder.instrument << "," << buyOrder.side << "," << execStatus << ","
                         << executedQuantity << "," << buyOrder.price << "\n";

                execFile << orderIDMap[sellOrder.clientOrderID] << "," << sellOrder.clientOrderID << ","
                         << sellOrder.instrument << "," << sellOrder.side << "," << execStatus << ","
                         << executedQuantity << "," << buyOrder.price << "\n";

                if (sellOrder.quantity == 0) {
                    break; // Sell order is fully filled, move to next sell order
                }
            }
        }
    }

    // Write remaining new buy orders to the execution report
    for (Order& buyOrder : buyOrders) {
        if (buyOrder.quantity > 0) {
            execFile << orderIDMap[buyOrder.clientOrderID] << "," << buyOrder.clientOrderID << ","
                     << buyOrder.instrument << "," << buyOrder.side << ",New,"
                     << buyOrder.quantity << "," << buyOrder.price << "\n";
        }
    }

    // Write remaining new sell orders to the execution report
    for (Order& sellOrder : sellOrders) {
        if (sellOrder.quantity > 0) {
            execFile << orderIDMap[sellOrder.clientOrderID] << "," << sellOrder.clientOrderID << ","
                     << sellOrder.instrument << "," << sellOrder.side << ",New,"
                     << sellOrder.quantity << "," << sellOrder.price << "\n";
        }
    }

    execFile.close();
    return 0;
}
