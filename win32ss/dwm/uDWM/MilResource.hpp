class MilResource
{
private:

public:
    HRESULT WINAPI InitializeResource(MIL_RESOURCE_TYPE type, MIL_CHANNEL MilChannel);
    HRESULT WINAPI SendCommand(PVOID Command, UINT32 Size);
    MilResource();
    ~MilResource();
    HMIL_RESOURCE GlobalResourceHandle;
};
