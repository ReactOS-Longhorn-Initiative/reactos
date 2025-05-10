#ifndef DWM_APP_LPC_HPP
#define DWM_APP_LPC_HPP

#include "uxsms.h"

class DwmAppLpc
{
public:
    DwmAppLpc();
    ~DwmAppLpc();

    // Initializes the LPC connection
    bool Initialize(const std::wstring& portName);

    // Sends a message through LPC
    bool SendMessage(const void* message, size_t messageSize);

    // Receives a message through LPC
    bool ReceiveMessage(void* buffer, size_t bufferSize, size_t& bytesRead);

    // Closes the LPC connection
    void Close();

private:
    HANDLE m_portHandle; // Handle to the LPC port
};

#endif // DWM_APP_LPC_HPP