#include "utils.hpp"
namespace UtilityNamespace
{

    std::string getInstrumentOrderbook(const std::string &instrumentName)
    {
        std::string url = "https://test.deribit.com/api/v2/public/get_order_book?instrument_name=" + instrumentName;
        return sendGetRequest(url); // Sends a GET request to retrieve the order book for the specified instrument
    }
    // callback func to write data received from cURL to a string
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
    {
        size_t newLength = size * nmemb;
        s->append((char *)contents, newLength);
        return newLength;
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
}