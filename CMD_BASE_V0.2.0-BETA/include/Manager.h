#ifndef MANAGER_H
#define MANAGER_H

#include "SystemCore.h"
#include <string>
#include <windows.h>

class Manager {
private:
    SystemCore &sc;
public:
    Manager(SystemCore &s);
  
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();

};

#endif