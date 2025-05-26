#include <windows.h>
#include <iostream>

DEFINE_GUID(GUID_DEVINTERFACE_MyDevice,
            0xaabbccdd, 0x1234, 0x5678, 0xab, 0xcd, 0xef, 0xaa, 0xbb, 0xcc, 0xdd, 0xee);

#define IOCTL_MY_DRIVER_COMMAND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main()
{

    HDEVINFO deviceInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_MyDevice, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to get device info." << std::endl;
        return 1;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {0};
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiEnumDeviceInterfaces(deviceInfo, NULL, &GUID_DEVINTERFACE_MyDevice, 0, &deviceInterfaceData))
    {
        std::cerr << "Device interface not found." << std::endl;
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return 1;
    }

    DWORD requiredSize = 0;
    SetupDiGetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
    detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, detailData, requiredSize, NULL, NULL))
    {
        std::cerr << "Failed to get interface detail." << std::endl;
        SetupDiDestroyDeviceInfoList(deviceInfo);
        free(detailData);
        return 1;
    }

    HANDLE hDevice = CreateFile(detailData->DevicePath,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to open device." << std::endl;
        SetupDiDestroyDeviceInfoList(deviceInfo);
        free(detailData);
        return 1;
    }

    char inBuf[] = "HELLO";
    char outBuf[128] = {0};
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(hDevice,
                                  IOCTL_MY_DRIVER_COMMAND,
                                  inBuf,
                                  sizeof(inBuf),
                                  outBuf,
                                  sizeof(outBuf),
                                  &bytesReturned,
                                  NULL);

    if (result)
    {
        std::cout << "Received from kernel: " << outBuf << std::endl;
    }
    else
    {
        std::cerr << "IOCTL call failed." << std::endl;
    }

    CloseHandle(hDevice);
    SetupDiDestroyDeviceInfoList(deviceInfo);
    free(detailData);
    return 0;
}
