#pragma once
#include <string>
#include <future>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class NezhaAPI {
public:
    NezhaAPI(const std::string& dashboardUrl,
        const std::string& username,
        const std::string& password);

    ~NezhaAPI();

    std::future<json> getServer(int id);
    std::future<json> getServers();
	bool  testConnection();

private:
    std::string baseUrl;
    std::string username;
    std::string password;
    std::string token;
    std::mutex mtx;

    static std::string trimSlash(const std::string& url);
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

    json httpRequest(const std::string& method,
        const std::string& url,
        const std::string& body = "",
        const std::string& auth = "");

    void authenticate();

    std::future<json> requestAsync(const std::string& method, const std::string& endpoint);
};
