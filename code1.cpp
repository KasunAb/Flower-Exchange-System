#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <chrono>
#include <map>
#include <algorithm>
#include <functional> 
#include <iomanip>

struct Order {
    std::string order_id;
    std::string client_order_id;
    std::string instrument;
    int side;
    int quantity;
    double price;

    // Define a constructor that takes the necessary arguments.
    Order(const std::string& id, const std::string& cid, const std::string& instr, int sd, double pr, int qty)
        : order_id(id), client_order_id(cid), instrument(instr), side(sd), price(pr), quantity(qty) {}

    void print() const {
        std::cout << "Order - Client Order ID: " << client_order_id
                  << ", Instrument: " << instrument
                  << ", Side: " << (side == 1 ? "Buy" : "Sell")
                  << ", Quantity: " << quantity
                  << ", Price: " << price << std::endl;
    }
};

struct ExecutionReport {
    std::string order_id;
    std::string client_order_id;
    std::string instrument;
    int side;
    std::string exec_status; // "New", "Fill", or "PFill"
    int quantity;
    double price;
    std::string reason;
    std::string timestamp;

    // Constructor for ExecutionReport
    ExecutionReport(const std::string& oid, const std::string& cid, const std::string& instr, int sd, 
                    const std::string& status, int qty, double pr, const std::string& r, const std::string& ts)
        : order_id(oid), client_order_id(cid), instrument(instr), side(sd), 
          exec_status(status), quantity(qty), price(pr), reason(r), timestamp(ts) {}
};

// std::string current_time() {
//     auto now = std::chrono::system_clock::now();
//     auto now_c = std::chrono::system_clock::to_time_t(now);
//     std::stringstream ss;
//     ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
//     return ss.str();
// }

std::string current_time() {
    // Get the current time
    auto now = std::chrono::system_clock::now();

    // Get the number of milliseconds since the last second (remainder after division into seconds)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Convert system time to `time_t` in order to convert to calendar time
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to `tm` struct for use with `strftime`
    std::tm now_tm = *std::localtime(&now_c);

    // Use a stringstream to concatenate the formatted date/time with milliseconds
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y%m%d-%H%M%S"); // Format without the milliseconds
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count(); // Add milliseconds

    return ss.str();
}


// Helper functions
std::vector<Order> read_orders_from_csv(const std::string& file_path);
bool validate_order(const Order& order, std::string& reason);
std::string generate_order_id(int& count);
std::vector<ExecutionReport> process_orders(std::vector<Order>& orders);

int safe_stoi(const std::string& str) {
    // Check if the string is empty or contains any non-digit characters
    if(str.empty() || !std::all_of(str.begin(), str.end(), ::isdigit)) {
        throw std::invalid_argument("Input string is not a valid integer");
    }
    return std::stoi(str);
}


struct BuyOrderCompare {
    bool operator()(const Order& lhs, const Order& rhs) const {
        if (lhs.price == rhs.price) {
            return lhs.client_order_id > rhs.client_order_id; // Earlier order first
        }
        return lhs.price < rhs.price; // Higher price first
    }
};

struct SellOrderCompare {
    bool operator()(const Order& lhs, const Order& rhs) const {
        if (lhs.price == rhs.price) {
            return lhs.client_order_id > rhs.client_order_id; // Earlier order first
        }
        return lhs.price > rhs.price; // Lower price first
    }
};

// TO DO : 
// 0 – New
// 1 – Rejected
// 2 – Fill
// 3 - Pfill

std::vector<ExecutionReport> process_orders(std::vector<Order>& orders) {
    std::vector<ExecutionReport> execution_reports;
    std::priority_queue<Order, std::vector<Order>, BuyOrderCompare> buy_order_book;
    std::priority_queue<Order, std::vector<Order>, SellOrderCompare> sell_order_book;

    for (Order incoming_order : orders) { // Create a modifiable copy of each incoming_order
        std::cout << "Processing order: " << incoming_order.client_order_id << "\n";
        std::string validationReason;
        if (!validate_order(incoming_order, validationReason)) {
            execution_reports.push_back(ExecutionReport(
                incoming_order.order_id, incoming_order.client_order_id,
                incoming_order.instrument, incoming_order.side, "Rejected",
                incoming_order.quantity, incoming_order.price, validationReason, current_time()
            ));
            continue;
        }

        

        if (incoming_order.side == 1) { // Buy order
            std::cout << "Incoming order is a buy order.\n";
            if (sell_order_book.empty() || sell_order_book.top().price > incoming_order.price) {
                execution_reports.push_back(ExecutionReport(
                    incoming_order.order_id, incoming_order.client_order_id,
                    incoming_order.instrument, 1, "New",
                    incoming_order.quantity, incoming_order.price, "", current_time()
                ));
            }
            while (!sell_order_book.empty() && sell_order_book.top().price <= incoming_order.price && incoming_order.quantity > 0) {
                Order sell_order = sell_order_book.top();
                sell_order_book.pop();

                int trade_quantity = std::min(incoming_order.quantity, sell_order.quantity);
                incoming_order.quantity -= trade_quantity;
                sell_order.quantity -= trade_quantity;

                execution_reports.push_back(ExecutionReport(
                    incoming_order.order_id, incoming_order.client_order_id,
                    incoming_order.instrument, 1, incoming_order.quantity == 0 ? "Fill" : "PFill",
                    trade_quantity, sell_order.price, "", current_time()
                ));

                execution_reports.push_back(ExecutionReport(
                    sell_order.order_id, sell_order.client_order_id,
                    incoming_order.instrument, 2, sell_order.quantity == 0 ? "Fill" : "PFill",
                    trade_quantity, sell_order.price, "", current_time()
                ));

                if (sell_order.quantity > 0) {
                    sell_order_book.push(sell_order);
                }
            }
            if (incoming_order.quantity > 0) {
                buy_order_book.push(incoming_order);
            }
        } else if (incoming_order.side == 2) { // Sell order
            std::cout << "Incoming order is a sell order.\n";
            if (buy_order_book.empty() || buy_order_book.top().price < incoming_order.price) {
                execution_reports.push_back(ExecutionReport(
                    incoming_order.order_id, incoming_order.client_order_id,
                    incoming_order.instrument, 2, "New",
                    incoming_order.quantity, incoming_order.price, "", current_time()
                ));
            }
            while (!buy_order_book.empty() && buy_order_book.top().price >= incoming_order.price && incoming_order.quantity > 0) {
                Order buy_order = buy_order_book.top();
                buy_order_book.pop();

                int trade_quantity = std::min(incoming_order.quantity, buy_order.quantity);
                incoming_order.quantity -= trade_quantity;
                buy_order.quantity -= trade_quantity;

                execution_reports.push_back(ExecutionReport(
                    buy_order.order_id, buy_order.client_order_id,
                    buy_order.instrument, 1, buy_order.quantity == 0 ? "Fill" : "PFill",
                    trade_quantity, buy_order.price, "", current_time()
                ));

                execution_reports.push_back(ExecutionReport(
                    incoming_order.order_id, incoming_order.client_order_id,
                    buy_order.instrument, 2, incoming_order.quantity == 0 ? "Fill" : "PFill",
                    trade_quantity, buy_order.price, "", current_time()
                ));

                if (buy_order.quantity > 0) {
                    buy_order_book.push(buy_order);
                }
            }
            if (incoming_order.quantity > 0) {
                sell_order_book.push(incoming_order);
            }
        }
    }

    // Remaining unmatched buy and sell orders can be handled here if needed


    return execution_reports;
}

// This is just a placeholder for the actual ID generation logic.
std::string generate_order_id(int& count) {
    return "ord" + std::to_string(++count);
}

// Assume validate_order is implemented to check order parameters such as price and quantity.
bool validate_order(const Order& order, std::string& reason) {
    // Check if the instrument is valid
    std::vector<std::string> valid_instruments = {"Rose", "Lavender", "Lotus", "Tulip", "Orchid"};
    if (std::find(valid_instruments.begin(), valid_instruments.end(), order.instrument) == valid_instruments.end()) {
        reason = "Invalid instrument: " + order.instrument;
        return false;
    }

    // Check if the side is valid (1 for buy, 2 for sell)
    if (order.side != 1 && order.side != 2) {
        reason = "Invalid side for order " + order.client_order_id + ": " + std::to_string(order.side);
        return false;
    }

    // Check if the price is greater than 0
    if (order.price <= 0) {
        reason = "Invalid price for order " + order.client_order_id + ": " + std::to_string(order.price);
        return false;
    }

    // Check if the quantity is a multiple of 10 and within the range [10, 1000]
    if (order.quantity % 10 != 0 || order.quantity < 10 || order.quantity > 1000) {
        reason = "Invalid quantity for order " + order.client_order_id + ": " + std::to_string(order.quantity);
        return false;
    }

    reason = "Order " + order.client_order_id + " is valid.";
    return true;
}


std::vector<Order> read_orders_from_csv(const std::string& file_path) {
    std::vector<Order> orders;
    int order_count = 0;

    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }
    
    std::string line;
    bool is_header = true; // Flag to track if we're on the header line
    while (std::getline(file, line)) {
        if (is_header) { // If it's the first line (header), skip it
            is_header = false;
            continue;
        }
        if (line.empty()) continue; // Skip empty lines

        std::stringstream ss(line);
        std::vector<std::string> row;
        std::string value;

        while (std::getline(ss, value, ',')) {
            row.push_back(value);
        }

        if (row.size() >= 5) {
            try {
                // Using safe_stoi to convert string to int safely
                int side = safe_stoi(row[2]);
                int quantity = safe_stoi(row[3]);
                double price = std::stod(row[4]);
                

                orders.emplace_back(generate_order_id(order_count), row[0], row[1], side, price, quantity);
            } catch (const std::invalid_argument& e) {
                // Handle the conversion error, e.g., skip the line or handle it as needed
                std::cerr << "Error parsing line: " << line << "\n" << e.what() << std::endl;
            }
        }
    }
    return orders;
}



int main() {
    std::string input_file_path = "files/inputs/orders - large.csv"; // The path to your CSV file
    std::string output_file_path = "files/inputs/execution_rep - large.csv"; // Path for the output file

    std::vector<Order> orders = read_orders_from_csv(input_file_path);
    std::cout << "Number of orders read: " << orders.size() << std::endl;

    if (orders.empty()) {
        std::cerr << "No orders were read from the file." << std::endl;
        return 1;
    }

    std::vector<ExecutionReport> reports = process_orders(orders);
    std::cout << "Number of execution reports generated: " << reports.size() << std::endl;

    if (reports.empty()) {
        std::cerr << "No execution reports were generated." << std::endl;
        return 1;
    }

    std::ofstream outfile(output_file_path);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open the output file." << std::endl;
        return 1;
    }

    // Write the column headers
    outfile << "Client Order ID,Order ID,Instrument,Side,Price,Quantity,Status,Reason,Transaction Time \n";

    // Write the execution reports to the file in CSV format
    for (const auto& report : reports) {
        outfile << report.client_order_id << ","
                << report.order_id << ","
                << report.instrument << ","
                << report.side << ","
                << report.price << ","
                << report.quantity << ","
                << report.exec_status << ","
                << report.reason << ","
                << report.timestamp << "\n";
    }

    outfile.close();
    return 0;
    
}

