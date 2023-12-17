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

std::vector<std::vector<std::string>> readSelectedColumns(const std::string& filename, const std::set<int>& columns) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> rowData;
        int colIndex = 0;

        while (std::getline(ss, cell, ',')) {
            if (columns.find(colIndex) != columns.end()) {
                rowData.push_back(cell);
            }
            colIndex++;
        }

        data.push_back(rowData);
    }

    return data;
}

bool compareData(const std::vector<std::vector<std::string>>& data1, const std::vector<std::vector<std::string>>& data2) {
    if (data1.size() != data2.size()) {
        return false;
    }

    for (size_t i = 0; i < data1.size(); ++i) {
        if (data1[i] != data2[i]) {
            return false;
        }
    }

    return true;
}

int main() {
    std::set<int> columnsToCompare = {0, 1, 2, 3, 4, 5, 6}; 

    std::string correct_file_path = "test/inputs/execution-rep-correct";
    std::string output_file_path = "test/outputs/execution_rep";

    std::vector<std::vector<std::string>> csvData1 = readSelectedColumns(correct_file_path, columnsToCompare);
    std::vector<std::vector<std::string>> csvData2 = readSelectedColumns(output_file_path, columnsToCompare);

    bool areSimilar = compareData(csvData1, csvData2);

    if (areSimilar) {
        std::cout << "The selected columns in the CSV files are similar." << std::endl;
    } else {
        std::cout << "The selected columns in the CSV files differ." << std::endl;
    }

    return 0;
}
