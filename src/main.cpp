#include "order_manager.hpp"
#include "utils.hpp"
#include "websocket_con.hpp"

int parseLine(char *line)
{
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char *p = line;
    while (*p < '0' || *p > '9')
        p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}

int getValue()
{ // Note: this value is in KB!
    FILE *file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

void init()
{
    FILE *file;
    struct tms timeSample;
    char line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    file = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "processor", 9) == 0)
            numProcessors++;
    }
    fclose(file);
}

double getCurrentValue()
{
    struct tms timeSample;
    clock_t now;
    double percent;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU)
    {
        // Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else
    {
        percent = (timeSample.tms_stime - lastSysCPU) +
                  (timeSample.tms_utime - lastUserCPU);
        percent /= (now - lastCPU);
        percent /= numProcessors;
        percent *= 100;
    }
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return percent;
}

// Define actions
enum Action
{
    PLACE_ORDER,
    CANCEL_ORDER,
    MODIFY_ORDER,
    GET_ORDERBOOK,
    GET_POSITIONS,
    VIEW_OPEN_ORDERS,
    VIEW_TRADE_HISTORY,
    CONNECT,
    EXIT
};

// Convert user input to action
Action parseAction(const std::string &input)
{
    static std::unordered_map<std::string, Action> actionMap = {
        {"place", PLACE_ORDER},
        {"cancel", CANCEL_ORDER},
        {"modify", MODIFY_ORDER},
        {"orderbook", GET_ORDERBOOK},
        {"positions", GET_POSITIONS},
        {"open_orders", VIEW_OPEN_ORDERS},
        {"trade_history", VIEW_TRADE_HISTORY},
        {"connect", CONNECT},
        {"exit", EXIT}};
    return actionMap.count(input) ? actionMap[input] : EXIT;
}

void orderManagementSystem(OrderManager &orderManager)
{
    while (true)
    {
        std::cout << "\nChoose an action:\n";
        std::cout << "1. Place order (type 'place')\n";
        std::cout << "2. Cancel order (type 'cancel')\n";
        std::cout << "3. Modify order (type 'modify')\n";
        std::cout << "4. Get orderbook (type 'orderbook')\n";
        std::cout << "5. View positions (type 'positions')\n";
        std::cout << "6. View open orders (type 'open_orders')\n";
        std::cout << "7. View trade history (type 'trade_history')\n";
        std::cout << "8. Connect to websocket server (type 'connect')\n";
        std::cout << "9. Exit (type 'exit')\n";
        std::cout << "Enter choice: ";

        std::string userInput;
        std::cin >> userInput;

        Action action = parseAction(userInput);
        switch (action)
        {
        case PLACE_ORDER:
        {
            std::string symbol, side, type;
            double amount, price;
            std::cout << "Enter instrument (e.g., BTC-PERPETUAL): ";
            std::cin >> symbol;
            std::cout << "Enter side (buy/sell): ";
            std::cin >> side;
            std::cout << "Enter amount: ";
            std::cin >> amount;
            std::cout << "Enter price: ";
            std::cin >> price;
            std::cout << "Enter type (limit/market): ";
            std::cin >> type;

            auto start = std::chrono::high_resolution_clock::now();
            std::string response = orderManager.placeOrder(symbol, side, amount, price, type);
            auto end = std::chrono::high_resolution_clock::now();

            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            // Check the API response for success
            simdjson::ondemand::parser parser;
            simdjson::ondemand::document jsonResponse = parser.iterate(response);

            try
            {
                if (jsonResponse["result"].error() != simdjson::SUCCESS)
                {
                    std::cerr << "Order placement failed. Response: " << response << "\n";
                }
                else
                {
                    std::cout << "Order placed successfully. Latency: " << latency << " ms\n";
                    std::cout << "Response: " << UtilityNamespace::beautifyJSON(response) << "\n";
                }
            }
            catch (const simdjson::simdjson_error &e)
            {
                std::cerr << "Order placement failed. Response: " << response << "\n";
            }
            break;
        }
        case CANCEL_ORDER:
        {
            std::string orderId;
            std::cout << "Enter order ID to cancel: ";
            std::cin >> orderId;

            auto start = std::chrono::high_resolution_clock::now();
            std::string response = orderManager.cancelOrder(orderId);
            auto end = std::chrono::high_resolution_clock::now();

            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            // Check the API response for success
            simdjson::ondemand::parser parser;
            simdjson::ondemand::document jsonResponse = parser.iterate(response);

            try
            {
                if (jsonResponse["result"].error() != simdjson::SUCCESS)
                {
                    std::cerr << "Order cancellation failed. Response: " << response << "\n";
                }
                else
                {
                    std::cout << "Order canceled successfully. Latency: " << latency << " ms\n";
                    std::cout << "Response: " << UtilityNamespace::beautifyJSON(response) << "\n";
                }
            }
            catch (const simdjson::simdjson_error &e)
            {
                std::cerr << "Order cancellation failed. Response: " << response << "\n";
            }
            break;
        }
        case MODIFY_ORDER:
        {
            std::string orderId;
            double newAmount, newPrice;
            std::cout << "Enter order ID to modify: ";
            std::cin >> orderId;
            std::cout << "Enter new amount: ";
            std::cin >> newAmount;
            std::cout << "Enter new price: ";
            std::cin >> newPrice;

            auto start = std::chrono::high_resolution_clock::now();
            std::string response = orderManager.modifyOrder(orderId, newAmount, newPrice);
            auto end = std::chrono::high_resolution_clock::now();

            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            // Check the API response for success
            simdjson::ondemand::parser parser;
            simdjson::ondemand::document jsonResponse = parser.iterate(response);

            try
            {
                if (jsonResponse["result"].error() != simdjson::SUCCESS)
                {
                    std::cerr << "Order modification failed. Response: " << response << "\n";
                }
                else
                {
                    std::cout << "Order modified successfully. Latency: " << latency << " ms\n";
                    std::cout << "Response: " << UtilityNamespace::beautifyJSON(response) << "\n";
                }
            }
            catch (const simdjson::simdjson_error &e)
            {
                std::cerr << "Order modification failed. Response: " << response << "\n";
            }
            break;
        }
        case GET_ORDERBOOK:
        {
            std::string symbol;
            std::cout << "Enter instrument for orderbook (e.g., BTC-PERPETUAL): ";
            std::cin >> symbol;

            auto start = std::chrono::high_resolution_clock::now();
            std::string response = orderManager.getOrderBook(symbol);
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Market Data processing Latency: " << latency << " ms\n";
            std::cout << "Orderbook: " << response << "\n";
            break;
        }
        case GET_POSITIONS:
        {
            std::string currency;
            std::cout << "Enter currency (e.g., BTC): ";
            std::cin >> currency;

            std::string response = orderManager.getCurrentPositions(currency);
            std::cout << "Positions: " << response << "\n";
            break;
        }
        case VIEW_OPEN_ORDERS:
        {
            std::cout << "Fetching open orders...\n";
            auto start = std::chrono::high_resolution_clock::now();
            std::string response = orderManager.getOpenOrders();
            auto end = std::chrono::high_resolution_clock::now();

            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Open Orders:\n"
                      << response << "\n";
            std::cout << "Latency: " << latency << " ms\n";
            break;
        }
        case VIEW_TRADE_HISTORY:
        {
            std::string currency;
            std::cout << "Enter currency (e.g., BTC): ";
            std::cin >> currency;
            std::cout << "Fetching trade history...\n";
            auto start = std::chrono::high_resolution_clock::now();
            std::string response = orderManager.getTradeHistory(currency);
            auto end = std::chrono::high_resolution_clock::now();

            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Trade History:\n"
                      << response << "\n";
            std::cout << "Latency: " << latency << " ms\n";
            break;
        }
        case CONNECT:
        {
            // Implement subscription and unsubscribe and disconnect feature
            WebSocketClient wsClient;
            try
            {
                std::thread websocketThread(&WebSocketClient::start, &wsClient);
                websocketThread.detach();
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << "\n";
                break;
            }

            std::thread managerThread(&WebSocketClient::manageWebSocket, &wsClient);
            managerThread.join();
            break;
        }
        case EXIT:
        {
            std::cout << "Exiting order management system...\n";
            std::cout << "Memory usage: " << getValue() << "KB\n";
            std::cout << "CPU usage: " << getCurrentValue() << "%\n";
            return;
        }
        default:
            std::cout << "Invalid action. Please try again.\n";
            break;
        }
    }
}

int main()
{
    init(); // Initialize CPU usage tracking

    // examples
    //"USDC_USDT", "buy", 10.0, 350.0,"market"  ->spot
    //"BTC-PERPETUAL", "buy", 10.0, 350.0,"limit" ->future
    //"BTC-20DEC24","buy", 10.0, 250.0,"limit" ->future with expiry
    // ETH-26SEP25-1900-C","buy", 1, 0.7,"limit" ->option
    // spot= STETH_USDC
    // future= BTC-PERPETUAL
    // option= ETH-26SEP25-1900-C
    try
    {
        OrderManager orderManager;
        std::cout << "Enter your account credentials to authenticate\n";
        std::cout << "Enter your Client ID: ";
        std::cin >> API_KEY;
        std::cout << "Enter your Client Secret: ";
        std::cin >> SECRET_KEY;
        access_token = UtilityNamespace::authenticate();
        std::cout << "Authenticated successfully.\nToken: " << access_token << "\n";

        orderManagementSystem(orderManager);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
