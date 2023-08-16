#include "HttpClientCurl.hpp"

HTTPClientCurl::HTTPClientCurl()
{
    curl_easy_setopt(GetContext(), CURLOPT_SSLVERSION, CURL_SSLVERSION_MAX_TLSv1_2);
    curl_easy_setopt(GetContext(), CURLOPT_USERAGENT, "AutoUpgrader/1.0");
    curl_easy_setopt(GetContext(), CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(GetContext(), CURLOPT_FOLLOWLOCATION, 1);
}

bool HTTPClientCurl::Get(const std::string& url, std::string& response)
{
    curl_easy_setopt(GetContext(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(GetContext(), CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(GetContext(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(GetContext(), CURLOPT_CUSTOMREQUEST, "GET");

    return PerformCurlRequest();
}

bool HTTPClientCurl::Post(const std::string& url, std::string& fields, std::string& response)
{
    curl_easy_setopt(GetContext(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(GetContext(), CURLOPT_POST, 1L);
    curl_easy_setopt(GetContext(), CURLOPT_POSTFIELDS, fields.c_str());

    return PerformCurlRequest();
}

bool HTTPClientCurl::Download(const std::string& url, FILE* data)
{
    curl_slist_append(headers, "Accept:application/octet-stream");

    curl_easy_setopt(GetContext(), CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(GetContext(), CURLOPT_WRITEDATA, data);
    curl_easy_setopt(GetContext(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(GetContext(), CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(GetContext(), CURLOPT_HTTPHEADER, headers);

    return PerformCurlRequest();
}

bool HTTPClientCurl::PerformCurlRequest()
{
    if (CURLcode res = curl_easy_perform(GetContext()); res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    return true;
}
