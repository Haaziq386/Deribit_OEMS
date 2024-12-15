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
std::vector<std::string> subscribedSymbols;
std::atomic<bool> running(true);//allow safe updates from multiple threads without direct memory access management by user
void fetchAndStreamOrderbook(WebSocketServer& wsServer) {
    while (running) {
        try {
            auto sentTime = std::chrono::high_resolution_clock::now();
            wsServer.sendOrderbookUpdate("none",nlohmann::json::parse(UtilityNamespace::getInstruments()));
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto propagationDelay = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime-sentTime).count();
            std::cout << "WebSocket Message Propagation Delay: " << propagationDelay << " ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            std::cerr << "Error fetching orderbook: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void on_message(websocketpp::client<websocketpp::config::asio>* , websocketpp::connection_hdl , websocketpp::client<websocketpp::config::asio>::message_ptr msg) {
    try {
        // Parse the incoming message (JSON)
        nlohmann::json orderbookData = nlohmann::json::parse(msg->get_payload());
        //std::cout << "Received orderbook update:\n" << orderbookData.dump(4) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}
std::vector<std::string> fetchAllSymbols() {
    std::string response = UtilityNamespace::getInstruments(); // Implement this function to call the Deribit API
    auto jsonResponse = nlohmann::json::parse(response);
    std::vector<std::string> symbols;
    std::vector<std::string> future;
    std::vector<std::string> option;
    // bool flag = false;
    for (const auto& instrument : jsonResponse["result"]) {
        // if(instrument["instrument_name"]=="BTC-PERPETUAL")flag=true;
        // if(!flag)continue;
        if(instrument["kind"]=="future"){
            future.push_back(instrument["instrument_name"]);
            symbols.push_back(instrument["instrument_name"]);
        }
        else if(instrument["kind"]=="option"){
            option.push_back(instrument["instrument_name"]);
            symbols.push_back(instrument["instrument_name"]);
        }
        else symbols.push_back(instrument["instrument_name"]);
        // std::cout<<symbols.back()<<std::endl;
    }
    return symbols;
}
void subscribeToSymbols(websocketpp::client<websocketpp::config::asio>& , websocketpp::connection_hdl ) {
    nlohmann::json subscribeMessage;
    subscribeMessage["jsonrpc"] = "2.0";
    subscribeMessage["id"] = 1;
    subscribeMessage["method"] = "public/subscribe";
    
    nlohmann::json params;
    for (const auto& symbol : subscribedSymbols) {
        params["channels"].push_back("orderbook." + symbol);
    }
    subscribeMessage["params"] = params;

    //client.send(hdl, subscribeMessage.dump(), websocketpp::frame::opcode::text);
}
void startWebSocketClient() {
    websocketpp::client<websocketpp::config::asio> client;
    client.init_asio();

    client.set_message_handler(
        [](websocketpp::connection_hdl , websocketpp::config::asio::message_type::ptr msg) {
            try {
                nlohmann::json orderbookData = nlohmann::json::parse(msg->get_payload());
                //std::cout << "Received orderbook update:\n" << orderbookData.dump(4) << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
            }
        });

    websocketpp::lib::error_code ec;
    std::string uri = "ws://localhost:9002";
    websocketpp::client<websocketpp::config::asio>::connection_ptr con = client.get_connection(uri, ec);

    if (ec) {
        std::cerr << "Connection error: " << ec.message() << std::endl;
        return;
    }

    // Send subscription request after connecting
    con->set_open_handler([&client](websocketpp::connection_hdl hdl) {
        auto start1 = std::chrono::high_resolution_clock::now();
        subscribedSymbols = fetchAllSymbols(); // Fetch all symbols
        subscribeToSymbols(client, hdl); // Subscribe to all symbols
        auto end1 = std::chrono::high_resolution_clock::now();
        auto marketProcessTime= std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
        std::cout<<"Market Data Processing Latency: "<<marketProcessTime<<" ms"<<std::endl;

    });

    client.connect(con);
    client.run();
}

int main() {
    OrderManager orderManager;
    //examples
    //spot= STETH_USDC
    //future= BTC-PERPETUAL
    //option= ETH-26SEP25-19000-C
    try {
        std::string token = UtilityNamespace::authenticate();
        // std::cout << "Token: " << token << std::endl;

        // Place an order
        auto start = std::chrono::high_resolution_clock::now();
        std::string response1 = orderManager.placeOrder("STETH_USDC", "buy", 10.0, 300.0,"market");
        auto end = std::chrono::high_resolution_clock::now();
        auto orderPlacementLatency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Order Placement Latency: " << orderPlacementLatency << " ms" << std::endl;
        auto jsonResponse = nlohmann::json::parse(response1);
        std::string orderId = jsonResponse["result"]["order"]["order_id"];
        std::cout << "Order placed successfully!" << std::endl;
        std::cout << "Order ID: " << orderId << std::endl;
        std::cout << "Instrument: " << jsonResponse["result"]["order"]["instrument_name"] << std::endl;
        // std::cout << "Direction: " << jsonResponse["result"]["order"]["direction"] << std::endl;
        std::cout << "Price: " << jsonResponse["result"]["order"]["price"] << std::endl;
        std::cout << "Amount: " << jsonResponse["result"]["order"]["amount"] << std::endl;
        std::cout << "Order State: " << jsonResponse["result"]["order"]["order_state"] << std::endl;
        // std::cout << "Creation Timestamp: " << jsonResponse["result"]["order"]["creation_timestamp"] << std::endl;
        std::cout << std::endl;

        // Modify an order
        std::string response3 = orderManager.modifyOrder(orderId, 10.0, 400.0);
        auto jsonResponseModify = nlohmann::json::parse(response3);
        std::cout << "Order modified successfully!" << std::endl;
        std::cout << "Order ID: " << jsonResponseModify["result"]["order"]["order_id"] << std::endl;
        std::cout << "New Price: " << jsonResponseModify["result"]["order"]["price"] << std::endl;
        std::cout << "New Amount: " << jsonResponseModify["result"]["order"]["amount"] << std::endl;
        std::cout << "Order State: " << jsonResponseModify["result"]["order"]["order_state"] << std::endl;
        std::cout << std::endl;

        // Get Order Book
        std::string response4 = orderManager.getOrderBook("BTC-PERPETUAL");
        auto jsonResponseOrderBook = nlohmann::json::parse(response4);
        std::cout << "Order Book for " << jsonResponseOrderBook["result"]["instrument_name"] << ":" << std::endl;
        // std::cout << "Timestamp: " << jsonResponseOrderBook["result"]["timestamp"] << std::endl;
        std::cout << "Last Price: " << jsonResponseOrderBook["result"]["last_price"] << std::endl;
        std::cout << "Best Bid Price: " << jsonResponseOrderBook["result"]["best_bid_price"] << " (Amount: " << jsonResponseOrderBook["result"]["best_bid_amount"] << ")" << std::endl;
        std::cout << "Best Ask Price: " << jsonResponseOrderBook["result"]["best_ask_price"] << " (Amount: " << jsonResponseOrderBook["result"]["best_ask_amount"] << ")" << std::endl;

        std::cout << "Bids:" << std::endl;
        for (const auto& bid : jsonResponseOrderBook["result"]["bids"]) {
            std::cout << "Price: " << bid[0] << ", Amount: " << bid[1] << std::endl;
        }

        std::cout << "Asks:" << std::endl;
        for (const auto& ask : jsonResponseOrderBook["result"]["asks"]) {
            std::cout << "Price: " << ask[0] << ", Amount: " << ask[1] << std::endl;
        }
        std::cout << std::endl;

        // Get Current Positions
        std::string currency = "BTC"; 
        std::string response5 = orderManager.getCurrentPositions(currency);
        auto jsonResponsePositions = nlohmann::json::parse(response5);
        std::cout << "Current Positions:" << std::endl;
        for (const auto& position : jsonResponsePositions["result"]) {
            std::cout << "Instrument: " << position["instrument_name"] << std::endl;
            std::cout << "Size: " << position["size"] << std::endl;
            // std::cout << "Direction: " << position["direction"] << std::endl;
            std::cout << "Average Price: " << position["average_price"] << std::endl;
            std::cout << "Floating P&L: " << position["floating_profit_loss"] << std::endl;
            std::cout << "Realized P&L: " << position["realized_profit_loss"] << std::endl;
            std::cout << "Leverage: " << position["leverage"] << std::endl;
            std::cout << std::endl;
        }
        // // cancel order
        // std::string response2 = orderManager.cancelOrder(orderId);
        // auto jsonResponseCancel = nlohmann::json::parse(response2);
        // std::cout << "Order cancelled successfully!" << std::endl;

        // // Start WebSocket server for streaming orderbook
        WebSocketServer wsServer;
        std::thread serverThread([&wsServer]() {
            wsServer.startServer(9002);
        });

        // Start a thread to fetch and stream orderbook updates
        std::thread orderbookThread(fetchAndStreamOrderbook, std::ref(wsServer));

        // Start WebSocket client in a separate thread to connect to the server
        std::thread clientThread(startWebSocketClient);

        // Wait for the orderbook thread to finish (it won't in this example)
        std::cout << "Press Enter to stop..." << std::endl;
        std::cin.get();

        // Signal the orderbook thread to stop gracefully
        running = false;
        orderbookThread.join();  // Wait for the thread to finish
        clientThread.join(); // Wait for the WebSocket client to finish
        serverThread.join(); // Wait for the server thread to finish
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}
