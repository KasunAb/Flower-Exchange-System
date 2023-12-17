#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Order {
    std::string order_id;
    std::string client_order_id;
    std::string instrument;
    int side;
    int quantity;
    double price;

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
    int exec_status;
    int quantity;
    double price;
    std::string reason;
    std::string timestamp;

    ExecutionReport(const std::string& oid, const std::string& cid, const std::string& instr, int sd, 
                    int status, int qty, double pr, const std::string& r, const std::string& ts)
        : order_id(oid), client_order_id(cid), instrument(instr), side(sd), 
          exec_status(status), quantity(qty), price(pr), reason(r), timestamp(ts) {}
};

// functions
std::vector<Order> read_orders_from_csv(const std::string& file_path);
bool validate_order(const Order& order, std::string& reason);
std::string generate_order_id(int& count);
std::vector<ExecutionReport> process_orders(std::vector<Order>& orders);

std::string current_time() {
    auto now = std::chrono::system_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    auto now_c = std::chrono::system_clock::to_time_t(now);

    std::tm now_tm = *std::localtime(&now_c);

    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y%m%d-%H%M%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

int safe_stoi(const std::string& str) {
    // Check if the string is empty or contains any non-digit characters
    if(str.empty() || !std::all_of(str.begin(), str.end(), ::isdigit)) {
        throw std::invalid_argument("Input string is not a valid integer");
    }
    return std::stoi(str);
}

int getExecutionReportStatus(const std::string& status) {
    static const std::map<std::string, int> statusMap = {
        {"New", 0},
        {"Rejected", 1},
        {"Fill", 2},
        {"PFill", 3}
    };

    auto it = statusMap.find(status);
    if (it != statusMap.end()) {
        return it->second;
    } else {
        return -1;
    }
};

struct BuyOrderCompare {
    bool operator()(const Order& lhs, const Order& rhs) const {
        return (lhs.price == rhs.price) ? lhs.client_order_id > rhs.client_order_id : lhs.price < rhs.price;
    }
};

struct SellOrderCompare {
    bool operator()(const Order& lhs, const Order& rhs) const {
        return (lhs.price == rhs.price) ? lhs.client_order_id > rhs.client_order_id : lhs.price > rhs.price;
    }
};

ExecutionReport createExecutionReport(const Order& order, const std::string& status, int quantity, double price, const std::string& reason = "") {
    return ExecutionReport(order.order_id, order.client_order_id, order.instrument, order.side, getExecutionReportStatus(status), quantity, price, reason, current_time());
}

template <typename OrderCompare>
void processMatchingOrders(Order& incoming_order, std::priority_queue<Order, std::vector<Order>, OrderCompare>& opposite_orders, std::vector<ExecutionReport>& reports, bool isBuyOrder) {
    while (!opposite_orders.empty() && ((isBuyOrder && opposite_orders.top().price <= incoming_order.price) || (!isBuyOrder && opposite_orders.top().price >= incoming_order.price)) && incoming_order.quantity > 0) {
        const Order& top_order = opposite_orders.top();

        int trade_quantity = std::min(incoming_order.quantity, top_order.quantity);
        incoming_order.quantity -= trade_quantity;
        Order modified_top_order = top_order;
        modified_top_order.quantity -= trade_quantity;

        reports.push_back(createExecutionReport(incoming_order, incoming_order.quantity == 0 ? "Fill" : "PFill", trade_quantity, top_order.price));
        reports.push_back(createExecutionReport(modified_top_order, modified_top_order.quantity == 0 ? "Fill" : "PFill", trade_quantity, top_order.price));

        opposite_orders.pop();
        if (modified_top_order.quantity > 0) {
            opposite_orders.push(modified_top_order);
        }
    }
}

std::vector<ExecutionReport> process_orders(std::vector<Order>& orders) {
    std::vector<ExecutionReport> execution_reports;
    std::map<std::string, std::priority_queue<Order, std::vector<Order>, BuyOrderCompare>> buy_order_books;
    std::map<std::string, std::priority_queue<Order, std::vector<Order>, SellOrderCompare>> sell_order_books;

    for (Order& incoming_order : orders) {
        std::string validationReason;
        if (!validate_order(incoming_order, validationReason)) {
            execution_reports.push_back(createExecutionReport(incoming_order, "Rejected", incoming_order.quantity, incoming_order.price, validationReason));
            continue;
        }

        if (incoming_order.side == 1) { // Buy order
            if (sell_order_books[incoming_order.instrument].empty() || sell_order_books[incoming_order.instrument].top().price > incoming_order.price) {
                execution_reports.push_back(createExecutionReport(incoming_order, "New", incoming_order.quantity, incoming_order.price));
            }
            processMatchingOrders<SellOrderCompare>(incoming_order, sell_order_books[incoming_order.instrument], execution_reports, true);
            if (incoming_order.quantity > 0) {
                buy_order_books[incoming_order.instrument].push(incoming_order);
            }
        } else if (incoming_order.side == 2) { // Sell order
            if (buy_order_books[incoming_order.instrument].empty() || buy_order_books[incoming_order.instrument].top().price < incoming_order.price) {
                execution_reports.push_back(createExecutionReport(incoming_order, "New", incoming_order.quantity, incoming_order.price));
            }
            processMatchingOrders<BuyOrderCompare>(incoming_order, buy_order_books[incoming_order.instrument], execution_reports, false);
            if (incoming_order.quantity > 0) {
                sell_order_books[incoming_order.instrument].push(incoming_order);
            }
        }
    }

    return execution_reports;
}


std::string generate_order_id(int& count) {
    return "ord" + std::to_string(++count);
}

bool validate_order(const Order& order, std::string& reason) {
    static const std::set<std::string> valid_instruments = {"Rose", "Lavender", "Lotus", "Tulip", "Orchid"};
    
    if (valid_instruments.find(order.instrument) == valid_instruments.end()) {
        reason = "Invalid instrument: " + order.instrument;
        return false;
    }

    if (order.side != 1 && order.side != 2) {
        reason = "Invalid side for order " + order.client_order_id + ": " + std::to_string(order.side);
        return false;
    }

    if (order.price <= 0) {
        reason = "Invalid price for order " + order.client_order_id + ": " + std::to_string(order.price);
        return false;
    }

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
    bool is_header = true; 
    while (std::getline(file, line)) {
        if (is_header) {
            is_header = false;
            continue;
        }
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::vector<std::string> row;
        std::string value;

        while (std::getline(ss, value, ',')) {
            row.push_back(value);
        }

        if (row.size() >= 5) {
            try {
               
                int side = safe_stoi(row[2]);
                int quantity = safe_stoi(row[3]);
                double price = std::stod(row[4]);
                
                orders.emplace_back(generate_order_id(order_count), row[0], row[1], side, price, quantity);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error parsing line: " << line << "\n" << e.what() << std::endl;
            }
        }
    }
    return orders;
}

int write_execution_reports_to_csv(const std::string& output_file_path, const std::vector<ExecutionReport>& reports) {
    std::ofstream outfile(output_file_path);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open the output file." << std::endl;
        return 1;
    }

    outfile << "Client Order ID,Order ID,Instrument,Side,Price,Quantity,Status,Reason,Transaction Time\n";

    for (const auto& report : reports) {
        std::ostringstream line;
        line << report.client_order_id << ","
             << report.order_id << ","
             << report.instrument << ","
             << (report.side == 1 ? "Buy" : "Sell") << ","
             << report.price << ","
             << report.quantity << ","
             << report.exec_status << ","
             << report.reason << ","
             << report.timestamp << "\n";
        outfile << line.str();
    }

    outfile.close();

    return 0;
}


int main() {
    std::string input_file_path = "test/inputs/orders.csv"; // The path to your order CSV file
    std::string output_file_path = "test/outputs/execution_rep.csv"; // Path for the execution report file

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

    int result = write_execution_reports_to_csv(output_file_path, reports);

    return result;
    
}
