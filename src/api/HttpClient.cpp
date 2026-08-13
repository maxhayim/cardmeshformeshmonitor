#include "HttpClient.h"

#include <curl/curl.h>

#include <mutex>

namespace cardmesh::api {

namespace {

void ensureCurlGlobalInit() {
    static std::once_flag flag;
    std::call_once(flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

}  // namespace

HttpClient::HttpClient() { ensureCurlGlobalInit(); }

void HttpClient::setBaseUrl(const std::string& baseUrl) { baseUrl_ = baseUrl; }

void HttpClient::setBearerToken(const std::string& token) { bearerToken_ = token; }

void HttpClient::setTimeoutSeconds(long seconds) { timeoutSeconds_ = seconds; }

void HttpClient::setVerifyTls(bool verify) { verifyTls_ = verify; }

HttpResponse HttpClient::get(const std::string& path) const { return perform("GET", path, ""); }

HttpResponse HttpClient::post(const std::string& path, const std::string& jsonBody) const {
    return perform("POST", path, jsonBody);
}

HttpResponse HttpClient::perform(const std::string& method, const std::string& path,
                                  const std::string& body) const {
    HttpResponse response;

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        response.networkError = true;
        response.errorMessage = "failed to initialize curl handle";
        return response;
    }

    const std::string url = baseUrl_ + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds_);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifyTls_ ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyTls_ ? 2L : 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    curl_slist* headers = nullptr;
    if (!bearerToken_.empty()) {
        const std::string authHeader = "Authorization: Bearer " + bearerToken_;
        headers = curl_slist_append(headers, authHeader.c_str());
    }

    if (method == "POST") {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    if (headers != nullptr) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    const CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        response.networkError = true;
        response.errorMessage = curl_easy_strerror(result);
    } else {
        long statusCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        response.statusCode = statusCode;
    }

    if (headers != nullptr) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    return response;
}

}  // namespace cardmesh::api
