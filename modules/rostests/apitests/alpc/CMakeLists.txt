
list(APPEND SOURCE
    AlpcHelpers.c
    NtAlpcCreatePort.c
    NtAlpcConnectPort.c
    NtAlpcAcceptConnectPort.c
    SendReceiveSync.c
    SendReceiveAsync.c
    NtAlpcDisconnectPort.c
    NtAlpcCancelMessage.c
    MessageValidation.c
    NtAlpcQueryInformation.c
    AlpcView.c
    NtAlpcOpenSender.c
    NtAlpcQueryInformationMessage.c
    NtAlpcImpersonate.c
    NtAlpcResourceReserve.c
    NtAlpcSecurityContext.c
    CompletionList.c
    ViewTransfer.c
    NtAlpcConnectPortEx.c
    alpc_helpers.c
    precomp.h)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(alpc_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(alpc_apitest wine ${PSEH_LIB})
set_module_type(alpc_apitest win32cui)
add_importlibs(alpc_apitest msvcrt advapi32 kernel32 ntdll)
add_pch(alpc_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET alpc_apitest)
