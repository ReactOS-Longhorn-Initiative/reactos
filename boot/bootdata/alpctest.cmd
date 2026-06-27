@echo off
rem Run the ALPC apitests through the standard ReactOS rosautotest runner.
rem rosautotest matches the module by the "<module>_*.exe" pattern, so the module
rem name is "alpc" (matches alpc_apitest.exe). It enumerates each test via
rem "--list" and runs them; its StringOut mirrors all output to the kernel debug
rem log (COM1). The run is framed by sentinels so the host harness can extract it.
dbgprint "===ALPC_TEST_BEGIN==="
"%SystemRoot%\system32\rosautotest.exe" alpc
dbgprint "===ALPC_TEST_END==="
