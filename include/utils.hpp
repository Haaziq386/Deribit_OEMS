#pragma once

#include <string>
#include <curl/curl.h>
#include <iostream>

extern std::string API_KEY,SECRET_KEY,access_token;//global variables

namespace UtilityNamespace {

    // Function declarations
    std::string sendPostRequest(const std::string& url, const std::string& postFields);
    std::string sendPostRequestWithAuth(const std::string& url, const std::string& postFields, const std::string& token);
    std::string sendGetRequest(const std::string& url);
    std::string authenticate();
    std::string getOrderBook(const std::string& symbol);
    std::string getInstruments();
    std::string getInstrumentOrderbook(const std::string& instrumentName);

    void logMessage(const std::string& message);

}