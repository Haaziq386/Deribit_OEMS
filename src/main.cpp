#include "order_manager.hpp"
#include "utils.hpp"

std::vector<std::string> subscribedSymbols;
std::atomic<bool> running(true); // allow safe updates from multiple threads without direct memory access management by user

void startWebSocketClient()
{
    websocketpp::client<websocketpp::config::asio> client;
    client.init_asio();

    client.set_message_handler(
        [](websocketpp::connection_hdl, websocketpp::config::asio::message_type::ptr msg)
        {
            try
            {
                nlohmann::json orderbookData = nlohmann::json::parse(msg->get_payload());
                // std::cout << "Received orderbook update:\n" << orderbookData.dump(4) << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
            }
        });

    websocketpp::lib::error_code ec;
    std::string uri = "ws://localhost:9002";
    websocketpp::client<websocketpp::config::asio>::connection_ptr con = client.get_connection(uri, ec);

    if (ec)
    {
        std::cerr << "Connection error: " << ec.message() << std::endl;
        return;
    }

    // Send subscription request after connecting
    con->set_open_handler([&client](websocketpp::connection_hdl hdl) {});

    client.connect(con);
    client.run();
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
            auto jsonResponse = nlohmann::json::parse(response, nullptr, false);
            if (jsonResponse.is_discarded() || !jsonResponse.contains("result"))
            {
                std::cerr << "Order placement failed. Response: " << jsonResponse.dump(4) << "\n";
            }
            else
            {
                std::cout << "Order placed successfully. Latency: " << latency << " ms\n";
                std::cout << "Response: " << jsonResponse.dump(4) << "\n";
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
            auto jsonResponse = nlohmann::json::parse(response, nullptr, false);
            if (jsonResponse.is_discarded() || !jsonResponse.contains("result"))
            {
                std::cerr << "Order cancellation failed. Response: " << jsonResponse.dump(4) << "\n";
            }
            else
            {
                std::cout << "Order canceled successfully. Latency: " << latency << " ms\n";
                std::cout << "Response: " << jsonResponse.dump(4) << "\n";
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
            auto jsonResponse = nlohmann::json::parse(response, nullptr, false);
            if (jsonResponse.is_discarded() || !jsonResponse.contains("result"))
            {
                std::cerr << "Order modification failed. Response: " << jsonResponse.dump(4) << "\n";
            }
            else
            {
                std::cout << "Order modified successfully. Latency: " << latency << " ms\n";
                std::cout << "Response: " << jsonResponse.dump(4) << "\n";
            }
            break;
        }
        case GET_ORDERBOOK:
        {
            std::string symbol;
            std::cout << "Enter instrument for orderbook (e.g., BTC-PERPETUAL): ";
            std::cin >> symbol;

            std::string response = orderManager.getOrderBook(symbol);
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
            startWebSocketClient;
            // std::string symbol;
            // std::cout << "Enter the symbol to subscribe (e.g., BTC-PERPETUAL): ";
            // std::cin >> symbol;
            // handleSubscription(symbol, true); // Subscribe
            break;
        }
        case EXIT:
        {
            std::cout << "Exiting order management system...\n";
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
    // examples
    //"USDC_USDT", "buy", 10.0, 350.0,"market"
    //"BTC-PERPETUAL", "buy", 10.0, 350.0,"limit"
    // spot= STETH_USDC
    // future= BTC-PERPETUAL
    // option= ETH-26SEP25-19000-C
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
