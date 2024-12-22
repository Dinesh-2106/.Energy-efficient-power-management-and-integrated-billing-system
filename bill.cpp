#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <sstream>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <climits> // Include for INT_MAX
#include <algorithm> // Include for max_element and min_element

using namespace std;

const double FINE_RATE = 0.02; // Fine rate per day (2% of the bill amount)

int getch() {
#ifdef _WIN32
    return _getch();
#else
    char buf = 0;
    struct termios old = {0};
    struct termios newTerm = {0};
    fflush(stdout);
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    newTerm = old;
    newTerm.c_lflag &= ~ICANON;
    newTerm.c_lflag &= ~ECHO;
    newTerm.c_cc[VMIN] = 1;
    newTerm.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &newTerm) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
#endif
}

class Transaction {
public:
    std::string date;
    double amount;

    Transaction(double amount) : amount(amount) {
        time_t now = time(0);
        tm* currentDateTime = localtime(&now);
        char dateBuffer[20];
        strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d %H:%M:%S", currentDateTime);
        date = dateBuffer;
    }
};

enum ElectricityType {
    DOMESTIC,
    COMMERCIAL
};

class Bill {
public:
    int id;
    string vendorName;
    double amount;
    string dueDate;
    string status;
    ElectricityType electricityType;
    std::vector<Transaction> transactionHistory;

    Bill(int id, const string& vendorName, double amount, const string& status, ElectricityType electricityType)
        : id(id), vendorName(vendorName), amount(amount), status(status), electricityType(electricityType) {
        time_t now = time(0);
        tm* currentDateTime = localtime(&now);
        char dateBuffer[11];
        strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", currentDateTime);
        dueDate = dateBuffer;
    }
};

int billIdCounter = 1;

void addBill(vector<Bill>& bills, const string& vendorName, double amount, const string& status, ElectricityType electricityType) {
    Bill newBill(billIdCounter++, vendorName, amount, status, electricityType);

    time_t now = time(0);
    tm* currentDateTime = localtime(&now);
    currentDateTime->tm_mday += 10;
    mktime(currentDateTime);
    char dateBuffer[11];
    strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", currentDateTime);
    newBill.dueDate = dateBuffer;

    Transaction transaction(amount);
    newBill.transactionHistory.push_back(transaction);

    if (vendorName == "Deposit") {
        newBill.status = "Paid";
        newBill.dueDate = "";
    }

    bills.push_back(newBill);
    cout << "Bill added successfully.\n";
}

void viewBills(const vector<Bill>& bills, bool isGeneralUser) {
    if (bills.empty()) {
        cout << "No bills found.\n";
        return;
    }

    cout << setw(5) << "ID" << setw(20) << "Vendor Name" << setw(10) << "Amount" << setw(15) << "Due Date" << setw(10) << "Status" << setw(15) << "Electricity Type" << endl;
    cout << setfill('-') << setw(80) << "-" << setfill(' ') << endl;

    time_t now = time(0);
    tm* currentDateTime = localtime(&now);

    for (const Bill& bill : bills) {
        cout << setw(5) << bill.id << setw(20) << bill.vendorName << setw(10) << bill.amount;

        if (bill.vendorName != "Deposit") {
            cout << setw(15) << bill.dueDate;
        } else {
            cout << setw(15) << "N/A";
        }

        cout << setw(10) << bill.status;

        if (bill.electricityType == DOMESTIC) {
            cout << setw(15) << "Domestic";
        } else if (bill.electricityType == COMMERCIAL) {
            cout << setw(15) << "Commercial";
        }

        if (isGeneralUser && bill.status == "Unpaid" && bill.vendorName != "Deposit") {
            tm dueDateTime = {};
            istringstream dateStream(bill.dueDate);
            dateStream >> get_time(&dueDateTime, "%Y-%m-%d");
            if (mktime(&dueDateTime) < mktime(currentDateTime)) {
                int daysRemaining = (mktime(&dueDateTime) - mktime(currentDateTime)) / (60 * 60 * 24);
                if (daysRemaining <= 10) {
                    cout << " (Due in " << daysRemaining << " days)";
                } else {
                    cout << " (Due in more than 10 days)";
                }
            }
        }

        cout << endl;
    }
}

void markAsPaid(vector<Bill>& bills, int billId) {
    for (Bill& bill : bills) {
        if (bill.id == billId) {
            if (bill.status == "Unpaid") {
                // Calculate fine based on the due date and the current date
                time_t now = time(0);
                tm* currentDateTime = localtime(&now);

                tm dueDateTime = {};
                istringstream dateStream(bill.dueDate);
                dateStream >> get_time(&dueDateTime, "%Y-%m-%d");

                int daysLate = (mktime(currentDateTime) - mktime(&dueDateTime)) / (60 * 60 * 24);

                if (daysLate > 0) {
                    double fine = bill.amount * FINE_RATE * daysLate;
                    cout << "Fine applied for late payment: RS." << fine << endl;
                    bill.amount += fine;
                }
            }

            bill.status = "Paid";
            cout << "Bill marked as paid.\n";
            return;
        }
    }
    cout << "Bill not found.\n";
}

void viewTransactionHistory(const vector<Bill>& bills) {
    if (bills.empty()) {
        cout << "No transaction history available.\n";
        return;
    }

    cout << setw(5) << "Bill ID" << setw(20) << "Vendor Name" << setw(10) << "Amount" << setw(20) << "Date" << endl;
    cout << setfill('-') << setw(60) << "-" << setfill(' ') << endl;

    for (const Bill& bill : bills) {
        for (const Transaction& transaction : bill.transactionHistory) {
            cout << setw(5) << bill.id << setw(20) << bill.vendorName << setw(10) << transaction.amount << setw(20) << transaction.date << endl;
        }
    }
}

ElectricityType getElectricityType(int choice) {
    switch (choice) {
        case 1:
            return ElectricityType::DOMESTIC;
        case 2:
            return ElectricityType::COMMERCIAL;
        default:
            return ElectricityType::DOMESTIC;
    }
}

int main() {
    vector<Bill> bills;
    int choice;
    int userRole;

    do {
        cout << "+-----------------------------------------------+" << endl;
        cout << "|    \"Efficient Power Management and            |" << endl;
        cout << "|    Integrated Billing System\"                 |" << endl;
        cout << "+-----------------------------------------------+" << endl;

        cout << "1. General Login \n";
        cout << "2. Admin Login\n";
        cout << "3. Exit\n";
        cout << "Select your Login: ";
        cin >> userRole;

        switch (userRole) {
            case 1:
                {
                    // General user login
                    int generalUserChoice;
                    do {
                        cout << "\nGeneral User Menu\n";
                        cout << "1. Calculate Electricity Bill\n";
                        cout << "2. Deposit Money\n";
                        cout << "3. Notifications\n";
                        cout << "4. Logout\n";
                        cout << "Select your Options: ";
                        cin >> generalUserChoice;

                        switch (generalUserChoice) {
                            case 1: {
                                int electricityTypeChoice;
                                int n, x;
                                double total_bill;

                                cout << "Choose electricity type (1 for Domestic, 2 for Commercial): ";
                                cin >> electricityTypeChoice;

                                if (electricityTypeChoice == 1) {
                                    cout << "Total Number of Units used: ";
                                    cin >> n;
                                    if (n > 0 && n < 101) {
                                        total_bill = (n * 4.80);
                                    }
                                    if (n > 100 && n < 201) {
                                        total_bill = (n * 5.80);
                                    }
                                    if (n > 200) {
                                        total_bill = (n * 6.50);
                                    }
                                    cout << "--------------------------------------------" << endl;
                                    cout << "Total bill = RS." << total_bill << endl;
                                    cout << "--------------------------------------------" << endl;
                                } else if (electricityTypeChoice == 2) {
                                    cout << "Total Number of Persons : ";
                                    cin >> x;
                                    vector<int> unitsUsed(x);

                                    // Assume random usage for each person (between 50 and 150 units)
                                    srand(time(NULL));
                                    for (int i = 0; i < x; ++i) {
                                        unitsUsed[i] = rand() % 101 + 50;
                                    }

                                    double total_bill = 0;
                                    for (int i = 0; i < x; ++i) {
                                        total_bill += unitsUsed[i] * 6.70; // Assuming the rate for commercial usage
                                    }

                                    cout << "--------------------------------------------" << endl;
                                    cout << "Total bill = RS." << total_bill << endl;
                                    cout << "--------------------------------------------" << endl;
                                    cout << "Bill Per Each Person:" << endl;

                                    for (int i = 0; i < x; ++i) {
                                        cout << "Person " << i + 1 << ": " << unitsUsed[i] << " units, RS." << unitsUsed[i] * 6.70 << endl;
                                    }

                                    // Find the person with the highest and lowest usage
                                    int maxUsage = *max_element(unitsUsed.begin(), unitsUsed.end());
                                    int minUsage = *min_element(unitsUsed.begin(), unitsUsed.end());
                                    int maxIndex = distance(unitsUsed.begin(), max_element(unitsUsed.begin(), unitsUsed.end()));
                                    int minIndex = distance(unitsUsed.begin(), min_element(unitsUsed.begin(), unitsUsed.end()));

                                    cout << "--------------------------------------------" << endl;
                                    cout << "Person " << maxIndex + 1 << " used the most units: " << maxUsage << " units" << endl;
                                    cout << "Person " << minIndex + 1 << " used the least units: " << minUsage << " units" << endl;
                                    addBill(bills, "Electricity", total_bill, "Unpaid", getElectricityType(electricityTypeChoice));
                                } else {
                                    cout << "Invalid choice for electricity type.\n";
                                }
                                break;
                            }
                            case 2: {
                                // Deposit Money
                                double depositAmount;
                                int electricityTypeChoice;

                                // Show unpaid bills
                                cout << "\nUnpaid Bills:\n";
                                viewBills(bills, true); // Display unpaid bills for the user

                                cout << "Choose electricity type for deposit (1 for Domestic, 2 for Commercial): ";
                                cin >> electricityTypeChoice;

                                cout << "Enter deposit amount: ₹ ";
                                cin >> depositAmount;

                                // Assuming some initial deposit
                                addBill(bills, "Deposit", depositAmount, "Paid", getElectricityType(electricityTypeChoice));
                                cout << "Deposit of ₹" << depositAmount << " successful.\n";
                                break;
                            }
                            case 3: {
                                cout << "\nNotifications\n";
                                cout << "------------------------------------------------------------------------------------\n";
                                viewBills(bills, true);
                                cout << "-------------------------------------------------------------------------------------\n";
                                break;
                            }
                            case 4:
                                cout << "Exiting  User Menu.\n";
                                break;
                            default:
                                cout << "Invalid choice. Please try again.\n";
                        }
                    } while (generalUserChoice != 4);
                    break;
                }

            case 2:
                {
                    string enteredPassword;
                    string correctPassword = "admin123";

                    cout << "Enter Admin password: ";
                    char ch;
                    string password;
                    while ((ch = getch()) != '\n') {
                    if (ch == '\b' && !password.empty()) {
                    cout << "\b \b";
                    password.pop_back();
                    }
                    else if (ch != '\b') {
                    password.push_back(ch);
                    cout << '*';
                    }
                }
                cout << endl;
			if (password != correctPassword) {
                        cout << "Incorrect password. Exiting Admin Login.\n\n\n";
                        break;
                    }

                    do {
                        cout << "\nAdmin Menu\n";
                        cout << "1. Add Bill\n";
                        cout << "2. View Bills\n";
                        cout << "3. View Transaction History\n";
                        cout << "4. Mark as Paid\n";
                        cout << "5. Logout\n";
                        cout << "Select your Options: ";
                        cin >> choice;

                        switch (choice) {
                            case 1: {
                                // Admin can add a bill
                                string vendorName;
                                double amount;
                                int electricityTypeChoice;

                                cout << "Enter Vendor Name: ";
                                cin.ignore(); // Ignore any previous newline character in the buffer
                                getline(cin, vendorName);
                                cout << "Enter Amount: ";
                                cin >> amount;
                                cout << "Choose electricity type (1 for Domestic, 2 for Commercial): ";
                                cin >> electricityTypeChoice;
                                addBill(bills, vendorName, amount, "Unpaid", getElectricityType(electricityTypeChoice));
                                break;
                            }
                            case 2:
                                viewBills(bills, false);
                                break;
                            case 3:
                                viewTransactionHistory(bills);
                                break;
                            case 4: {
                                int billId;
                                cout << "Enter the ID of the bill to mark as paid: ";
                                cin >> billId;
                                markAsPaid(bills, billId);
                                break;
                            }
                            case 5:
                                cout << "Exiting Admin Menu.\n";
                                break;
                            default:
                                cout << "Invalid choice. Please try again.\n";
                        }
                    } while (choice != 5);
                    break;
                }

            case 3:
                cout << "Exiting the program.\n";
                break;
            default:
                cout << "Invalid choice. Please try again.\n";
        }

    } while (userRole != 3);

    return 0;
}