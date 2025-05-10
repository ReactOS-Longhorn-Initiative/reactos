Notes from Azzy while she reversed it:
UX.SS
CSessionPort

Shared
CPortBase <- Creates a port 
CPortClient <- handles connecting to a prot

uxsms.dll

CServicePort




-> uxss.exe creates a session Port 
<- uxsms.dll create a CPortClient instacne of the session port.


-> Uxsmsl.dll creates \\UxSmsApiPort
<- Uxss connects to \\UxSmsApiPort


5112
uxss.exe startup
-> Initialize the tracer (We dont need to do this)
-> SetProcessDPIAware
-> open event WinSta0_DesktopSwitch
-> Initialize the Session port (the one uxsms connects to.)
-> connect to\\UxSmsApiPort
-> send thicc sync lpc request.
-> Create dwm invisible window.
