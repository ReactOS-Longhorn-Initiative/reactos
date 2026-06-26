#define STANDALONE
#include <apitest.h>

extern void func_AlpcHelpers(void);
extern void func_NtAlpcCreatePort(void);
extern void func_NtAlpcConnectPort(void);
extern void func_NtAlpcAcceptConnectPort(void);
extern void func_SendReceiveSync(void);
extern void func_SendReceiveAsync(void);
extern void func_NtAlpcDisconnectPort(void);
extern void func_NtAlpcCancelMessage(void);
extern void func_MessageValidation(void);
extern void func_NtAlpcQueryInformation(void);
extern void func_AlpcView(void);
extern void func_NtAlpcOpenSender(void);
extern void func_NtAlpcQueryInformationMessage(void);
extern void func_NtAlpcImpersonate(void);
extern void func_NtAlpcResourceReserve(void);
extern void func_NtAlpcSecurityContext(void);
extern void func_CompletionList(void);
extern void func_ViewTransfer(void);
extern void func_NtAlpcConnectPortEx(void);

const struct test winetest_testlist[] =
{
    { "AlpcHelpers",             func_AlpcHelpers },
    { "NtAlpcCreatePort",        func_NtAlpcCreatePort },
    { "NtAlpcConnectPort",       func_NtAlpcConnectPort },
    { "NtAlpcAcceptConnectPort", func_NtAlpcAcceptConnectPort },
    { "SendReceiveSync",         func_SendReceiveSync },
    { "SendReceiveAsync",        func_SendReceiveAsync },
    { "NtAlpcDisconnectPort",    func_NtAlpcDisconnectPort },
    { "NtAlpcCancelMessage",     func_NtAlpcCancelMessage },
    { "MessageValidation",       func_MessageValidation },
    { "NtAlpcQueryInformation",  func_NtAlpcQueryInformation },
    { "AlpcView",                func_AlpcView },
    { "NtAlpcOpenSender",        func_NtAlpcOpenSender },
    { "NtAlpcQueryInformationMessage", func_NtAlpcQueryInformationMessage },
    { "NtAlpcImpersonate",       func_NtAlpcImpersonate },
    { "NtAlpcResourceReserve",   func_NtAlpcResourceReserve },
    { "NtAlpcSecurityContext",   func_NtAlpcSecurityContext },
    { "CompletionList",          func_CompletionList },
    { "ViewTransfer",            func_ViewTransfer },
    { "NtAlpcConnectPortEx",     func_NtAlpcConnectPortEx },
    { 0, 0 }
};
