#include <ntddk.h>
#include <wdf.h>
#include <initguid.h> // Để dùng DEFINE_GUID
#include <ntstrsafe.h>

// Định nghĩa GUID cho thiết bị
DEFINE_GUID(GUID_DEVINTERFACE_MyDevice,
            0xaabbccdd, 0x1234, 0x5678, 0xab, 0xcd, 0xef, 0xaa, 0xbb, 0xcc, 0xdd, 0xee);

// IOCTL định nghĩa
#define IOCTL_MY_DRIVER_COMMAND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Prototype
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD MyEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MyEvtIoDeviceControl;

// DriverEntry
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("[Driver kernel] DriverEntry called\n"));

    WDF_DRIVER_CONFIG_INIT(&config, MyEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("[Driver kernel] WdfDriverCreate failed: 0x%08x\n", status));
    }

    return status;
}

// EvtDeviceAdd
NTSTATUS
MyEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);
    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;

    KdPrint(("[Driver kernel] EvtDeviceAdd called\n"));

    // Tạo thiết bị
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[Driver kernel] WdfDeviceCreate failed: 0x%08x\n", status));
        return status;
    }

    // Đăng ký interface dùng GUID
    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_MyDevice, NULL);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[Driver kernel] WdfDeviceCreateDeviceInterface failed: 0x%08x\n", status));
        return status;
    }

    // Tạo hàng đợi IOCTL
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
    ioQueueConfig.EvtIoDeviceControl = MyEvtIoDeviceControl;

    status = WdfIoQueueCreate(device,
                              &ioQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              NULL);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("[Driver kernel] WdfIoQueueCreate failed: 0x%08x\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

// Xử lý IOCTL
VOID MyEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(Queue);
    NTSTATUS status = STATUS_SUCCESS;
    PVOID inBuf = NULL, outBuf = NULL;
    size_t inLen = 0, outLen = 0;
    KdPrint(("[Driver kernel] Call IOCTL!\n"));
    if (IoControlCode == IOCTL_MY_DRIVER_COMMAND)
    {
        status = WdfRequestRetrieveInputBuffer(Request, 5, &inBuf, &inLen);
        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(Request, status);
            return;
        }

        status = WdfRequestRetrieveOutputBuffer(Request, OutputBufferLength, &outBuf, &outLen);
        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(Request, status);
            return;
        }

        if (inLen >= 5 && memcmp(inBuf, "HELLO", 5) == 0)
        {
            RtlStringCchCopyA((char *)outBuf, outLen, "Hi from Kernel!");
        }
        else
        {
            RtlStringCchCopyA((char *)outBuf, outLen, "Unknown Command");
        }

        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, strlen((char *)outBuf) + 1);
    }
    else
    {
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
    }
}