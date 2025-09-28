﻿#define MILE_MOBILITY_ENABLE_MINIMUM_SAL
#include <Mile.Mobility.Portable.Types.h>
#include <Mile.HyperV.VMBus.h>
#include <Mile.Helpers.CppBase.h>

#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>

#include <cstdio>
#include <cstring>

#include <atomic>
#include <filesystem>
#include <span>

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
    MO_UINT32 Offset;
    MO_UINT32 Length;
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
        for (MO_UINT32 i = 0; i < RL_HV_UIO_MAP_MAXIMUM; ++i)
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
                    &Information->MemoryMapItems[i].Length))
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

typedef struct _RL_HV_UIO_DEVICE
{
    RL_HV_UIO_DEVICE_INFOMATION DeviceInformation;
    int FileDescriptor;
    MO_UINT32 RingBufferSize;
    MO_UINT32 ControlMaximumSize;
    MO_UINT32 DataMaximumSize;
    PVMRCB OutgoingControl;
    PMO_UINT8 OutgoingData;
    PVMRCB IncomingControl;
    PMO_UINT8 IncomingData;
} RL_HV_UIO_DEVICE, *PRL_HV_UIO_DEVICE;

EXTERN_C VOID MOAPI RlHvUioCloseDevice(
    _Inout_ PRL_HV_UIO_DEVICE Instance)
{
    if (Instance)
    {
        if (MAP_FAILED != Instance->OutgoingControl)
        {
            ::munmap(
                Instance->OutgoingControl,
                Instance->RingBufferSize * 2);
        }

        if (-1 != Instance->FileDescriptor)
        {
            ::close(Instance->FileDescriptor);
        }

        std::memset(Instance, 0, sizeof(RL_HV_UIO_DEVICE));
    }
}

EXTERN_C int MOAPI RlHvUioOpenDevice(
    _Out_ PRL_HV_UIO_DEVICE Instance,
    _In_ CONST MO_GUID* InstanceId)
{
    if (!Instance || !InstanceId)
    {
        return EINVAL;
    }
    std::memset(Instance, 0, sizeof(RL_HV_UIO_DEVICE));

    int ErrorCode = ::RlHvUioGetDeviceInformation(
        &Instance->DeviceInformation,
        InstanceId);
    if (0 == ErrorCode)
    {
        Instance->FileDescriptor = ::open(
            Instance->DeviceInformation.DeviceObjectPath,
            O_RDWR);
        if (-1 != Instance->FileDescriptor)
        {
            MO_UINT32 BlockOffset = Instance->DeviceInformation.MemoryMapItems[
                RL_HV_UIO_MAP_TRANSMIT_RECEIVE_RING_BUFFER].Offset;
            MO_UINT32 BlockLength = Instance->DeviceInformation.MemoryMapItems[
                RL_HV_UIO_MAP_TRANSMIT_RECEIVE_RING_BUFFER].Length;

            PMO_UINT8 MapStart = reinterpret_cast<PMO_UINT8>(::mmap(
                nullptr,
                BlockLength,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                Instance->FileDescriptor,
                BlockOffset));
            if (MAP_FAILED != MapStart)
            {
                int PageSize = ::getpagesize();

                Instance->RingBufferSize = BlockLength / 2;
                Instance->ControlMaximumSize = PageSize;
                Instance->DataMaximumSize =
                    Instance->RingBufferSize - Instance->ControlMaximumSize;

                PMO_UINT8 OutgoingStart = MapStart;
                PMO_UINT8 IncomingStart = MapStart + Instance->RingBufferSize;

                Instance->OutgoingControl = 
                    reinterpret_cast<PVMRCB>(OutgoingStart);
                Instance->OutgoingData =
                    OutgoingStart + Instance->ControlMaximumSize;
                Instance->IncomingControl = 
                    reinterpret_cast<PVMRCB>(IncomingStart);
                Instance->IncomingData =
                    IncomingStart + Instance->ControlMaximumSize;

                Instance->IncomingControl->InterruptMask = 0;
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
    }

    if (0 != ErrorCode)
    {
        ::RlHvUioCloseDevice(Instance);
    }

    return ErrorCode;
}

EXTERN_C int MOAPI RlHvUioSetInterruptState(
    _In_ PRL_HV_UIO_DEVICE Instance,
    _In_ MO_BOOL InterruptState)
{
    if (!Instance)
    {
        return EINVAL;
    }

    MO_UINT32 Value = InterruptState ? 1 : 0;
    return (sizeof(Value) == ::write(
        Instance->FileDescriptor,
        &Value,
        sizeof(Value)))
        ? 0
        : errno;
}

EXTERN_C int MOAPI RlHvUioWaitInterrupt(
    _In_ PRL_HV_UIO_DEVICE Instance,
    _Out_ PMO_UINT32 InterruptCount)
{
    if (!Instance || !InterruptCount)
    {
        return EINVAL;
    }

    return (sizeof(*InterruptCount) == ::read(
        Instance->FileDescriptor,
        InterruptCount,
        sizeof(*InterruptCount)))
        ? 0
        : errno;
}

namespace
{
    static void GetAvailableSizeInformation(
        _Out_opt_ PMO_UINT32 AvailableReceiveSize,
        _Out_opt_ PMO_UINT32 AvailableTransmitSize,
        _In_ PVMRCB RingControl,
        _In_ MO_UINT32 DataMaximumSize)
    {
        MO_UINT32 AvailableReceiveSizeResult = 0;
        MO_UINT32 AvailableTransmitSizeResult = 0;

        if (RingControl)
        {
            if (RingControl->In >= RingControl->Out)
            {
                AvailableReceiveSizeResult =
                    RingControl->In - RingControl->Out;
                AvailableTransmitSizeResult =
                    DataMaximumSize - AvailableReceiveSizeResult;
            }
            else
            {
                AvailableTransmitSizeResult =
                    RingControl->Out - RingControl->In;
                AvailableReceiveSizeResult =
                    DataMaximumSize - AvailableTransmitSizeResult;
            }
        }

        if (AvailableReceiveSize)
        {
            *AvailableReceiveSize = AvailableReceiveSizeResult;
        }

        if (AvailableTransmitSize)
        {
            *AvailableTransmitSize = AvailableTransmitSizeResult;
        }
    }

    static bool PopBytesToStructure(
        _Out_ void* Target,
        std::span<std::uint8_t> const& Source,
        std::size_t const& Size)
    {
        if (!Target || !Size || Source.size() < Size)
        {
            return false;
        }

        std::memcpy(Target, Source.data(), Size);
        return true;
    }

    static MO_UINTN GetAlignedSize(
        _In_ MO_UINTN Size,
        _In_ MO_UINTN Alignment)
    {
        return (Size + Alignment - 1) & ~(Alignment - 1);
    }
}

#define RL_HV_UIO_PIPE_PACKET_HEADER_SIZE \
    (sizeof(VMPACKET_DESCRIPTOR) + sizeof(VMPIPE_PROTOCOL_HEADER))

EXTERN_C int MOAPI RlHvUioReceive(
    _In_ PRL_HV_UIO_DEVICE Instance,
    _Out_ MO_POINTER Buffer,
    _In_ MO_UINT32 NumberOfBytesToReceive,
    _Out_ PMO_UINT32 NumberOfBytesReceived)
{
    if (!Instance ||
        !Buffer ||
        !NumberOfBytesToReceive ||
        !NumberOfBytesReceived)
    {
        return EINVAL;
    }
    *NumberOfBytesReceived = 0;

    MO_UINT32 AvailableSize = 0;
    ::GetAvailableSizeInformation(
        &AvailableSize,
        nullptr,
        Instance->IncomingControl,
        Instance->DataMaximumSize);
    if (AvailableSize < RL_HV_UIO_PIPE_PACKET_HEADER_SIZE)
    {
        return EAGAIN;
    }

    MO_UINT32 FirstReadSize = AvailableSize;
    MO_UINT32 SecondReadSize = 0;
    if (Instance->IncomingControl->In < Instance->IncomingControl->Out)
    {
        MO_UINT32 FragileSize =
            Instance->DataMaximumSize - Instance->IncomingControl->Out;
        if (FragileSize < AvailableSize)
        {
            FirstReadSize = FragileSize;
            SecondReadSize = AvailableSize - FragileSize;
        }
    }

    std::vector<MO_UINT8> AvailableBytes(AvailableSize);
    std::memcpy(
        &AvailableBytes[0],
        Instance->IncomingData + Instance->IncomingControl->Out,
        FirstReadSize);
    if (SecondReadSize)
    {
        std::memcpy(
            &AvailableBytes[FirstReadSize],
            Instance->IncomingData,
            SecondReadSize);
    }

    std::span<MO_UINT8> BytesSpan(AvailableBytes);

    VMPACKET_DESCRIPTOR Descriptor = {};
    if (!PopBytesToStructure(&Descriptor, BytesSpan, sizeof(Descriptor)))
    {
        return EIO;
    }
    if (VmbusPacketTypeDataInBand != Descriptor.Type)
    {
        return EIO;
    }
    MO_UINT32 DataOffset = static_cast<MO_UINT32>(Descriptor.DataOffset8) << 3;
    MO_UINT32 Length = static_cast<MO_UINT32>(Descriptor.Length8) << 3;
    if (DataOffset < sizeof(VMPACKET_DESCRIPTOR) ||
        DataOffset > AvailableBytes.size() ||
        DataOffset > Length)
    {
        return EIO;
    }
    MO_UINT32 ActualPacketSize = Length + sizeof(MO_UINT64);
    if (AvailableBytes.size() < ActualPacketSize)
    {
        return EIO;
    }
    BytesSpan = BytesSpan.subspan(DataOffset);

    VMPIPE_PROTOCOL_HEADER Header = {};
    if (!PopBytesToStructure(&Header, BytesSpan, sizeof(Header)))
    {
        return EIO;
    }
    if (VmPipeMessageData != Header.PacketType)
    {
        return EIO;
    }
    MO_UINT32 PipeDataSize = Header.DataSize;
    if (AvailableBytes.size() < sizeof(VMPIPE_PROTOCOL_HEADER) + PipeDataSize)
    {
        return EIO;
    }
    BytesSpan = BytesSpan.subspan(sizeof(VMPIPE_PROTOCOL_HEADER));

    *NumberOfBytesReceived = PipeDataSize;
    if (NumberOfBytesToReceive < PipeDataSize)
    {
        return EAGAIN;
    }
    if (!PopBytesToStructure(Buffer, BytesSpan, PipeDataSize))
    {
        return EIO;
    }

    MO_UINT32 FinalOffset = Instance->IncomingControl->Out + ActualPacketSize;
    if (Instance->IncomingControl->In < Instance->IncomingControl->Out)
    {
        MO_UINT32 FragileSize =
            Instance->DataMaximumSize - Instance->IncomingControl->Out;
        if (FragileSize < ActualPacketSize)
        {
            FinalOffset = ActualPacketSize - FragileSize;
        }
    }

    std::atomic_signal_fence(std::memory_order_acq_rel);
    MO_UINT32 PreviousOut = Instance->IncomingControl->Out;
    std::atomic_ref<MO_UINT32> AtomicOut(Instance->IncomingControl->Out);
    while (!AtomicOut.compare_exchange_weak(
        PreviousOut,
        FinalOffset,
        std::memory_order_acq_rel,
        std::memory_order_acquire))
    {
#if defined(__i386__) || defined(__x86_64__)
        asm volatile("pause" ::: "memory");
#elif defined(__aarch64__)
        asm volatile("yield" ::: "memory");
#endif
    }

    return 0;
}

EXTERN_C int MOAPI RlHvUioTransmit(
    _In_ PRL_HV_UIO_DEVICE Instance,
    _In_opt_ MO_CONSTANT_POINTER Buffer,
    _In_ MO_UINT32 NumberOfBytesToTransmit)
{
    if (!Instance || !Buffer || !NumberOfBytesToTransmit)
    {
        return EINVAL;
    }

    VMPACKET_DESCRIPTOR Descriptor = {};
    Descriptor.Type = VmbusPacketTypeDataInBand;
    Descriptor.DataOffset8 = static_cast<MO_UINT16>(
        sizeof(VMPACKET_DESCRIPTOR) >> 3);
    Descriptor.Length8 = std::min(
        static_cast<MO_UINT16>(::GetAlignedSize(
            RL_HV_UIO_PIPE_PACKET_HEADER_SIZE + NumberOfBytesToTransmit,
            sizeof(MO_UINT64)) >> 3),
        static_cast<MO_UINT16>(MO_UINT16_MAX));
    Descriptor.Flags = 0;
    Descriptor.TransactionId = MO_UINT64_MAX;

    VMPIPE_PROTOCOL_HEADER Header = {};
    Header.PacketType = VmPipeMessageData;
    Header.DataSize = NumberOfBytesToTransmit;

    MO_UINT32 PacketSize = static_cast<MO_UINT32>(Descriptor.Length8) << 3;
    PacketSize += static_cast<MO_UINT32>(sizeof(MO_UINT64));
    std::vector<MO_UINT8> PacketBytes(PacketSize);
    std::memcpy(
        &PacketBytes[0],
        &Descriptor,
        sizeof(Descriptor));
    std::memcpy(
        &PacketBytes[sizeof(Descriptor)],
        &Header,
        sizeof(Header));
    std::memcpy(
        &PacketBytes[sizeof(Descriptor) + sizeof(Header)],
        Buffer,
        NumberOfBytesToTransmit);
    std::memcpy(
        &PacketBytes[PacketSize - sizeof(MO_UINT64)],
        &Instance->OutgoingControl->In,
        sizeof(MO_UINT64));

    MO_UINT32 AvailableSize = 0;
    ::GetAvailableSizeInformation(
        nullptr,
        &AvailableSize,
        Instance->OutgoingControl,
        Instance->DataMaximumSize);
    if (AvailableSize < PacketSize)
    {
        return EAGAIN;
    }

    MO_UINT32 FirstWriteSize = PacketSize;
    MO_UINT32 SecondWriteSize = 0;
    if (Instance->OutgoingControl->In >= Instance->OutgoingControl->Out)
    {
        MO_UINT32 FragileSize =
            Instance->DataMaximumSize - Instance->OutgoingControl->In;
        if (FragileSize < PacketSize)
        {
            FirstWriteSize = FragileSize;
            SecondWriteSize = PacketSize - FragileSize;
        }
    }

    std::memcpy(
        Instance->OutgoingData + Instance->OutgoingControl->In,
        &PacketBytes[0],
        FirstWriteSize);
    if (SecondWriteSize)
    {
        std::memcpy(
            Instance->OutgoingData,
            &PacketBytes[FirstWriteSize],
            SecondWriteSize);
    }

    MO_UINT32 FinalOffset = SecondWriteSize > 0
        ? SecondWriteSize
        : Instance->OutgoingControl->In + FirstWriteSize;

    std::atomic_signal_fence(std::memory_order_acq_rel);
    MO_UINT32 PreviousIn = Instance->OutgoingControl->In;
    std::atomic_ref<MO_UINT32> AtomicIn(Instance->OutgoingControl->In);
    while (!AtomicIn.compare_exchange_weak(
        PreviousIn,
        FinalOffset,
        std::memory_order_acq_rel,
        std::memory_order_acquire))
    {
#if defined(__i386__) || defined(__x86_64__)
        asm volatile("pause" ::: "memory");
#elif defined(__aarch64__)
        asm volatile("yield" ::: "memory");
#endif
    }

    return 0;
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
