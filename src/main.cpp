#include "order_manager.hpp"
#include "utils.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include "websocket_server.hpp"
#include <chrono>
#include <atomic>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <functional>
#include <unordered_map>

std::vector<std::string> subscribedSymbols;
std::atomic<bool> running(true); // allow safe updates from multiple threads without direct memory access management by user
void fetchAndStreamOrderbook(WebSocketServer &wsServer)
{
    while (running)
    {
        try
        {
            auto sentTime = std::chrono::high_resolution_clock::now();
            wsServer.sendOrderbookUpdate("none", nlohmann::json::parse(UtilityNamespace::getInstruments()));
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto propagationDelay = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime).count();
            std::cout << "WebSocket Message Propagation Delay: " << propagationDelay << " ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error fetching orderbook: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void on_message(websocketpp::client<websocketpp::config::asio> *, websocketpp::connection_hdl, websocketpp::client<websocketpp::config::asio>::message_ptr msg)
{
    try
    {
        // Parse the incoming message (JSON)
        nlohmann::json orderbookData = nlohmann::json::parse(msg->get_payload());
        // std::cout << "Received orderbook update:\n" << orderbookData.dump(4) << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}
std::vector<std::string> fetchAllSymbols()
{
    std::string response = UtilityNamespace::getInstruments(); // Implement this function to call the Deribit API
    auto jsonResponse = nlohmann::json::parse(response);
    std::vector<std::string> symbols;
    std::vector<std::string> future;
    std::vector<std::string> option;
    // bool flag = false;
    for (const auto &instrument : jsonResponse["result"])
    {
        // if(instrument["instrument_name"]=="BTC-PERPETUAL")flag=true;
        // if(!flag)continue;
        if (instrument["kind"] == "future")
        {
            future.push_back(instrument["instrument_name"]);
            symbols.push_back(instrument["instrument_name"]);
        }
        else if (instrument["kind"] == "option")
        {
            option.push_back(instrument["instrument_name"]);
            symbols.push_back(instrument["instrument_name"]);
        }
        else
            symbols.push_back(instrument["instrument_name"]);
        // std::cout<<symbols.back()<<std::endl;
    }
    return symbols;
}
void subscribeToSymbols(websocketpp::client<websocketpp::config::asio> &, websocketpp::connection_hdl)
{
    nlohmann::json subscribeMessage;
    subscribeMessage["jsonrpc"] = "2.0";
    subscribeMessage["id"] = 1;
    subscribeMessage["method"] = "public/subscribe";

    nlohmann::json params;
    for (const auto &symbol : subscribedSymbols)
    {
        params["channels"].push_back("orderbook." + symbol);
    }
    subscribeMessage["params"] = params;

    // client.send(hdl, subscribeMessage.dump(), websocketpp::frame::opcode::text);
}
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
    con->set_open_handler([&client](websocketpp::connection_hdl hdl)
                          {
                              auto start1 = std::chrono::high_resolution_clock::now();
                              subscribedSymbols = fetchAllSymbols(); // Fetch all symbols
                              subscribeToSymbols(client, hdl);       // Subscribe to all symbols
                              auto end1 = std::chrono::high_resolution_clock::now();
                              auto marketProcessTime = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
                              std::cout << "Market Data Processing Latency: " << marketProcessTime << " ms" << std::endl; });

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
        {"open_orders", VIEW_OPEN_ORDERS},     // Map "open_orders" to VIEW_OPEN_ORDERS
        {"trade_history", VIEW_TRADE_HISTORY}, // Map "trade_history" to VIEW_TRADE_HISTORY
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
        std::cout << "8. Exit (type 'exit')\n";
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
