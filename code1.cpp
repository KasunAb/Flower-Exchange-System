#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

struct Order {
    std::string client_order_id;
    std::string instrument;
    int side;
    int quantity;
    double price;

    // Define a constructor that takes the necessary arguments.
    Order(const std::string& cid, const std::string& instr, int sd, double pr, int qty)
        : client_order_id(cid), instrument(instr), side(sd), price(pr), quantity(qty) {}
};

struct ExecutionReport {
    std::string order_id;
    std::string client_order_id;
    std::string instrument;
    int side;
    std::string exec_status; // "New", "Fill", or "PFill"
    int quantity;
    double price;

    // Constructor for ExecutionReport
    ExecutionReport(const std::string& oid, const std::string& cid, const std::string& instr, int sd, 
                    const std::string& status, int qty, double pr)
        : order_id(oid), client_order_id(cid), instrument(instr), side(sd), 
          exec_status(status), quantity(qty), price(pr) {}
};


// Helper functions
std::vector<Order> read_orders_from_csv(const std::string& file_path);
bool validate_order(const Order& order); // Assume this is implemented elsewhere.
std::string generate_order_id(int& count); // Assume this is implemented elsewhere.
std::vector<ExecutionReport> process_orders(const std::vector<Order>& orders);

int safe_stoi(const std::string& str) {
    // Check if the string is empty or contains any non-digit characters
    if(str.empty() || !std::all_of(str.begin(), str.end(), ::isdigit)) {
        throw std::invalid_argument("Input string is not a valid integer");
    }
    return std::stoi(str);
}


#include <iostream> // Make sure this is included for std::cout

std::vector<ExecutionReport> process_orders(const std::vector<Order>& orders) {
    std::vector<ExecutionReport> execution_reports;
    std::map<double, std::vector<Order>> buy_orders; // Sorted by price in descending order
    int order_count = 0;

    std::cout << "Starting order processing. Total orders: " << orders.size() << std::endl;

    for (const Order& incoming_order : orders) {
        std::cout << "Processing order: " << incoming_order.client_order_id << std::endl;

        // Validate the incoming order first
        if (!validate_order(incoming_order)) {
            std::cout << "Order " << incoming_order.client_order_id << " is invalid." << std::endl;
            execution_reports.push_back({generate_order_id(order_count), incoming_order.client_order_id,
                                         incoming_order.instrument, incoming_order.side, "Rejected",
                                         incoming_order.quantity, incoming_order.price});
            continue; // Skip invalid orders, alternatively add a rejected report
        }

        if (incoming_order.side == 1) { // Buy order
            std::cout << "Adding buy order to order book: " << incoming_order.client_order_id << std::endl;
            // Add to buy order book
            buy_orders[incoming_order.price].push_back(incoming_order);
            // Generate a "New" execution report for the buy order
            execution_reports.push_back({generate_order_id(order_count), incoming_order.client_order_id,
                                         incoming_order.instrument, incoming_order.side, "New",
                                         incoming_order.quantity, incoming_order.price});
        } else if (incoming_order.side == 2) { // Sell order
            int remaining_quantity = incoming_order.quantity;
            std::cout << "Processing sell order: " << incoming_order.client_order_id << std::endl;

            // Match with buy orders starting from the highest price
            for (auto it = buy_orders.rbegin(); it != buy_orders.rend() && remaining_quantity > 0; ) {
                double buy_price = it->first;
                std::vector<Order>& buy_order_list = it->second;

                for (auto& buy_order : buy_order_list) {
                    if (remaining_quantity == 0) break;

                    int trade_quantity = std::min(remaining_quantity, buy_order.quantity);
                    remaining_quantity -= trade_quantity;
                    buy_order.quantity -= trade_quantity;

                    std::cout << "Matching sell order with buy order. Trade quantity: " << trade_quantity << std::endl;

                    // Generate a "Fill" or "PFill" execution report for the buy order
                    execution_reports.push_back({generate_order_id(order_count), buy_order.client_order_id,
                                                 buy_order.instrument, 1, buy_order.quantity == 0 ? "Fill" : "PFill",
                                                 trade_quantity, buy_price});
                }

                // Remove fully matched buy orders
                buy_order_list.erase(std::remove_if(buy_order_list.begin(), buy_order_list.end(),
                                                    [](const Order& o) { return o.quantity == 0; }),
                                     buy_order_list.end());

                if (buy_order_list.empty()) {
                    it = std::map<double, std::vector<Order>>::reverse_iterator(buy_orders.erase(std::next(it).base()));
                } else {
                    ++it;
                }
            }

            std::string exec_status = remaining_quantity == incoming_order.quantity ? "New" : (remaining_quantity > 0 ? "PFill" : "Fill");
            std::cout << "Sell order status: " << exec_status << ", Remaining quantity: " << remaining_quantity << std::endl;

            // Generate a "Fill" or "PFill" execution report for the sell order
            execution_reports.push_back({generate_order_id(order_count), incoming_order.client_order_id,
                                         incoming_order.instrument, 2, exec_status,
                                         incoming_order.quantity - remaining_quantity, incoming_order.price});
        }
    }

    std::cout << "Finished processing orders. Total reports generated: " << execution_reports.size() << std::endl;

    return execution_reports;
}


// This is just a placeholder for the actual ID generation logic.
std::string generate_order_id(int& count) {
    return "ord" + std::to_string(++count);
}

// Assume validate_order is implemented to check order parameters such as price and quantity.
bool validate_order(const Order& order) {
    // Check if the instrument is valid
    std::vector<std::string> valid_instruments = {"Rose", "Lavender", "Lotus", "Tulip", "Orchid"};
    if (std::find(valid_instruments.begin(), valid_instruments.end(), order.instrument) == valid_instruments.end()) {
        std::cout << "Invalid instrument: " << order.instrument << std::endl;
        return false;
    }

    // Check if the side is valid (1 for buy, 2 for sell)
    if (order.side != 1 && order.side != 2) {
        std::cout << "Invalid side for order " << order.client_order_id << ": " << order.side << std::endl;
        return false;
    }

    // Check if the price is greater than 0
    if (order.price <= 0) {
        std::cout << "Invalid price for order " << order.client_order_id << ": " << order.price << std::endl;
        return false;
    }

    // Check if the quantity is a multiple of 10 and within the range [10, 1000]
    if (order.quantity % 10 != 0 || order.quantity < 10 || order.quantity > 1000) {
        std::cout << "Invalid quantity for order " << order.client_order_id << ": " << order.quantity << std::endl;
        return false;
    }

    std::cout << "Order " << order.client_order_id << " is valid." << std::endl;

    // If all checks pass, the order is valid
    return true;
}

std::vector<Order> read_orders_from_csv(const std::string& file_path) {
    std::vector<Order> orders;
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
                

                orders.emplace_back(row[0], row[1], side, price, quantity);
            } catch (const std::invalid_argument& e) {
                // Handle the conversion error, e.g., skip the line or handle it as needed
                std::cerr << "Error parsing line: " << line << "\n" << e.what() << std::endl;
            }
        }
    }
    return orders;
}



int main() {
    std::string input_file_path = "order-1.csv"; // The path to your CSV file
    std::string output_file_path = "execution_reports-1.csv"; // Path for the output file

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
    outfile << "Order ID,Client Order ID,Instrument,Side,Exec Status,Quantity,Price\n";

    // Write the execution reports to the file in CSV format
    for (const auto& report : reports) {
        outfile << report.order_id << ","
                << report.client_order_id << ","
                << report.instrument << ","
                << (report.side == 1 ? "Buy" : "Sell") << ","
                << report.exec_status << ","
                << report.quantity << ","
                << report.price << "\n"; // Use '\n' for new line in files instead of std::endl
    }

    outfile.close();
    return 0;
}