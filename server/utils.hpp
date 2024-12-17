#pragma once

#include <string>
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <functional>
#include <unordered_map>

namespace UtilityNamespace {
    std::string getInstrumentOrderbook(const std::string& instrumentName);
    std::string sendGetRequest(const std::string& url);
}