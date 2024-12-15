#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <mutex>
#include "order_manager.hpp"
#include "utils.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

class WebSocketServer {
public:
    WebSocketServer();
    void startServer(uint16_t port);
    void stopServer();
    void sendOrderbookUpdate(const std::string& symbol, const nlohmann::json& orderbook);

private:
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);

    server m_server;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> m_connections;
    std::unordered_map<std::string, std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>> m_subscribers;
    std::thread m_serverThread;
    std::mutex m_mutex; // Mutex for thread safety
};

// Implementation of the WebSocketServer methods

WebSocketServer::WebSocketServer() {
    m_server.init_asio();
    m_server.set_open_handler(bind(&WebSocketServer::onOpen, this, std::placeholders::_1));
    m_server.set_close_handler(bind(&WebSocketServer::onClose, this, std::placeholders::_1));
    m_server.set_message_handler(bind(&WebSocketServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
}

void WebSocketServer::startServer(uint16_t port) {
    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
}

void WebSocketServer::stopServer() {
    m_server.stop();
}

void WebSocketServer::sendOrderbookUpdate(const std::string& , const nlohmann::json& ) {
    std::lock_guard<std::mutex> lock(m_mutex); // Lock for thread safety

    // Iterate over all subscribers
    for (const auto& it : m_subscribers) {
        const std::string& symbol = it.first;
        std::string orderbook = UtilityNamespace::getInstrumentOrderbook(symbol);
        const nlohmann::json orderbookJson = nlohmann::json::parse(orderbook);
        std::string message = orderbookJson.dump();
        for (const auto& hdl : it.second) {
            m_server.send(hdl, message, websocketpp::frame::opcode::text);
        }
    }
}

void WebSocketServer::onOpen(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex); // Lock for thread safety
    m_connections.insert(hdl);
    std::cout << "Client connected." << std::endl;
}

void WebSocketServer::onClose(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex); // Lock for thread safety
    m_connections.erase(hdl);
    // Remove the connection from all subscriptions
    for (auto& pair : m_subscribers) {
        pair.second.erase(hdl);
    }
    std::cout << "Client disconnected." << std::endl;
}

void WebSocketServer::onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    auto payload = msg->get_payload();
    auto json = nlohmann::json::parse(payload);

    if (json.contains("action") && json["action"] == "subscribe" && json.contains("symbol")) {
        std::string symbol = json["symbol"];
        std::lock_guard<std::mutex> lock(m_mutex); // Lock for thread safety
        m_subscribers[symbol].insert(hdl);
        std::cout << "Client subscribed to: " << symbol << std::endl;
    }
}