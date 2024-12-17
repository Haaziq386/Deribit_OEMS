#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP

#include <websocketpp/config/asio.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <iostream>
#include <algorithm>

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    void start();
    void subscribe(const std::string &symbol);
    void unsubscribe(const std::string &symbol);
    void disconnect();
    void manageWebSocket();

private:
    void onMessage(websocketpp::connection_hdl, websocketpp::config::asio::message_type::ptr msg);
    void onOpen(websocketpp::connection_hdl hdl);

    websocketpp::client<websocketpp::config::asio> client;
    websocketpp::connection_hdl globalHdl;
    std::vector<std::string> subscribedSymbols;
    std::atomic<bool> running;
    std::mutex symbolMutex;
};

#endif // WEBSOCKET_HPP
