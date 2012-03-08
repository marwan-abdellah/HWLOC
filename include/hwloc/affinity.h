#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

#include <X11/Xlib.h>
#include <NVCtrl.h>
#include <NVCtrlLib.h>

/* This function is used for testing this module functionality and shall
 * be removed before commiting the code */
static void print_children(hwloc_topology_t topology, hwloc_obj_t obj,
                           int depth)
{
    char string[128];
    unsigned i;

    hwloc_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);
    printf("%*s%s\n", 2*depth, "", string);
    for (i = 0; i < obj->arity; i++) {
        print_children(topology, obj->children[i], depth + 1);
    }
}

struct infoPCI
{
    char* _name;
    int _deviceId;
    hwloc_obj_t _ancestorSocket_Node;
    hwloc_bitmap_t _cpuSet;
};

bool queryDisplay(char* displayName, int& gpuId)
{
    /* Display */
    Display* display;

    /* Establishing a connection */
    display = XOpenDisplay(displayName);

    /* Existing display */
    if (display != 0)
        printf("DISPLAY %s is existing \n", displayName);
    else
    {
        printf("DISPLAY %s is NOT existing \n", displayName);
        return false;
    }

    /*
     * NV_CTRL_PCI_ID is a "packed" integer attribute; the PCI vendor ID is stored
     * in the upper 16 bits of the integer, and the PCI device ID is stored in the
     * lower 16 bits of the integer. */
    int nvCtrlPciId;
    const bool _return = XNVCTRLQueryTargetAttribute(display,
                         NV_CTRL_TARGET_TYPE_GPU,
                         0,
                         0,
                         NV_CTRL_PCI_ID,
                         &nvCtrlPciId);

    /* Attribute exists */
    if (_return)
    {
        /* GPU device ID */
        gpuId = (int) nvCtrlPciId & 0x0000FFFF;
        XCloseDisplay(display);

        return true;
    }

    /* Closing the display */
    XCloseDisplay(display);

    return false;
}

int queryDevices()
{
    /* GPU ID is set initially to -1 */
    int gpuId = -1;

    int xServerMax = 10;
    int xScreenMax = 10;

    for (int i = 0; i < xServerMax; ++i)
        for (int j = 0; j < xScreenMax; ++j)
        {
            /* String formation */
            char _server [5] ;
            char _screen [5];
            char _display [4];

            /*
             * Set the display name with the format
             * " [:][_server][.][_screen] " */
            strcat(_display, ":");
            snprintf(_server,sizeof(_server),"%d",i) ;
            strcat(_display, _server);
            strcat(_display, ".");
            snprintf(_screen,sizeof(_screen),"%d",j) ;
            strcat(_display, _screen);

            printf("DISPLAY= %s \n", _display);

            if (queryDisplay(_display, gpuId))
                return gpuId;
            else
                continue;

            /* Clearing _display */
            memset(&_display[0], 0, sizeof(_display));
        }
    return -1;
}

/*
 * Returns a list of the PCI device existing in your topology,
 * and their related information.
 * */
infoPCI* getPciDeviceInfo(int& deviceCount)
{
    /* Info about all the PCI device in the topology */
    infoPCI* deviceList;

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
    const int pciDevCount = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);
    deviceCount = pciDevCount;
    printf("pciDevCount %d \n", pciDevCount);

    /* A list of structure holding the needed info about the
     * attached PCI devices */
    deviceList = new infoPCI[pciDevCount];

    for (int i = 0; i < pciDevCount; ++i)
    {
        /* PCI device object */
        const hwloc_obj_t pciDeviceObject = hwloc_get_obj_by_type
                                           (topology, HWLOC_OBJ_PCI_DEVICE, i);

        /* PCI device number */
        deviceList[i]._name = pciDeviceObject->name;

        /* PCI device ID */
        hwloc_obj_attr_u::hwloc_pcidev_attr_s pciDeviceAttr = pciDeviceObject->attr->pcidev;
        deviceList[i]._deviceId = pciDeviceAttr.device_id;

        /* Ancestor socket (or node) */
        hwloc_obj_t parentObj = hwloc_get_ancestor_obj_by_type
                                 (topology, HWLOC_OBJ_MACHINE, pciDeviceObject);

        /* print_children(topology, parentObj, 0); */

        hwloc_bitmap_t _cpuSet;// = hwloc_bitmap_alloc();
        // hwloc_bitmap_zero(_cpuSet);

        if (parentObj != 0)
        {
            printf("parentSocket is existing \n");
            deviceList[i]._ancestorSocket_Node = parentObj;
            _cpuSet = parentObj->allowed_cpuset;
            deviceList[i]._cpuSet = _cpuSet;

            char* _cpuset_string;
            hwloc_bitmap_snprintf(_cpuset_string, sizeof(_cpuset_string), _cpuSet);
            printf("%s \n", _cpuset_string);
            free(_cpuset_string);
        }
    }

    /* Topology object destruction */
    hwloc_topology_destroy(topology);

    return deviceList;
}

/* Returns the cpuset of the corresponding socket attached to our GPU */
hwloc_bitmap_t getAutoCpuSet()
{



    /* Get the PCI ID of the connected GPU */
    const int attchedGpuId = queryDevices();

    /* Device list containing info about PCI devices
     * in our topology and their PCI IDs */
    int deviceCount;
    infoPCI* deviceList = getPciDeviceInfo(deviceCount);

    //hwloc_bitmap_t _cpuset = hwloc_bitmap_alloc();
    //hwloc_bitmap_zero(_cpuset);
    return deviceList[0]._cpuSet;

/*
    for (int i = 0; i < deviceCount; ++i)
    {
        if (deviceList[i]._deviceId == attchedGpuId)
        {
            printf("Catching the device @ %d \n", deviceList[i]._deviceId);
            // _cpuset = deviceList[i]._ancestorSocket_Node->cpuset;
            // return _cpuset;


        }
    }
*/
}
