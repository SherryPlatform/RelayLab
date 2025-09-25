#define MILE_MOBILITY_ENABLE_MINIMUM_SAL
#include <Mile.Mobility.Portable.Types.h>
#include <Mile.HyperV.VMBus.h>
#include <Mile.Helpers.CppBase.h>

#include <unistd.h>
#include <dirent.h>
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

typedef enum _RL_HV_UIO_MAP_TYPE
{
    RL_HV_UIO_MAP_TRANSMIT_RECEIVE_RING_BUFFER = 0,
    RL_HV_UIO_MAP_INTERRUPT_PAGE = 1,
    RL_HV_UIO_MAP_MONITOR_PAGE = 2,
    RL_HV_UIO_MAP_RECEIVE_BUFFER = 3,
    RL_HV_UIO_MAP_TRANSMIT_BUFFER = 4,
    RL_HV_UIO_MAP_MAXIMUM = 5,
} RL_HV_UIO_MAPPING_TYPE, *PRL_HV_UIO_MAPPING_TYPE;

typedef struct _RL_HV_UIO_MAP_ITEM
{
    MO_UINTN Offset;
    MO_UINTN Length;
} RL_HV_UIO_MAP_ITEM, *PRL_HV_UIO_MAP_ITEM;

// Format: "/dev/uioX", which X is a decimal number and maximum X is 1048575.
#define RL_HV_UIO_DEVICE_OBJECT_PATH_PREFIX "/dev/uio"

// Maximum Length: 8 ("/dev/uio") + 7 ("1048575") = 15
#define RL_HV_UIO_DEVICE_OBJECT_PATH_MAXIMUM_LENGTH 15

// Without "/dev/" prefix.
#define RL_HV_UIO_DEVICE_OBJECT_PATH_NAME_OFFSET 5

typedef struct _RL_HV_UIO_DEVICE_INFOMATION
{
    MO_CHAR DeviceObjectPath[RL_HV_UIO_DEVICE_OBJECT_PATH_MAXIMUM_LENGTH + 1];
    RL_HV_UIO_MAP_ITEM MemoryMapItems[RL_HV_UIO_MAP_MAXIMUM];
} RL_HV_UIO_DEVICE_INFOMATION, *PRL_HV_UIO_DEVICE_INFOMATION;

EXTERN_C int MOAPI RlHvUioGetDeviceInformation(
    _Out_ PRL_HV_UIO_DEVICE_INFOMATION Information,
    _In_ CONST MO_GUID* InstanceId)
{
    if (!Information || !InstanceId)
    {
        return EINVAL;
    }
    std::memset(Information, 0, sizeof(RL_HV_UIO_DEVICE_INFOMATION));

    int ErrorCode = 0;

    // "/sys/bus/vmbus/devices/xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx/uio"
    std::string UioRootPath = Mile::FormatString(
        "/sys/bus/vmbus/devices/"
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
        "/uio",
        InstanceId->Data1,
        InstanceId->Data2,
        InstanceId->Data3,
        InstanceId->Data4[0],
        InstanceId->Data4[1],
        InstanceId->Data4[2],
        InstanceId->Data4[3],
        InstanceId->Data4[4],
        InstanceId->Data4[5],
        InstanceId->Data4[6],
        InstanceId->Data4[7]);

    DIR* DirectoryObject = ::opendir(UioRootPath.c_str());
    if (DirectoryObject)
    {
        for (;;)
        {
            dirent* DirectoryEntry = ::readdir(DirectoryObject);
            if (!DirectoryEntry)
            {
                break;
            }

            if (DT_DIR != DirectoryEntry->d_type)
            {
                continue;
            }

            if (0 == std::strcmp(DirectoryEntry->d_name, ".") ||
                0 == std::strcmp(DirectoryEntry->d_name, ".."))
            {
                continue;
            }

            if ('u' != DirectoryEntry->d_name[0] ||
                'i' != DirectoryEntry->d_name[1] ||
                'o' != DirectoryEntry->d_name[2])
            {
                continue;
            }

            std::string Candidate = Mile::FormatString(
                "/dev/%s",
                DirectoryEntry->d_name);
            if (RL_HV_UIO_DEVICE_OBJECT_PATH_MAXIMUM_LENGTH < Candidate.size())
            {
                continue;
            }

            if (0 != ::access(Candidate.c_str(), F_OK))
            {
                continue;
            }

            std::strncpy(
                Information->DeviceObjectPath,
                Candidate.c_str(),
                RL_HV_UIO_DEVICE_OBJECT_PATH_MAXIMUM_LENGTH);
            break;
        }

        if ('\0' == Information->DeviceObjectPath[0])
        {
            ErrorCode = ENOENT;
        }

        ::closedir(DirectoryObject);
    }
    else
    {
        ErrorCode = errno;
    }

    if (0 == ErrorCode)
    {
        int PageSize = ::getpagesize();
        for (size_t i = 0; i < RL_HV_UIO_MAP_MAXIMUM; ++i)
        {
            Information->MemoryMapItems[i].Offset = i * PageSize;

            MO_STRING DeviceObjectName = Information->DeviceObjectPath;
            DeviceObjectName += RL_HV_UIO_DEVICE_OBJECT_PATH_NAME_OFFSET;

            FILE* FileObject = std::fopen(
                Mile::FormatString(
                    "%s/%s/maps/map%zu/size",
                    UioRootPath.c_str(),
                    DeviceObjectName,
                    i).c_str(),
                "r");
            if (FileObject)
            {
                if (EOF == std::fscanf(
                    FileObject,
                    "0x%zx",
                    &Information->MemoryMapItems[i].Offset))
                {
                    ErrorCode = errno;
                }

                std::fclose(FileObject);
            }
            else
            {
                ErrorCode = errno;
            }

            if (0 != ErrorCode)
            {
                break;
            }
        }
    }

    if (0 != ErrorCode)
    {
        std::memset(Information, 0, sizeof(RL_HV_UIO_DEVICE_INFOMATION));
    }

    return ErrorCode;
}

namespace
{
    void MountGpuDriversShares()
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
            return;
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
    }
}

int main()
{
    ::MountGpuDriversShares();

    return 0;
}
