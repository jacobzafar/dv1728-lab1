#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <regex>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
    typedef int socklen_t;
    typedef int ssize_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <errno.h>
#endif

#include "protocol.h"
#include "calcLib.h"

// Debug macro - can be enabled with -DDEBUG during compilation
#ifdef DEBUG
    #define DEBUG_PRINT(x) std::cout << x << std::endl
#else
    #define DEBUG_PRINT(x)
#endif

// Structure to hold parsed URL information
struct URLInfo {
    std::string protocol;  // TCP, UDP, or ANY
    std::string host;
    int port;
    std::string api;       // text or binary
};

// Function prototypes
bool parseURL(const std::string& url, URLInfo& info);
int connectTCP(const std::string& host, int port);
int createUDPSocket(const std::string& host, int port, struct sockaddr_in& server_addr);
bool handleTCPText(int sockfd, const std::string& host, int port);
bool handleTCPBinary(int sockfd, const std::string& host, int port);
bool handleUDPText(int sockfd, const struct sockaddr_in& server_addr, const std::string& host, int port);
bool handleUDPBinary(int sockfd, const struct sockaddr_in& server_addr, const std::string& host, int port);
void printError(const std::string& message);
std::string toLowerCase(const std::string& str);
uint16_t hton16(uint16_t value);
uint32_t hton32(uint32_t value);
uint16_t ntoh16(uint16_t value);
uint32_t ntoh32(uint32_t value);

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printError("WSAStartup failed");
        return EXIT_FAILURE;
    }
#endif

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " PROTOCOL://server:port/api" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return EXIT_FAILURE;
    }

    URLInfo url_info;
    if (!parseURL(argv[1], url_info)) {
        printError("Invalid URL format");
        return EXIT_FAILURE;
    }

    std::cout << "Protocol: " << toLowerCase(url_info.protocol) 
              << ", Host " << url_info.host 
              << ", port " << url_info.port 
              << " and path " << toLowerCase(url_info.api) << "." << std::endl;

    bool success = false;
    std::string successful_protocol;

    // Handle different protocol combinations
    if (toLowerCase(url_info.protocol) == "tcp") {
        int sockfd = connectTCP(url_info.host, url_info.port);
        if (sockfd < 0) {
            printError("CANT CONNECT TO " + url_info.host);
            return EXIT_FAILURE;
        }

        DEBUG_PRINT("Connected to  " << url_info.host << ":" << url_info.port);

        if (toLowerCase(url_info.api) == "text") {
            success = handleTCPText(sockfd, url_info.host, url_info.port);
        } else if (toLowerCase(url_info.api) == "binary") {
            success = handleTCPBinary(sockfd, url_info.host, url_info.port);
        }

        close(sockfd);
        successful_protocol = "TCP";
    } 
    else if (toLowerCase(url_info.protocol) == "udp") {
        struct sockaddr_in server_addr;
        int sockfd = createUDPSocket(url_info.host, url_info.port, server_addr);
        if (sockfd < 0) {
            printError("CANT CONNECT TO " + url_info.host);
            return EXIT_FAILURE;
        }

        if (toLowerCase(url_info.api) == "text") {
            success = handleUDPText(sockfd, server_addr, url_info.host, url_info.port);
        } else if (toLowerCase(url_info.api) == "binary") {
            success = handleUDPBinary(sockfd, server_addr, url_info.host, url_info.port);
        }

        close(sockfd);
        successful_protocol = "UDP";
    }
    else if (toLowerCase(url_info.protocol) == "any") {
        // Try UDP first
        struct sockaddr_in server_addr;
        int udp_sockfd = createUDPSocket(url_info.host, url_info.port, server_addr);
        if (udp_sockfd >= 0) {
            if (toLowerCase(url_info.api) == "text") {
                success = handleUDPText(udp_sockfd, server_addr, url_info.host, url_info.port);
            } else if (toLowerCase(url_info.api) == "binary") {
                success = handleUDPBinary(udp_sockfd, server_addr, url_info.host, url_info.port);
            }
            close(udp_sockfd);
            
            if (success) {
                successful_protocol = "UDP";
            }
        }

        // If UDP failed, try TCP
        if (!success) {
            int tcp_sockfd = connectTCP(url_info.host, url_info.port);
            if (tcp_sockfd >= 0) {
                if (toLowerCase(url_info.api) == "text") {
                    success = handleTCPText(tcp_sockfd, url_info.host, url_info.port);
                } else if (toLowerCase(url_info.api) == "binary") {
                    success = handleTCPBinary(tcp_sockfd, url_info.host, url_info.port);
                }
                close(tcp_sockfd);
                
                if (success) {
                    successful_protocol = "TCP";
                }
            }
        }

        if (!success) {
            printError("CANT CONNECT TO " + url_info.host);
            return EXIT_FAILURE;
        }
        
        // Report which protocol was successfully used for ANY mode
        std::cout << "Successfully connected using " << successful_protocol << std::endl;
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool parseURL(const std::string& url, URLInfo& info) {
    // Regular expression to parse PROTOCOL://host:port/api
    std::regex url_regex(R"(^(TCP|UDP|ANY|tcp|udp|any)://([^:/]+):(\d+)/(TEXT|BINARY|text|binary)$)", std::regex_constants::icase);
    std::smatch matches;
    
    if (std::regex_match(url, matches, url_regex)) {
        info.protocol = matches[1].str();
        info.host = matches[2].str();
        info.port = std::stoi(matches[3].str());
        info.api = matches[4].str();
        return true;
    }
    
    return false;
}

int connectTCP(const std::string& host, int port) {
    struct addrinfo hints, *res;
    int sockfd;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (status != 0) {
        printError("RESOLVE ISSUE");
        return -1;
    }
    
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        return -1;
    }
    
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }
    
    freeaddrinfo(res);
    return sockfd;
}

int createUDPSocket(const std::string& host, int port, struct sockaddr_in& server_addr) {
    struct addrinfo hints, *res;
    int sockfd;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4 for UDP
    hints.ai_socktype = SOCK_DGRAM;
    
    int status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (status != 0) {
        printError("RESOLVE ISSUE");
        return -1;
    }
    
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        return -1;
    }
    
    memcpy(&server_addr, res->ai_addr, sizeof(server_addr));
    freeaddrinfo(res);
    return sockfd;
}

bool handleTCPText(int sockfd, const std::string& host, int port) {
    char buffer[1024];
    std::string protocol_response;
    
    // Read protocol negotiation from server
    ssize_t bytes_read;
    while ((bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        protocol_response += buffer;
        
        // Check if we've received the complete protocol list (ends with empty line)
        if (protocol_response.find("\n\n") != std::string::npos) {
            break;
        }
    }
    
    if (bytes_read <= 0) {
        printError("Failed to receive protocol information");
        return false;
    }
    
    // Check if server supports TEXT TCP 1.1
    if (protocol_response.find("TEXT TCP 1.1\n") == std::string::npos) {
        printError("MISSMATCH PROTOCOL");
        return false;
    }
    
    // Send protocol acceptance
    std::string accept_msg = "TEXT TCP 1.1 OK\n";
    if (send(sockfd, accept_msg.c_str(), accept_msg.length(), 0) < 0) {
        printError("Failed to send protocol acceptance");
        return false;
    }
    
    // Read arithmetic assignment
    bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        printError("Failed to receive assignment");
        return false;
    }
    
    buffer[bytes_read] = '\0';
    std::string assignment(buffer);
    
    // Remove trailing newline
    if (!assignment.empty() && assignment.back() == '\n') {
        assignment.pop_back();
    }
    
    std::cout << "ASSIGNMENT: " << assignment << std::endl;
    
    // Parse assignment (format: "operation value1 value2")
    std::istringstream iss(assignment);
    std::string operation;
    int value1, value2;
    
    if (!(iss >> operation >> value1 >> value2)) {
        printError("Invalid assignment format");
        return false;
    }
    
    // Calculate result
    uint32_t op_code = string_to_operation(operation.c_str());
    int32_t result = calculate(op_code, value1, value2);
    
    DEBUG_PRINT("Calculated the result to " << result);
    
    // Send result
    std::string result_str = std::to_string(result) + "\n";
    if (send(sockfd, result_str.c_str(), result_str.length(), 0) < 0) {
        printError("Failed to send result");
        return false;
    }
    
    // Read server response
    bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        printError("Failed to receive server response");
        return false;
    }
    
    buffer[bytes_read] = '\0';
    std::string response(buffer);
    
    // Remove trailing newline
    if (!response.empty() && response.back() == '\n') {
        response.pop_back();
    }
    
    if (response == "OK") {
        std::cout << "OK (myresult=" << result << ")" << std::endl;
        return true;
    } else {
        std::cout << "ERROR (myresult=" << result << ")" << std::endl;
        return false;
    }
}

bool handleTCPBinary(int sockfd, const std::string& host, int port) {
    char buffer[1024];
    std::string protocol_response;
    
    // Read protocol negotiation from server
    ssize_t bytes_read;
    while ((bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        protocol_response += buffer;
        
        // Check if we've received the complete protocol list (ends with empty line)
        if (protocol_response.find("\n\n") != std::string::npos) {
            break;
        }
    }
    
    if (bytes_read <= 0) {
        printError("Failed to receive protocol information");
        return false;
    }
    
    // Check if server supports BINARY TCP 1.1
    if (protocol_response.find("BINARY TCP 1.1\n") == std::string::npos) {
        printError("MISSMATCH PROTOCOL");
        return false;
    }
    
    // Send protocol acceptance
    std::string accept_msg = "BINARY TCP 1.1 OK\n";
    if (send(sockfd, accept_msg.c_str(), accept_msg.length(), 0) < 0) {
        printError("Failed to send protocol acceptance");
        return false;
    }
    
    // Read calcProtocol message
    calcProtocol calc_msg;
    bytes_read = recv(sockfd, &calc_msg, sizeof(calc_msg), 0);
    if (bytes_read != sizeof(calc_msg)) {
        printError("WRONG SIZE OR INCORRECT PROTOCOL");
        return false;
    }
    
    // Convert from network byte order
    calc_msg.type = ntoh16(calc_msg.type);
    calc_msg.major_version = ntoh16(calc_msg.major_version);
    calc_msg.minor_version = ntoh16(calc_msg.minor_version);
    calc_msg.id = ntoh32(calc_msg.id);
    calc_msg.arith = ntoh32(calc_msg.arith);
    calc_msg.inValue1 = ntoh32(calc_msg.inValue1);
    calc_msg.inValue2 = ntoh32(calc_msg.inValue2);
    
    // Check message type and version
    if (calc_msg.type != MSG_TYPE_CALC_PROTOCOL || 
        calc_msg.major_version != MAJOR_VERSION || 
        calc_msg.minor_version != MINOR_VERSION) {
        printError("WRONG SIZE OR INCORRECT PROTOCOL");
        return false;
    }
    
    // Display assignment
    std::cout << "ASSIGNMENT: " << operation_to_string(calc_msg.arith) 
              << " " << calc_msg.inValue1 << " " << calc_msg.inValue2 << std::endl;
    
    // Calculate result
    int32_t result = calculate(calc_msg.arith, calc_msg.inValue1, calc_msg.inValue2);
    DEBUG_PRINT("Calculated the result to " << result);
    
    // Fill in result and convert to network byte order
    calc_msg.inResult = hton32(result);
    calc_msg.type = hton16(calc_msg.type);
    calc_msg.major_version = hton16(calc_msg.major_version);
    calc_msg.minor_version = hton16(calc_msg.minor_version);
    calc_msg.id = hton32(calc_msg.id);
    calc_msg.arith = hton32(calc_msg.arith);
    calc_msg.inValue1 = hton32(calc_msg.inValue1);
    calc_msg.inValue2 = hton32(calc_msg.inValue2);
    
    // Send response
    if (send(sockfd, &calc_msg, sizeof(calc_msg), 0) < 0) {
        printError("Failed to send result");
        return false;
    }
    
    // Read server response
    calcMessage response;
    bytes_read = recv(sockfd, &response, sizeof(response), 0);
    if (bytes_read != sizeof(response)) {
        printError("WRONG SIZE OR INCORRECT PROTOCOL");
        return false;
    }
    
    // Convert from network byte order
    response.type = ntoh16(response.type);
    response.message = ntoh16(response.message);
    response.protocol = ntoh16(response.protocol);
    response.major_version = ntoh16(response.major_version);
    response.minor_version = ntoh16(response.minor_version);
    
    if (response.type == MSG_TYPE_CALC_MESSAGE) {
        if (response.message == 1) { // OK
            std::cout << "OK (myresult=" << result << ")" << std::endl;
            return true;
        } else if (response.message == 2) { // NOT OK
            printError("Server sent NOT OK message");
            return false;
        }
    }
    
    printError("Invalid server response");
    return false;
}

bool handleUDPText(int sockfd, const struct sockaddr_in& server_addr, const std::string& host, int port) {
    // Send initial message
    std::string init_msg = "TEXT UDP 1.1\n";
    if (sendto(sockfd, init_msg.c_str(), init_msg.length(), 0, 
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printError("Failed to send initial message");
        return false;
    }
    
    // Set timeout for UDP communication
#ifdef _WIN32
    DWORD timeout = 2000; // 2 seconds in milliseconds
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
#else
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
#endif
        printError("Failed to set socket timeout");
        return false;
    }
    
    // Receive assignment
    char buffer[1024];
    socklen_t addr_len = sizeof(server_addr);
    ssize_t bytes_read = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                  nullptr, nullptr);
    
    if (bytes_read < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive assignment");
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive assignment");
        }
#endif
        return false;
    }
    
    buffer[bytes_read] = '\0';
    std::string assignment(buffer);
    
    // Remove trailing newline
    if (!assignment.empty() && assignment.back() == '\n') {
        assignment.pop_back();
    }
    
    std::cout << "ASSIGNMENT: " << assignment << std::endl;
    
    // Parse assignment
    std::istringstream iss(assignment);
    std::string operation;
    int value1, value2;
    
    if (!(iss >> operation >> value1 >> value2)) {
        printError("Invalid assignment format");
        return false;
    }
    
    // Calculate result
    uint32_t op_code = string_to_operation(operation.c_str());
    int32_t result = calculate(op_code, value1, value2);
    
    DEBUG_PRINT("Calculated the result to " << result);
    
    // Send result
    std::string result_str = std::to_string(result) + "\n";
    if (sendto(sockfd, result_str.c_str(), result_str.length(), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printError("Failed to send result");
        return false;
    }
    
    // Receive server response
    bytes_read = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                          nullptr, nullptr);
    
    if (bytes_read < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive server response");
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive server response");
        }
#endif
        return false;
    }
    
    buffer[bytes_read] = '\0';
    std::string response(buffer);
    
    // Remove trailing newline
    if (!response.empty() && response.back() == '\n') {
        response.pop_back();
    }
    
    if (response == "OK") {
        std::cout << "OK (myresult=" << result << ")" << std::endl;
        return true;
    } else {
        std::cout << "ERROR (myresult=" << result << ")" << std::endl;
        return false;
    }
}

bool handleUDPBinary(int sockfd, const struct sockaddr_in& server_addr, const std::string& host, int port) {
    // Create initial calcMessage
    calcMessage init_msg;
    init_msg.type = hton16(MSG_TYPE_CALC_MESSAGE);
    init_msg.message = hton16(0);
    init_msg.protocol = hton16(PROTOCOL_UDP);
    init_msg.major_version = hton16(MAJOR_VERSION);
    init_msg.minor_version = hton16(MINOR_VERSION);
    
    // Send initial message
    if (sendto(sockfd, &init_msg, sizeof(init_msg), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printError("Failed to send initial message");
        return false;
    }
    
    // Set timeout for UDP communication
#ifdef _WIN32
    DWORD timeout = 2000; // 2 seconds in milliseconds
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
#else
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
#endif
        printError("Failed to set socket timeout");
        return false;
    }
    
    // Receive server response
    char buffer[1024];
    ssize_t bytes_read = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                  nullptr, nullptr);
    
    if (bytes_read < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive server response");
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive server response");
        }
#endif
        return false;
    }
    
    // Check if it's a calcMessage (NOT OK response)
    if (bytes_read == sizeof(calcMessage)) {
        calcMessage* msg = (calcMessage*)buffer;
        msg->type = ntoh16(msg->type);
        msg->message = ntoh16(msg->message);
        
        if (msg->type == MSG_TYPE_CALC_MESSAGE && msg->message == 2) {
            printError("Server sent NOT OK message");
            return false;
        }
    }
    
    // Check if it's a calcProtocol message
    if (bytes_read != sizeof(calcProtocol)) {
        printError("WRONG SIZE OR INCORRECT PROTOCOL");
        return false;
    }
    
    calcProtocol* calc_msg = (calcProtocol*)buffer;
    
    // Convert from network byte order
    calc_msg->type = ntoh16(calc_msg->type);
    calc_msg->major_version = ntoh16(calc_msg->major_version);
    calc_msg->minor_version = ntoh16(calc_msg->minor_version);
    calc_msg->id = ntoh32(calc_msg->id);
    calc_msg->arith = ntoh32(calc_msg->arith);
    calc_msg->inValue1 = ntoh32(calc_msg->inValue1);
    calc_msg->inValue2 = ntoh32(calc_msg->inValue2);
    
    // Check message type and version
    if (calc_msg->type != MSG_TYPE_CALC_PROTOCOL ||
        calc_msg->major_version != MAJOR_VERSION ||
        calc_msg->minor_version != MINOR_VERSION) {
        printError("WRONG SIZE OR INCORRECT PROTOCOL");
        return false;
    }
    
    // Display assignment
    std::cout << "ASSIGNMENT: " << operation_to_string(calc_msg->arith)
              << " " << calc_msg->inValue1 << " " << calc_msg->inValue2 << std::endl;
    
    // Calculate result
    int32_t result = calculate(calc_msg->arith, calc_msg->inValue1, calc_msg->inValue2);
    DEBUG_PRINT("Calculated the result to " << result);
    
    // Fill in result and convert to network byte order
    calc_msg->inResult = hton32(result);
    calc_msg->type = hton16(calc_msg->type);
    calc_msg->major_version = hton16(calc_msg->major_version);
    calc_msg->minor_version = hton16(calc_msg->minor_version);
    calc_msg->id = hton32(calc_msg->id);
    calc_msg->arith = hton32(calc_msg->arith);
    calc_msg->inValue1 = hton32(calc_msg->inValue1);
    calc_msg->inValue2 = hton32(calc_msg->inValue2);
    
    // Send response
    if (sendto(sockfd, calc_msg, sizeof(*calc_msg), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printError("Failed to send result");
        return false;
    }
    
    // Receive final server response
    bytes_read = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                          nullptr, nullptr);
    
    if (bytes_read < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive final response");
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printError("MESSAGE LOST (TIMEOUT)");
        } else {
            printError("Failed to receive final response");
        }
#endif
        return false;
    }
    
    if (bytes_read != sizeof(calcMessage)) {
        printError("WRONG SIZE OR INCORRECT PROTOCOL");
        return false;
    }
    
    calcMessage* response = (calcMessage*)buffer;
    response->type = ntoh16(response->type);
    response->message = ntoh16(response->message);
    
    if (response->type == MSG_TYPE_CALC_MESSAGE) {
        if (response->message == 1) { // OK
            std::cout << "OK (myresult=" << result << ")" << std::endl;
            return true;
        } else if (response->message == 2) { // NOT OK
            printError("Server sent NOT OK message");
            return false;
        }
    }
    
    printError("Invalid server response");
    return false;
}

void printError(const std::string& message) {
    std::cerr << "ERROR: " << message << std::endl;
}

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Network byte order conversion functions
uint16_t hton16(uint16_t value) {
    return htons(value);
}

uint32_t hton32(uint32_t value) {
    return htonl(value);
}

uint16_t ntoh16(uint16_t value) {
    return ntohs(value);
}

uint32_t ntoh32(uint32_t value) {
    return ntohl(value);
}
