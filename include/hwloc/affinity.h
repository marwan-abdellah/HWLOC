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
    char* name;

    /* ID of the PCI device */
    int device_id;

    /* CPU set of the socket connected to the PCI device */
    hwloc_bitmap_t socket_cpuset;
};

/* This function checks if a display with the displayName is existing or not.
 * If true, it returns the PCI ID of the GPU connected to the default screen
 * of the display else, it returns -1.
 * */
int queryDisplay(char* displayName)
{
    /* PCI ID of the GPU connected to the display displayName */
    int gpu_pci_id = -1;

    /* Display */
    Display* display;

    /* Establishing a connection */
    display = XOpenDisplay(displayName);

    /* Is it an existing display */
    if (display != 0)
        printf("DISPLAY %s is existing \n", displayName);
    else {
        printf("DISPLAY %s is NOT existing \n", displayName);
        return -1;
    }

    /* Total number of screens attached to display */
    int number_screens;

    /* Query the number of screens connected to display */
    bool _return = XNVCTRLQueryTargetCount
                  (display,
                   NV_CTRL_TARGET_TYPE_X_SCREEN,
                   &number_screens);

    if (!_return) {
        fprintf(stderr, "Failed to retrieve the total number of screens attached to %s \n", displayName);
        return -1;
    }

    // TO BE REMOVED
    printf("Total number of screens of X screens via (NV-CONTROL): %d\n", number_screens);
    printf("Total number of screens of X screens via (ScreenCount): %d\n", ScreenCount(display));


    /* Default screen number */
    int default_screen_number = DefaultScreen(display);

    // TO BE REMOVED
    printf("Default screen number is : %d \n", default_screen_number);

    /* For further details, see the NVCtrlLib.h */
    unsigned int *ptr_binary_data;
    int data_lenght;

    /* Get the GPU number attached to the default screen */
    _return = XNVCTRLQueryTargetBinaryData
              (display,
              NV_CTRL_TARGET_TYPE_X_SCREEN,
              default_screen_number, /* target_id -> default_screen_number */
              0,
              NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
              (unsigned char **) &ptr_binary_data,
              &data_lenght);

    if (!_return) {
        fprintf(stderr, "Failed to query the GPUs attached to the default screen \n");
        return -1;
    }

    int gpu_number = ptr_binary_data[1];
    free(ptr_binary_data);

    // TO BE REMOVED
    printf("Connected GPU number is : %d \n", gpu_number);

    /* For further details, see the NVCtrlLib.h */
    int nvCtrlPciId;
     _return = XNVCTRLQueryTargetAttribute
               (display,
                NV_CTRL_TARGET_TYPE_GPU,
                gpu_number, /* target_id -> "gpu_number" */
                0, /* display_mask */
                NV_CTRL_PCI_ID,
                &nvCtrlPciId);

    if (_return)
    {
        /* GPU device ID */
        gpu_pci_id = (int) nvCtrlPciId & 0x0000FFFF;

        // TO BE REMOVED
        printf("Obtaining GPU PCI ID from the NV_control extension : %d \n", gpu_pci_id);

        /* Closing the connection */
        XCloseDisplay(display);
        return gpu_pci_id;
    }

    /* Closing the connection */
    XCloseDisplay(display);

    return -1;
}

/* Query all the displays on this system and return the GPU ID connected to
 * the default screen of an existing display */
int queryDevices(void)
{
    /* GPU ID is set initially to -1 */
    int gpu_pci_id = -1;

    int x_server_max = 10;
    int x_screen_max = 10;

    for (int i = 0; i < x_server_max; ++i) {
        for (int j = 0; j < x_screen_max; ++j) {
            char x_server [5] ;
            char x_screen [5];
            char x_display [10];

            /*
             * Set the display name with the format
             * " [:][x_server][.][x_screen] " */
            strcat(x_display, ":");
            snprintf(x_server,sizeof(x_server),"%d",i) ;
            strcat(x_display, x_server);
            strcat(x_display, ".");
            snprintf(x_screen,sizeof(x_screen),"%d",j) ;
            strcat(x_display, x_screen);

            // TO BE REMOVED
            printf("DISPLAY= %s \n", x_display);

            gpu_pci_id = queryDisplay(x_display);

            /* If -1, then it is not a correct display */
            if (gpu_pci_id == -1 && j == 0) {
                /* Clearing */
                memset(&x_server[0], 0, sizeof(x_server));
                memset(&x_screen[0], 0, sizeof(x_screen));
                memset(&x_display[0], 0, sizeof(x_display));
                break;
            }
            else { return gpu_pci_id; }
        }
    }
    return -1;
}

/*
 * Returns a list of the PCI device existing in your topology,
 * and their relevant info.
 * */
pci_dev_info* getPciDeviceInfo(int& device_count)
{
    /* Info about all the PCI device in the topology */
    pci_dev_info* device_list;

    /* Topology object */
    hwloc_topology_t topology;

    /* Topology initialization */
    hwloc_topology_init(&topology);

    /* Flags used for loading the I/O devices, bridges and their relevant info */
    unsigned long flags = 0x0000000000000000;
    flags ^= HWLOC_TOPOLOGY_FLAG_IO_BRIDGES;    /* I/O bridges */
    flags ^= HWLOC_TOPOLOGY_FLAG_IO_DEVICES;    /* I/O devices */

    /* Set the PCI discovery flags */
    const int _return = hwloc_topology_set_flags(topology, flags);

    /* Flags not set */
    if (_return < 0)
        printf("hwloc_topology_set_flags() failed \n");

    /* Perform topology detection */
    hwloc_topology_load(topology);

    // TO BE REMOVED
    // print_children(topology, hwloc_get_root_obj(topology), 0);

    /* Get the number of PCi devices in this topology */
    const int pci_dev_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);
    device_count = pci_dev_count;
    printf("Number of PCI devices existing in the topology is: %d \n", pci_dev_count);

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
        device_list[i].name = pci_dev_object->name;

        /* PCI device ID */
        device_list[i].device_id = pci_dev_object->attr->pcidev.device_id;

        /* Host bridge of the PCI device */
        const hwloc_obj_t host_bridge = hwloc_get_hostbridge_by_pcibus
                                (topology,
                                 pci_dev_object->attr->pcidev.domain,
                                 pci_dev_object->attr->pcidev.bus);

        device_list[i].socket_cpuset = hwloc_bitmap_dup(host_bridge->prev_sibling->cpuset);
    }

    /* Topology object destruction */
    hwloc_topology_destroy(topology);

    return device_list;
}


hwloc_bitmap_t getAutoCpuSet()
{
    /* Number of PCI devices existing in our topology */
    int device_count;
    pci_dev_info* device_list = getPciDeviceInfo(device_count);

    /* GPU ID deom quering the displays */
    int attchedGpuId = queryDevices();

    // TO BE REMOVED
    printf("Device Count is %d \n", device_count);

    /* Auto- CPU set */
    hwloc_bitmap_t _autoCpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_zero(_autoCpuset);


    // Matching GPU IDs
    for (int i = 0; i < device_count; ++i)
    {
        if (device_list[i].device_id != attchedGpuId) {
            // TO BE REMOVED
            printf("Not existing ... \n");

            continue;
        }
        else {
            // TO BE REMOVED
            printf("Existing ... \n");

            _autoCpuset = hwloc_bitmap_dup(device_list[i].socket_cpuset);
            return _autoCpuset;
        }
    }
    return _autoCpuset;
}
