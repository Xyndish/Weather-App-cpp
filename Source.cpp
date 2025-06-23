#include <fstream>
#include <sstream>
#include <unordered_map>
#include "httplib.h"
#include "json.hpp"
#include <curl/curl.h>
#include <iostream>

using json = nlohmann::json;
using namespace std;

std::unordered_map<std::string, std::string> readConfig(const std::string& filename) {
    std::unordered_map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, value;

        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config[key] = value;
        }
    }
    return config;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

json getWeatherFromAPI(const string& city, const string& apiKey) {
    CURL* curl = curl_easy_init();
    string response;
    if (!curl) return json({ {"error", "CURL init failed"} });

    string url = "http://api.weatherapi.com/v1/current.json?key=" + apiKey + "&q=" + city + "&aqi=no";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return json({ {"error", curl_easy_strerror(res)} });
    }

    try {
        auto j = json::parse(response);
        if (j.contains("error"))
            return json({ {"error", j["error"]["message"]} });

        return json{
            {"city", j["location"]["name"]},
            {"country", j["location"]["country"]},
            {"temperature", j["current"]["temp_c"]},
            {"condition", j["current"]["condition"]["text"]}
        };
    }
    catch (...) {
        return json({ {"error", "Failed to parse JSON"} });
    }
}

int main() {
    auto config = readConfig("config.txt");

    if (config.find("WEATHER_API_KEY") == config.end()) {
        std::cerr << "API is not found in config.txt" << std::endl;
        return 1;
    }

    const std::string apiKey = config["WEATHER_API_KEY"];
    httplib::Server svr;

    // CORS
    svr.Options("/weather", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
        });

    svr.Get("/weather", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        if (!req.has_param("city")) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing 'city' parameter\"}", "application/json");
            return;
        }

        std::string city = req.get_param_value("city");
        json result = getWeatherFromAPI(city, apiKey);
        res.set_content(result.dump(4), "application/json");
        });

    std::cout << "Server is running on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
