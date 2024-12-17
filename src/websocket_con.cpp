#include "websocket_con.hpp"

WebSocketClient::WebSocketClient() : running(true)
{
    client.init_asio();
    client.set_message_handler(bind(&WebSocketClient::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    client.set_open_handler(bind(&WebSocketClient::onOpen, this, std::placeholders::_1));
}
WebSocketClient::~WebSocketClient()
{
    if (running)
    {
        disconnect();
    }
}
void WebSocketClient::start()
{
    websocketpp::lib::error_code ec;
    std::string uri = "ws://localhost:9002";
    websocketpp::client<websocketpp::config::asio>::connection_ptr con = client.get_connection(uri, ec);

    if (ec)
    {
        std::cerr << "Connection error: " << ec.message() << std::endl;
        return;
    }

    client.connect(con);
    globalHdl = con->get_handle();

    try
    {
        client.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "WebSocket client error: " << e.what() << std::endl;
    }
    std::cout << "Server Stopped" << std::endl;
}

void WebSocketClient::subscribe(const std::string &symbol)
{
    nlohmann::json subscribeMessage = {{"action", "subscribe"}, {"symbol", symbol}};
    client.send(globalHdl, subscribeMessage.dump(), websocketpp::frame::opcode::text);
    std::lock_guard<std::mutex> lock(symbolMutex);
    subscribedSymbols.push_back(symbol);
    std::cout << "Subscribed to: " << symbol << std::endl;
}

void WebSocketClient::unsubscribe(const std::string &symbol)
{
    nlohmann::json unsubscribeMessage = {{"action", "unsubscribe"}, {"symbol", symbol}};
    client.send(globalHdl, unsubscribeMessage.dump(), websocketpp::frame::opcode::text);
    std::lock_guard<std::mutex> lock(symbolMutex);
    subscribedSymbols.erase(std::remove(subscribedSymbols.begin(), subscribedSymbols.end(), symbol), subscribedSymbols.end());
    std::cout << "Unsubscribed from: " << symbol << std::endl;
}

void WebSocketClient::disconnect()
{
    if (!running)
    {
        std::cerr << "Already disconnected." << std::endl;
        return;
    }

    std::cout << "Disconnecting from server..." << std::endl;

    websocketpp::lib::error_code ec;
    client.close(globalHdl, websocketpp::close::status::going_away, "Client disconnecting", ec);

    if (ec)
    {
        std::cerr << "Error during disconnect: " << ec.message() << std::endl;
    }

    // Stop the WebSocket++ event loop
    client.stop();
    running = false;

    // Clear resources
    std::lock_guard<std::mutex> lock(symbolMutex);
    subscribedSymbols.clear();

    std::cout << "Disconnected from server." << std::endl;
}

void WebSocketClient::manageWebSocket()
{
    while (running)
    {
        std::cout << "For subscribing, enter SUB, unsubscribing enter UNSUB, and for disconnecting from the server enter DISC\n";
        std::cout << "Then enter the symbol to subscribe or unsubscribe (e.g., SUB BTC-PERPETUAL)\n";
        std::string action, symbol;
        std::cin >> action;
        if (action == "SUB")
        {
            std::cin >> symbol;
            subscribe(symbol);
        }
        else if (action == "UNSUB")
        {
            std::cin >> symbol;
            unsubscribe(symbol);
        }
        else
        {
            disconnect();
            break;
        }
    }
}

void WebSocketClient::onMessage(websocketpp::connection_hdl, websocketpp::config::asio::message_type::ptr msg)
{
    try
    {
        auto receivedTime = std::chrono::system_clock::now();
        nlohmann::json orderbookData = nlohmann::json::parse(msg->get_payload());
        if (orderbookData.contains("timestamp"))
        {
            auto sentTimestamp = orderbookData["timestamp"].get<long long>();
            auto receivedTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(receivedTime.time_since_epoch()).count();
            auto propagationDelay = receivedTimestamp - sentTimestamp;
            std::cout << "Propagation delay: " << propagationDelay << " ms" << std::endl;
        }
        std::cout << "Received orderbook update:\n"
                  << orderbookData.dump(4) << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}
void WebSocketClient::onOpen(websocketpp::connection_hdl hdl)
{
    globalHdl = hdl;
    std::cout << "Connected to server." << std::endl;
}
