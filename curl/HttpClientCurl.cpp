#include "HttpClientCurl.hpp"

HTTPClientCurl::HTTPClientCurl()
{
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_MAX_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "AutoUpgrader/1.0");
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
}

bool HTTPClientCurl::Get(const std::string& url, std::string& response)
{
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");

    return PerformCurlRequest();
}

bool HTTPClientCurl::Post(const std::string& url, std::string& fields, std::string& response)
{
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());

    return PerformCurlRequest();
}

bool HTTPClientCurl::Download(const std::string& url, FILE* data)
{
    curl_slist_append(headers, "Accept:application/octet-stream");

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    return PerformCurlRequest();
}

bool HTTPClientCurl::PerformCurlRequest()
{
    if (CURLcode res = curl_easy_perform(curl); res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    return true;
}
