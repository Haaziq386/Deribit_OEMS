#include "utils.hpp"

std::string API_KEY, SECRET_KEY, access_token;
namespace UtilityNamespace
{
    std::string authenticate()
    {
        std::string url = "https://test.deribit.com/api/v2/public/auth";
        std::string payload = "{\"jsonrpc\":\"2.0\", \"method\":\"public/auth\", \"params\":{\"grant_type\":\"client_credentials\", \"client_id\":\"" + API_KEY + "\", \"client_secret\":\"" + SECRET_KEY + "\"}, \"id\":1}";

        // POST request to authenticate
        std::string response = sendPostRequest(url, payload);

        // parsing the response to get the access token
        simdjson::ondemand::parser parser;
        simdjson::ondemand::document jsonResponse = parser.iterate(response);

        try
        {
            if (jsonResponse["result"].error() == simdjson::SUCCESS && jsonResponse["result"]["access_token"].error() == simdjson::SUCCESS)
            {
                auto sv = jsonResponse["result"]["access_token"].get_string().value();
                access_token = std::string(sv.data(), sv.length());
                // std::cout << "Access Token: " << access_token << std::endl;
                return access_token;
            }
            else
            {
                std::cerr << "Authentication failed. Response: " << response << std::endl;
                throw std::runtime_error("Authentication failed.");
            }
        }
        catch (const simdjson::simdjson_error &e)
        {
            std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
            throw std::runtime_error("Error parsing JSON response.");
        }
    }

    // callback func to write data received from cURL to a string
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
    {
        size_t newLength = size * nmemb;
        s->append((char *)contents, newLength);
        return newLength;
    }

    std::string sendPostRequestWithAuth(const std::string &url, const std::string &payload, const std::string &authHeader)
    {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl)
        {
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, authHeader.c_str());

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                std::cerr << "cURL Error: " << curl_easy_strerror(res) << std::endl;
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
        return readBuffer;
    }

    // function to perform HTTP POST requests
    std::string sendPostRequest(const std::string &url, const std::string &payload)
    {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl)
        {
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            std::string authHeader = "Authorization: Bearer " + API_KEY;
            headers = curl_slist_append(headers, authHeader.c_str());

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
        return readBuffer;
    }

    // function to perform HTTP GET requests
    std::string sendGetRequest(const std::string &url)
    {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            // callback setup to capture the response
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                std::cerr << "cURL Error: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        return readBuffer;
    }
    // function to get order book
    std::string getOrderBook(const std::string &symbol)
    {
        std::string url = "https://test.deribit.com/api/v2/public/get_order_book?instrument_name=" + symbol;
        return sendGetRequest(url); // Assuming sendGetRequest is a function that sends a GET request and returns the response as a string
    }
    std::string getInstruments()
    {
        std::string url = "https://test.deribit.com/api/v2/public/get_instruments";
        return sendGetRequest(url); // Sends a GET request to retrieve instruments
    }

}