#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <regex>

using namespace std;

// Basit SQLi ve XSS tespiti
bool isMalicious(const string& request) {
    vector<string> patterns = {
        "UNION SELECT", "1=1", "<script>", "javascript:", "DROP TABLE"
    };
    for (const auto& pattern : patterns) {
        if (request.find(pattern) != string::npos) {
            return true;
        }
    }
    return false;
}

// Cihaz Parmak İzi Doğrulama
bool hasValidFingerprint(const string& request) {
    if (request.find("X-Device-Fingerprint:") != string::npos) {
        return true;
    }
    return false;
}

void handleClient(int clientSocket) {
    char buffer[4096] = {0};
    read(clientSocket, buffer, 4096);
    string request(buffer);

    string responseBody;
    string statusCode;

    if (isMalicious(request)) {
        cout << "[WAF BLOCKED] Malicious payload detected!" << endl;
        statusCode = "403 Forbidden";
        responseBody = "{\"error\": \"Blocked by IAL WAF - Malicious\"}\n";
    } 
    else if (request.find("/api/qr/verify") != string::npos) {
        if (!hasValidFingerprint(request)) {
            cout << "[WAF BLOCKED] QR Share Attempt Detected (Invalid Fingerprint)!" << endl;
            statusCode = "403 Forbidden";
            responseBody = "{\"error\": \"Bu baglanti baska bir cihazdan kopyalanmis!\"}\n";
        } else {
            cout << "[WAF APPROVED] QR Validated for Fingerprint." << endl;
            statusCode = "200 OK";
            responseBody = "{\"status\": \"QR_VERIFIED_OK\"}\n";
        }
    } 
    else {
        cout << "[WAF APPROVED] Traffic Passed." << endl;
        statusCode = "200 OK";
        responseBody = "{\"status\": \"WAF_PASSED\"}\n";
    }

    string response = "HTTP/1.1 " + statusCode + "\r\n"
                    + "Content-Type: application/json\r\n"
                    + "Content-Length: " + to_string(responseBody.length()) + "\r\n"
                    + "Connection: close\r\n\r\n"
                    + responseBody;

    send(clientSocket, response.c_str(), response.length(), 0);
    close(clientSocket);
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == 0) {
        cerr << "Socket failed!" << endl;
        return -1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(getenv("PORT") ? atoi(getenv("PORT")) : 8080);

    if (::bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Bind failed!" << endl;
        return -1;
    }

    if (listen(serverSocket, 10) < 0) {
        cerr << "Listen failed!" << endl;
        return -1;
    }

    cout << "=========================================" << endl;
    cout << " IAL WAF Core (C++) Started on Port " << (getenv("PORT") ? getenv("PORT") : "8080") << " " << endl;
    cout << "=========================================" << endl;

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            thread(handleClient, clientSocket).detach();
        }
    }

    return 0;
}
