#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>
#define CURL_STATICLIB
#include "curl/curl.h"

/* Windows Specific Additional Dependencies */
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")



static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


CURL* initializeCurl() {
    CURL* curl = curl_easy_init();
    return curl;
}

std::string getValue(const std::string& response, std::string key)
{
    std::string value;
    size_t start = response.find(key);
    if (start != std::string::npos)
    {
        start += key.length();
        size_t end = response.find("\"", start);
        if (end != std::string::npos)
        {
            value = response.substr(start, end - start);
        }
    }

    if (value.back() == ',')
    {
        value.pop_back();
    }
    return value;
}

void getLocalFiles()
{
    namespace fs = std::filesystem;
    std::string path = "E:\\testProgress";

    struct stat sb;

    for (const auto& entry : fs::directory_iterator(path)) 
    {

        std::filesystem::path outfilename = entry.path();
        std::string outfilename_str = outfilename.string();
        const char* path = outfilename_str.c_str();

        if (stat(path, &sb) == 0 && !(sb.st_mode & S_IFDIR))
        {
            std::cout << path << std::endl;
        }
    }
}

std::string getAccessToken(CURL* curl, const std::string& username, const std::string& password) {
    CURLcode res;
    std::string readBuffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://testserver.moveitcloud.com/api/v1/token");
        std::string postfields = "grant_type=password&username=" + username + "&password=" + password;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    return getValue(readBuffer, "\"access_token\":\"");
}

std::string getFolderID(CURL* curl, const std::string& token) {
    CURLcode res;
    std::string readBuffer;
    struct curl_slist* headers = nullptr;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://testserver.moveitcloud.com/api/v1/users/self");
        std::string tokenRequest = "Authorization: Bearer " + token;
        headers = curl_slist_append(headers, tokenRequest.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    return getValue(readBuffer, "\"homeFolderID\":");
}

void uploadFile(CURL* curl, const std::string& folderID, const std::string& filePath) {
    CURLcode res;
    struct curl_slist* headers = nullptr;
    curl_mime* form = nullptr;
    curl_mimepart* field = nullptr;

    if (curl) {
        std::string url = "https://testserver.moveitcloud.com/api/v1/folders/" + folderID + "/files";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        headers = curl_slist_append(headers, "accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        form = curl_mime_init(curl);
        field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, filePath.c_str());
        curl_mime_type(field, "image/png");
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_mime_free(form);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    std::string username;
    std::string password;
    std::string filePath;

    std::cout << "Please enter your username:" << std::endl;
    std::cin >> username;
    std::cout << "Please enter your password:" << std::endl;
    std::cin >> password;
    std::cout << "Please enter the path to the local file you want to upload." << std::endl;
    std::cout << "Instead of using a single slash, please use double slash when separating the folders:" << std::endl;
    std::cin >> filePath;

    CURL* curl = initializeCurl();
    std::string token = getAccessToken(curl, username, password);

    curl = initializeCurl();
    std::string folderID = getFolderID(curl, token);
    std::cout << "Your folder id is: " << folderID << std::endl;

    curl = initializeCurl();
    uploadFile(curl, folderID, filePath);

    curl_global_cleanup();
    return 0;

}
