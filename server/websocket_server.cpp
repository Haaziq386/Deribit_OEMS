#include "websocket_server.hpp"
#include "utils.hpp"

// Implementation of the WebSocketServer methods

WebSocketServer::WebSocketServer()
{
    m_server.init_asio();
    m_server.set_open_handler(bind(&WebSocketServer::onOpen, this, std::placeholders::_1));
    m_server.set_close_handler(bind(&WebSocketServer::onClose, this, std::placeholders::_1));
    m_server.set_message_handler(bind(&WebSocketServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
}

void WebSocketServer::startServer(uint16_t port)
{
    std::cout << "started" << std::endl;
    m_server.listen(port);
    m_server.start_accept();
    std::thread server_thread([this]()
                              { m_server.run(); }); // Detach the thread to allow it to run independently
    server_thread.detach();
}

void WebSocketServer::stopServer()
{
    m_server.stop();
}

void WebSocketServer::sendOrderbookUpdate()
{
    // Iterate over all subscribers
    for (const auto &it : m_subscribers)
    {
        const std::string &symbol = it.first;
        std::string orderbook = UtilityNamespace::getInstrumentOrderbook(symbol);
        const nlohmann::json orderbookJson = nlohmann::json::parse(orderbook);
        for (const auto &hdl : it.second)
        {
            auto currentTime = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();

            std::string combinedMessage = nlohmann::json{
                {"symbol", symbol},
                {"data", orderbookJson},
                {"timestamp", timestamp}}.dump();

            m_server.send(hdl, combinedMessage, websocketpp::frame::opcode::text);
        }
    }
}

void WebSocketServer::onOpen(websocketpp::connection_hdl hdl)
{

    m_connections.insert(hdl);
    std::cout << "Client connected." << std::endl;
}

void WebSocketServer::onClose(websocketpp::connection_hdl hdl)
{

    m_connections.erase(hdl);
    // Remove the connection from all subscriptions
    for (auto &pair : m_subscribers)
    {
        pair.second.erase(hdl);
    }
    std::cout << "Client disconnected." << std::endl;
}

void WebSocketServer::onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    auto payload = msg->get_payload();
    auto json = nlohmann::json::parse(payload);

    if (json.contains("action"))
    {
        if (json["action"] == "subscribe" && json.contains("symbol"))
        {
            std::string symbol = json["symbol"];
            m_subscribers[symbol].insert(hdl);
            std::cout << "Client subscribed to: " << symbol << std::endl;
        }
        else if (json["action"] == "unsubscribe" && json.contains("symbol"))
        {
            std::string symbol = json["symbol"];
            m_subscribers[symbol].erase(hdl);
            std::cout << "Client unsubscribed from: " << symbol << std::endl;
        }
        else
        {
            std::cout << "Unknown action received: " << json["action"] << std::endl;
        }
    }
}
int main()
{
    WebSocketServer wsServer;
    try
    {
        wsServer.startServer(9002);
        while (1)
        {
            wsServer.sendOrderbookUpdate();
            sleep(10);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        wsServer.stopServer();
    }
    wsServer.stopServer();

    return 0;
}