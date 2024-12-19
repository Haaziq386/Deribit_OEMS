#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <shared_mutex>
#include "threadpool.hpp"
typedef websocketpp::server<websocketpp::config::asio> server;

class WebSocketServer
{
public:
    WebSocketServer();
    void startServer(uint16_t port);
    void stopServer();
    void sendOrderbookUpdate();

private:
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);

    server m_server;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> m_connections;
    std::unordered_map<std::string, std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>> m_subscribers;
    std::thread m_serverThread;
    ThreadPool threadPool;
    std::shared_mutex m_subscribersMutex;

};