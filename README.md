# Flower Exchange Trading System

## Introduction
The Flower Exchange is a trading system designed for the basic trading of flowers. It includes a Trader Application for submitting buy or sell orders and an Exchange Application for processing these orders.

## Features
- **Trader Application:** Traders can submit buy or sell orders for flowers.
- **Exchange Application:** This application processes incoming orders against existing orders in the Order Book, performing full or partial executions.
- **Execution Report:** Every order is responded to with an Execution Report by the Exchange Application, indicating the status of the order.
- **Order Validation:** Orders may be rejected due to quantity limitations, invalid flower types, etc.

## Implementation
The system is implemented in C++ and only includes algorithms to generate execution reports for given order files [submission](https://github.com/KasunAb/Flower-Exchange-System/blob/main/submission.cpp). This robust and efficient approach ensures smooth operation and accurate order processing.

## Improvements
To enhance the performance of the code, the following improvements have been implemented:

- **Thread-Safe Queue for Execution Reports:** A thread-safe queue has been introduced for managing execution reports. This queue ensures the execution reports are handled efficiently in a multi-threaded environment.

- **Writer Thread for Report Writing:** A dedicated writer thread is utilized to write the execution reports into a CSV file. This approach follows the producer-consumer model, significantly improving the performance by allowing simultaneous processing and report generation.


## Team Members
Two undergraduate students from the University of Moratuwa developed this project, specializing in Data Science and Machine Learning within the Computer Science & Engineering department.

### Kasun Abegunawardhana
- **Email:** [kasunabegunawardhana.19@cse.mrt.ac.lk](mailto://github.com/KasunAb)

### Dilusha Madushan
- **Email:** [dilusha.19@cse.mrt.ac.lk](mailto://github.com/Dilusha-Madushan)

## License
[MIT License](LICENSE)
