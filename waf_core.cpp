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
    // Normalde burada asimetrik şifreleme ile Device ID çözülür.
    // Şimdilik header içinde "X-Device-Fingerprint" arıyoruz.
    if (request.find("X-Device-Fingerprint:") != string::npos) {
        return true;
    }
    return false;
}

void handleClient(int clientSocket) {
    char buffer[4096] = {0};
    read(clientSocket, buffer, 4096);
    string request(buffer);

    string response;

    if (isMalicious(request)) {
        cout << "[WAF BLOCKED] Malicious payload detected!" << endl;
        response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 46\r\n\r\n{\"error\": \"Blocked by IAL WAF - Malicious\"}\n";
    } 
    else if (request.find("/api/qr/verify") != string::npos) {
        // QR Doğrulama Endpointi
        if (!hasValidFingerprint(request)) {
            cout << "[WAF BLOCKED] QR Share Attempt Detected (Invalid Fingerprint)!" << endl;
            response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 64\r\n\r\n{\"error\": \"Bu baglanti baska bir cihazdan kopyalanmis!\"}\n";
        } else {
            cout << "[WAF APPROVED] QR Validated for Fingerprint." << endl;
            response = "HTTP/1.1 200 OK\r\nContent-Length: 30\r\n\r\n{\"status\": \"QR_VERIFIED_OK\"}\n";
        }
    } 
    else {
        // Normal trafik
        cout << "[WAF APPROVED] Traffic Passed." << endl;
        response = "HTTP/1.1 200 OK\r\nContent-Length: 26\r\n\r\n{\"status\": \"WAF_PASSED\"}\n";
    }

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
    address.sin_port = htons(getenv("PORT") ? atoi(getenv("PORT")) : 8080); // WAF 8080 portunda çalışır

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
