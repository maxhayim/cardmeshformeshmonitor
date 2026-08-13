#pragma once

#include <string>

namespace cardmesh::api {

struct HttpResponse {
    long statusCode = 0;
    std::string body;
    bool networkError = false;
    std::string errorMessage;

    bool ok() const { return !networkError && statusCode >= 200 && statusCode < 300; }
};

class HttpClient {
public:
    HttpClient();

    void setBaseUrl(const std::string& baseUrl);
    void setBearerToken(const std::string& token);
    void setTimeoutSeconds(long seconds);
    void setVerifyTls(bool verify);

    HttpResponse get(const std::string& path) const;
    HttpResponse post(const std::string& path, const std::string& jsonBody) const;

private:
    HttpResponse perform(const std::string& method, const std::string& path, const std::string& body) const;

    std::string baseUrl_;
    std::string bearerToken_;
    long timeoutSeconds_ = 10;
    bool verifyTls_ = true;
};

}  // namespace cardmesh::api
