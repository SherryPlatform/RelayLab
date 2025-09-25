#define MILE_MOBILITY_ENABLE_MINIMUM_SAL
#include <Mile.Mobility.Portable.Types.h>
#include <Mile.HyperV.VMBus.h>
#include <Mile.Helpers.CppBase.h>

#include <unistd.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>

#include <cstdio>
#include <cstring>

#include <filesystem>

EXTERN_C int MOAPI RlHvMountHcsPlan9Share(
    _In_ MO_UINT32 Port,
    _In_ MO_CONSTANT_STRING AccessName,
    _In_ MO_CONSTANT_STRING MountPoint,
    _In_ MO_BOOL ReadOnly,
    _In_ MO_UINT32 BufferSize)
{
    int ErrorCode = 0;

    int Socket = ::socket(AF_VSOCK, SOCK_STREAM, 0);
    if (-1 != Socket)
    {
        sockaddr_vm SocketAddress = { 0 };
        SocketAddress.svm_family = AF_VSOCK;
        SocketAddress.svm_port = Port;
        SocketAddress.svm_cid = VMADDR_CID_HOST;
        if (0 == ::connect(
            Socket,
            reinterpret_cast<sockaddr*>(&SocketAddress),
            sizeof(SocketAddress)))
        {
            if (0 == ::setsockopt(
                Socket,
                SOL_SOCKET,
                SO_SNDBUF,
                &BufferSize,
                sizeof(BufferSize)))
            {
                if (0 != ::mount(
                    AccessName,
                    MountPoint,
                    "9p",
                    ReadOnly ? MS_RDONLY : 0,
                    Mile::FormatString(
                        "trans=fd,rfdno=%d,wfdno=%d,msize=%d,noload,aname=%s",
                        Socket,
                        Socket,
                        BufferSize,
                        AccessName).c_str()))
                {
                    ErrorCode = errno;
                }
            }
            else
            {
                ErrorCode = errno;
            }
        }
        else
        {
            ErrorCode = errno;
        }

        ::close(Socket);
    }
    else
    {
        ErrorCode = errno;
    }

    return ErrorCode;
}

EXTERN_C int MOAPI RlHvUioRegisterDevice(
    _In_ CONST MO_GUID* ClassId)
{
    if (!ClassId)
    {
        return EINVAL;
    }

    int ErrorCode = 0;

    FILE* FileObject = std::fopen(
        "/sys/bus/vmbus/drivers/uio_hv_generic/new_id",
        "w");
    if (FileObject)
    {
        // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        const int GuidStringLength = 36;

        int Result = std::fprintf(
            FileObject,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            ClassId->Data1,
            ClassId->Data2,
            ClassId->Data3,
            ClassId->Data4[0],
            ClassId->Data4[1],
            ClassId->Data4[2],
            ClassId->Data4[3],
            ClassId->Data4[4],
            ClassId->Data4[5],
            ClassId->Data4[6],
            ClassId->Data4[7]);
        if (Result < 0)
        {
            ErrorCode = errno;
        }
        else if (GuidStringLength != Result)
        {
            ErrorCode = EIO;
        }

        std::fclose(FileObject);
    }
    else
    {
        ErrorCode = errno;
    }

    return ErrorCode;
}

int main()
{
    try
    {
        std::filesystem::create_directories("/usr/lib/wsl/drivers");
        std::filesystem::create_directories("/usr/lib/wsl/lib");
        std::filesystem::create_directories("/mnt/host");
    }
    catch (std::exception const& ex)
    {
        std::fprintf(stderr, "Failed to create directory: %s\n", ex.what());
        return 1;
    }

    int Result = ::RlHvMountHcsPlan9Share(
        50001,
        "NanaBox.HostDrivers",
        "/usr/lib/wsl/drivers",
        MO_TRUE,
        65536);
    std::printf("RlHvMountHcsPlan9Share returns %d\n", Result);

    Result = ::RlHvMountHcsPlan9Share(
        50001,
        "NanaBox.HostLxssLib",
        "/usr/lib/wsl/lib",
        MO_TRUE,
        65536);
    std::printf("RlHvMountHcsPlan9Share returns %d\n", Result);

    Result = ::RlHvMountHcsPlan9Share(
        4869,
        "RelayLab.ShareRoot",
        "/mnt/host",
        MO_FALSE,
        65536);
    std::printf("RlHvMountHcsPlan9Share returns %d\n", Result);

    return 0;
}
