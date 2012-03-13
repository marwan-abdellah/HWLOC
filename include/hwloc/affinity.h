#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

#include <X11/Xlib.h>
#include <NVCtrl.h>
#include <NVCtrlLib.h>


/* This function is used for testing this module functionality and shall
 * be removed before committing the code */
void print_children(hwloc_topology_t topology, hwloc_obj_t obj, int depth)
{
    char string[128];
    unsigned i;

    hwloc_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);
    printf("%*s%s\n", 2*depth, "", string);
    for (i = 0; i < obj->arity; i++) {
        print_children(topology, obj->children[i], depth + 1);
    }
}

/* Struct holding info about the PCI device */
struct pci_dev_info
{
    /* Name of the PCI device */
    char* _name;

    /* ID of the PCI device */
    int _deviceId;

    /* CPU set contained by the Socket connected on the same
     * level to the PCI device */
    hwloc_bitmap_t _cpuSet;
};

/* This function checks if a display with the displayName is existing or not.
 * If true, it returns the PCI ID of the GPU connected to the display
 * else, it returns -1.
 * */
int queryDisplay(char* displayName)
{
    int gpuId = -1;

    /* Display */
    Display* display;

    /* Establishing a connection */
    display = XOpenDisplay(displayName);

    Screen* screen;
    screen = XDefaultScreenOfDisplay(display);

    int screen_number = XScreenNumberOfScreen(screen);
    printf("screen_number %d \n", screen_number);

    /* Existing display */
    if (display != 0)
        printf("DISPLAY %s is existing \n", displayName);
    else
    {
        printf("DISPLAY %s is NOT existing \n", displayName);
        return -1;
    }

    /*
     * NV_CTRL_PCI_ID is a "packed" integer attribute; the PCI vendor ID is stored
     * in the upper 16 bits of the integer, and the PCI device ID is stored in the
     * lower 16 bits of the integer. */
    int nvCtrlPciId;
    const bool _return = XNVCTRLQueryTargetAttribute(display,
                         NV_CTRL_TARGET_TYPE_GPU,
                         0, /* target_id */
                         0, /* display_mask */
                         NV_CTRL_PCI_ID,
                         &nvCtrlPciId);

    /// check the parameters and also the default screen of the display...

    /* Attribute exists */
    if (_return)
    {
        /* GPU device ID */
        gpuId = (int) nvCtrlPciId & 0x0000FFFF;
        printf("Obtaining gpuId from the NV_control extension : %d \n", gpuId);
        XCloseDisplay(display);
        return gpuId;
    }

    /* Closing the display */
    XCloseDisplay(display);

    return -1;
}

/* Query all the displays on this system and returns the GPU ID connected to an
 * existing display */
int queryDevices(void)
{
    /* GPU ID is set initially to -1 */
    int gpuId = -1;

    int xServerMax = 10;
    int xScreenMax = 10;

    for (int i = 0; i < xServerMax; ++i)
    {
        for (int j = 0; j < xScreenMax; ++j)
        {
            /* String formation */
            char _server [5] ;
            char _screen [5];
            char _display [10];

            /*
             * Set the display name with the format
             * " [:][_server][.][_screen] " */
            strcat(_display, ":");
            snprintf(_server,sizeof(_server),"%d",i) ;
            strcat(_display, _server);
            strcat(_display, ".");
            snprintf(_screen,sizeof(_screen),"%d",j) ;
            strcat(_display, _screen);

            // printf("DISPLAY= %s \n", _display);

            gpuId = queryDisplay(_display);

            /* If -1, then it is not a correct display */
            if (gpuId == -1)
            {
                continue;
                /* Clearing */
                memset(&_server[0], 0, sizeof(_server));
                memset(&_screen[0], 0, sizeof(_screen));
                memset(&_display[0], 0, sizeof(_display));
            }
            else
            {
                return gpuId;
            }
        }
    }
    // printf("DISPLAY %d \n", gpuId);
    return -1;
}


/*
 * Returns a list of the PCI device existing in your topology,
 * and their related information.
 * */
pci_dev_info* getPciDeviceInfo(int& deviceCount)
{
    /* Info about all the PCI device in the topology */
    pci_dev_info* device_list;

    /* Topology object */
    hwloc_topology_t topology;

    /* Topology initialization */
    hwloc_topology_init(&topology);

    /* Flags used for loading the I/O devices and their relevant info */
    unsigned long flags = 0x0000000000000000;
    flags ^= HWLOC_TOPOLOGY_FLAG_IO_BRIDGES;    /* I/O bridges TBR */
    flags ^= HWLOC_TOPOLOGY_FLAG_IO_DEVICES;    /* I/O devices */

    /* Set the PCI discovery flags */
    const int _return = hwloc_topology_set_flags(topology, flags);

    /* Flags not set */
    if (_return < 0)
        printf("hwloc_topology_set_flags() failed \n");

    /* Perform topology detection */
    hwloc_topology_load(topology);

    print_children(topology, hwloc_get_root_obj(topology), 0);

    /* Get the number of PCi devices in this topology */
    const int pci_dev_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);
    deviceCount = pci_dev_count;
    printf("pci_dev_count %d \n", pci_dev_count);

    /* A list of structure holding the needed info about the
     * attached PCI devices */
    device_list = (pci_dev_info*) malloc (sizeof(pci_dev_info) * pci_dev_count);
    char* _cpuset;

    for (int i = 0; i < pci_dev_count; ++i)
    {
        /* PCI device object */
        const hwloc_obj_t pci_dev_object = hwloc_get_obj_by_type
                                           (topology, HWLOC_OBJ_PCI_DEVICE, i);

        /* PCI device number */
        device_list[i]._name = pci_dev_object->name;

        /* PCI device ID */
        device_list[i]._deviceId = pci_dev_object->attr->pcidev.device_id;

        /* Host bridge */
        const hwloc_obj_t host_bridge = hwloc_get_hostbridge_by_pcibus
                                (topology,
                                 pci_dev_object->attr->pcidev.domain,
                                 pci_dev_object->attr->pcidev.bus);

        const hwloc_obj_t prev_socket_obj = host_bridge->prev_sibling;
        device_list[i]._cpuSet = hwloc_bitmap_dup(host_bridge->prev_sibling->cpuset);
    }

    /* Topology object destruction */
    hwloc_topology_destroy(topology);

    return device_list;
}


hwloc_bitmap_t getAutoCpuSet()
{
    /* Number of PCI devices existing in our topology */
    int deviceCount;
    pci_dev_info* deviceList = getPciDeviceInfo(deviceCount);

    /* GPU ID deom quering the displays */
    int attchedGpuId = queryDevices();


    //printf("deviceCount %d \n", deviceCount);

    /* Auto- CPU set */
    hwloc_bitmap_t _autoCpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_zero(_autoCpuset);


    // Device matching GPU
    for (int i = 0; i < deviceCount; ++i)
    {
        if (deviceList[i]._deviceId != attchedGpuId)
        {
            printf("Not existing ... \n");
            continue;
        }
        else
        {
            printf("Existing ... \n");
            // hwloc_bitmap_copy(_autoCpuset, deviceList[i]._cpuSet);
            _autoCpuset = hwloc_bitmap_dup(deviceList[i]._cpuSet);

            // char *str;
            // int error = errno;
            // hwloc_bitmap_asprintf(&str, deviceList[i]._cpuSet);
            // printf("The final cpuset is %s: %s\n", str, strerror(error));
            // free(str);
            return _autoCpuset;
        }
    }


    return _autoCpuset;
}

