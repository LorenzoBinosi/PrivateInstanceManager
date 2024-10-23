#ifndef API_HPP
#define API_HPP

#include <string>

std::string apiAddService(std::string endpoint, unsigned long api_port, unsigned long service_port);
unsigned long apiGetInfoPort(std::string endpoint, unsigned long api_port, const std::string& uuid_str);
bool apiPing(std::string endpoint, unsigned long api_port);

#endif // API_HPP