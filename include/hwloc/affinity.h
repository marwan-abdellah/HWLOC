#ifndef HWLOC_AUTO_AFFINITY_H
#define HWLOC_AUTO_AFFINITY_H

#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"



#include <hwloc.h>
#include </usr/include/X11/Xlib.h>
#include </usr/include/NVCtrl/NVCtrl.h>
#include </usr/include/NVCtrl/NVCtrlLib.h>

#ifdef __cplusplus
extern "C" {
#endif

static void
hwloc_linux_lookup_dpy_class(struct hwloc_topology *topology, struct hwloc_obj *pcidev);

struct display_info
{
    int port;   /* Port or X-server */
    int device; /* Device or X-screen */
};

struct gpu_info
{
    int pci_device_id;
    int bus_id;
};


/* A struct holding info about the PCI device */
struct pci_dev_info
{
    /* Name of the PCI device */
    char* name;

    /* Type of the PCI device */
    char* type;

    /* ID of the PCI device */
    int device_id;

    /* PCI Bus ID of the device */
    int bus_id;

    /* Used for printing its corresponding display */
    bool is_gpu;

    /* Display parameters "port & device" */
    display_info display;

    /* CPU set of the socket connected to the PCI device */
    hwloc_bitmap_t socket_cpuset;
};

/* Prototypes */

static __hwloc_inline pci_dev_info* getPciDeviceInfo(int& device_count);
static __hwloc_inline gpu_info queryDisplay(char* displayName);
static __hwloc_inline gpu_info queryDevices(void);
static __hwloc_inline hwloc_bitmap_t get_auto_cpuset();
static __hwloc_inline hwloc_bitmap_t get_display_cpuset(const int port, const int device);
static __hwloc_inline display_info get_gpu_display(const gpu_info gpu_ids);
static __hwloc_inline hwloc_bitmap_t get_gpu_cpuset(const gpu_info gpu_ids);
static __hwloc_inline void print_children_extended(hwloc_topology_t topology, hwloc_obj_t obj, int depth);

/*****************************************************************
 * This function is used for testing this module functionality and
 * shall be removed before committing the code
 *****************************************************************/
void print_children(hwloc_topology_t topology, hwloc_obj_t obj, int depth)
{
    char string[128];
    unsigned i;

    hwloc_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);

    printf("%*s%s %s \n", 2*depth, "", string, obj->name);
    for (i = 0; i < obj->arity; i++) {
        print_children(topology, obj->children[i], depth + 1);
    }
}

/*****************************************************************
 * To give extra information about the PCI devices existing in
 * this topology.
 *****************************************************************/
static __hwloc_inline void print_children_extended(hwloc_topology_t topology, hwloc_obj_t obj, int depth)
{
    char string[128];
    unsigned i;

    hwloc_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);

    hwloc_obj_t parent = obj->parent;
    if (parent != NULL) {
        if (parent->type == HWLOC_OBJ_PCI_DEVICE) {
            printf("%*s%s %s \n", 2*depth, "", string, obj->name); }
        else {
            printf("%*s%s Bus :%d\n", 2*depth, "", string, obj->attr->pcidev.bus); }
    }
    for (i = 0; i < obj->arity; i++) {
    print_children_extended(topology, obj->children[i], depth + 1);
    }
}

/*****************************************************************
 * This function checks if a display with the displayName is
 * existing or not. If true, it returns the PCI ID of the GPU
 * connected to the default screen of the display else, it
 * returns -1 indicating failure.
 *****************************************************************/
static __hwloc_inline gpu_info queryDisplay(char* displayName)
{
    /* PCI ID of the GPU connected to the input display */
    // int gpu_pci_id = -1;
    gpu_info gpu_ids;
    gpu_ids.bus_id = -1;
    gpu_ids.pci_device_id = -1;

    /* Display */
    Display* display = XOpenDisplay(displayName);
    if (display == 0) {
        // fprintf(stderr, "DISPLAY %s is NOT existing \n", displayName);
        return gpu_ids;
    }

    /* Query the number of screens connected to display */
    int number_screens;
    bool sucess = XNVCTRLQueryTargetCount
                      (display,
                       NV_CTRL_TARGET_TYPE_X_SCREEN,
                       &number_screens);

    /* If query fails */
    if (!sucess) {
        fprintf(stderr, "Failed to retrieve the total number of "
                        "screens attached to %s \n", displayName);
        return gpu_ids;
    }

    // TO BE REMOVED
    fprintf(stderr, "Total number of screens of X screens : %d\n", number_screens);


    /* Default screen number */
    const int default_screen_number = DefaultScreen(display);

    // TO BE REMOVED
    fprintf(stderr, "Default screen number is : %d \n", default_screen_number);

    /* Get the GPU number attached to the default screen.
     * For further details, see the NVCtrlLib.h */
    unsigned int *ptr_binary_data;
    int data_lenght;

    sucess = XNVCTRLQueryTargetBinaryData
                   (display,
                    NV_CTRL_TARGET_TYPE_X_SCREEN,
                    default_screen_number,
                    0,
                    NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                    (unsigned char **) &ptr_binary_data,
                    &data_lenght);

    if (!sucess) {
        fprintf(stderr, "Failed to query the GPUs attached to the default screen \n");
        return gpu_ids;
    }

    const int gpu_number = ptr_binary_data[1];
    free(ptr_binary_data);

    // TO BE REMOVED
    printf("Connected GPU number is : %d \n", gpu_number);

    /* For further details, see the NVCtrlLib.h */
    int nvCtrlPciId;
    bool sucess_1 = XNVCTRLQueryTargetAttribute
                   (display,
                    NV_CTRL_TARGET_TYPE_GPU,
                    gpu_number,
                    0,
                    NV_CTRL_PCI_ID,
                    &nvCtrlPciId);

     int nvCtrlPciBusId;
     bool sucess_2 = XNVCTRLQueryTargetAttribute
                        (display,
                         NV_CTRL_TARGET_TYPE_GPU,
                         gpu_number,
                         0,
                         NV_CTRL_PCI_BUS,
                         &nvCtrlPciBusId);

     printf("--- Connected GPU nvCtrlPciBusId is : %d \n", nvCtrlPciBusId);

    if (sucess_1 && sucess_2) {
        /* GPU device ID */
        gpu_ids.pci_device_id = (int) nvCtrlPciId & 0x0000FFFF;
        gpu_ids.bus_id = nvCtrlPciBusId;

        // TO BE REMOVED
        printf("Obtaining GPU PCI ID from the NV_control extension : %d, %d\n", gpu_ids.pci_device_id, gpu_ids.bus_id);

        /* Closing the connection */
        XCloseDisplay(display);
        return gpu_ids;
    }

    /* Closing the connection */
    XCloseDisplay(display);
    return gpu_ids;
}

/*****************************************************************
 * Query all the displays on this system and return the GPU ID
 * connected to the default screen of an existing display
 *****************************************************************/
static __hwloc_inline gpu_info queryDevices(void)
{
    /* GPU ID is set initially to -1 */
    // int gpu_pci_id = -1;
    gpu_info gpu_ids;
    gpu_ids.bus_id = -1;
    gpu_ids.pci_device_id = -1;

    int x_server_max = 10;
    int x_screen_max = 10;

    for (int i = 0; i < x_server_max; ++i) {
        for (int j = 0; j < x_screen_max; ++j) {

            /*
             * Set the display name with the format " [:][x_server][.][x_screen] " */
            char x_display [10];
            snprintf(x_display,sizeof(x_display),":%d.%d",i, j);

            // TO BE REMOVED
            printf("DISPLAY= %s \n", x_display);

            gpu_ids = queryDisplay(x_display);

            /* is a correct display */
            if (gpu_ids.pci_device_id != -1 || j != 0)
                return gpu_ids;
        }
    }
    return gpu_ids;
}

/*****************************************************************
 * Returns a list of the PCI device existing in this topology,
 * and their relevant info.
 ****************************************************************/
static __hwloc_inline pci_dev_info* getPciDeviceInfo(int& device_count)
{
    /* Info about all the PCI device in the topology */
    pci_dev_info* device_list;

    /* Topology object */
    hwloc_topology_t topology;

    /* Topology initialization */
    hwloc_topology_init(&topology);

    /* Flags used for loading the I/O devices, bridges and their relevant info */
    const unsigned long loading_flags =
        0x0000000000000000 ^ HWLOC_TOPOLOGY_FLAG_IO_BRIDGES ^ HWLOC_TOPOLOGY_FLAG_IO_DEVICES;

    /* Set discovery flags */
    const int sucess = hwloc_topology_set_flags(topology, loading_flags);

    /* Flags not set */
    if (sucess < 0)
        printf("hwloc_topology_set_flags() failed, PCI devices will not be loaded in the topology \n");

    /* Perform topology detection */
    hwloc_topology_load(topology);

    /* Get the number of PCi devices in this topology */
    const int pci_dev_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);
    device_count = pci_dev_count;



    // TO BE REMOVED
    printf("Number of PCI devices existing in the topology is: %d \n", pci_dev_count);

    /* A list of structure holding the needed info about the
     * attached PCI devices */
    device_list = (pci_dev_info*) malloc (sizeof(pci_dev_info) * pci_dev_count);

    for (int i = 0; i < pci_dev_count; ++i) {
        /* PCI device object */
         hwloc_obj_t pci_dev_object = hwloc_get_obj_by_type
                                           (topology, HWLOC_OBJ_PCI_DEVICE, i);

        /* PCI device number */
        device_list[i].name = pci_dev_object->name;

        // TO BE REMOVED
        char _type[128];
        hwloc_obj_type_snprintf(_type, sizeof(_type), pci_dev_object, 0);
        printf("*_type %s \n", _type);

        /* PCI device ID */
        device_list[i].device_id = pci_dev_object->attr->pcidev.device_id;

        gpu_info gpu_ids;
        gpu_ids.pci_device_id = pci_dev_object->attr->pcidev.device_id;
        gpu_ids.bus_id = pci_dev_object->attr->pcidev.bus;
        /* Getting the display info */
        display_info display = get_gpu_display(gpu_ids);

        // TO BE REMOVED
        printf("Display <%d.%d \n", display.port, display.device);

        /* If GPU */
        if (display.port > -1 && display.device > -1) {
            device_list[i].is_gpu = true;
            device_list[i].display = display;

            /* Appending the display as a children to the GPU object */
            char x_display[128];
            snprintf(x_display,sizeof(x_display),"DISPLAY= :%d.%d", (display.port), display.device);
            hwloc_obj_t disp_obj = hwloc_topology_insert_misc_object_by_parent(topology, pci_dev_object, x_display);




        }
        else {
            device_list[i].is_gpu = false;
            device_list[i].display = display;
        }

        // TO BE REMOVED
        printf("Display info for the PCI ID %d:%d.%d \n", pci_dev_object->attr->pcidev.device_id,
                                                           display.port, display.device);

        /* Host bridge of the PCI device */
        const hwloc_obj_t host_bridge = hwloc_get_hostbridge_by_pcibus
                                (topology,
                                 pci_dev_object->attr->pcidev.domain,
                                 pci_dev_object->attr->pcidev.bus);

        /* Get the cpuset of the socket attached to host bridge
         * at which the PCI device is connected */
        device_list[i].socket_cpuset = hwloc_bitmap_dup(host_bridge->prev_sibling->cpuset);
    }

    // TO BE REMOVED
    print_children_extended(topology, hwloc_get_root_obj(topology), 0);
    // print_children(topology, hwloc_get_root_obj(topology), 0);

    /* Topology object destruction */
    hwloc_topology_destroy(topology);

    return device_list;
}

/*****************************************************************
 * Returns a cpuset of the socket attached to the host bridge
 * where the GPU the is connected in the topology
 ****************************************************************/
static __hwloc_inline hwloc_bitmap_t get_gpu_cpuset(const gpu_info gpu_ids)
{
    /* Topology object */
    hwloc_topology_t topology;

    /* Topology initialization */
    hwloc_topology_init(&topology);

    /* Flags used for loading the I/O devices, bridges and their
     * relevant info to the topology */
    const unsigned long loading_flags =
        0x0000000000000000 ^ HWLOC_TOPOLOGY_FLAG_IO_BRIDGES ^ HWLOC_TOPOLOGY_FLAG_IO_DEVICES;

    /* Set discovery flags */
    const int sucess = hwloc_topology_set_flags(topology, loading_flags);

    /* Flags not set */
    if (sucess < 0)
        printf("hwloc_topology_set_flags() failed, PCI devices will not be loaded in the topology \n");

    /* Perform topology detection */
    hwloc_topology_load(topology);

    // TO BE REMOVED
    // print_children(topology, hwloc_get_root_obj(topology), 0);

    /* The number of PCI devices in the topology */
    const int pci_dev_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);

    // TO BE REMOVED
    printf("Number of PCI devices existing in the topology is: %d \n", pci_dev_count);

    for (int i = 0; i < pci_dev_count; ++i) {
        /* PCI device object */
        const hwloc_obj_t pci_dev_object = hwloc_get_obj_by_type
                                           (topology, HWLOC_OBJ_PCI_DEVICE, i);

        /* PCI device ID */
         if (gpu_ids.pci_device_id == pci_dev_object->attr->pcidev.device_id && gpu_ids.bus_id == pci_dev_object->attr->pcidev.bus) {
            /* Host bridge of the PCI device */
            const hwloc_obj_t host_bridge = hwloc_get_hostbridge_by_pcibus
                                   (topology,
                                    pci_dev_object->attr->pcidev.domain,
                                    pci_dev_object->attr->pcidev.bus);

            /* Get the cpuset of the socket attached to host bridge
            * at which the PCI device is connected */
            return hwloc_bitmap_dup(host_bridge->prev_sibling->cpuset);
         }
         else continue;
    }

    /* Topology object destruction */
    hwloc_topology_destroy(topology);


}

/*****************************************************************
 * Returns the cpuset of the socket connected to the host bridge
 * connecting the GPU attached to the display defined by the
 * input port and device integers and having the format
 * [:][port][.][device] under X systems
 ****************************************************************/
static __hwloc_inline hwloc_bitmap_t get_display_cpuset(const int port, const int device)
{
    char x_display [10];
    snprintf(x_display,sizeof(x_display),":%d.%d", port, device);

    // TO BE REMOVED
    printf("DISPLAY= %s \n", x_display);

    const gpu_info gpu_ids = queryDisplay(x_display);
    const hwloc_bitmap_t cpuset = get_gpu_cpuset(gpu_ids);

    // TO BE REMOVED
    char* cpuset_string;
    hwloc_bitmap_asprintf(&cpuset_string, cpuset);
    printf("Selected CPU set is %s: \n", cpuset_string);

    return hwloc_bitmap_dup(cpuset);
}

/*****************************************************************
 * Returns a DISPLAY for a given GPU.
 * The returned structure should have the format
 *                                      ([:][port][.][device])
 ****************************************************************/
static __hwloc_inline display_info get_gpu_display(const gpu_info gpu_ids)
{
    display_info display;
    display.port = -1;
    display.device = -1;

    int x_server_max = 10;
    int x_screen_max = 10;

    for (int i = 0; i < x_server_max; ++i) {
        for (int j = 0; j < x_screen_max; ++j) {

            /* Set the display name with the format "[:][x_server][.][x_screen]" */
            char x_display [10];
            snprintf(x_display,sizeof(x_display),":%d.%d",i, j);

            /* Connection failure */
            if (gpu_ids.bus_id != queryDisplay(x_display).bus_id) {
                continue;
            }
            else {

                display.port = i;
                display.device = j;

                // TO BE REMOVED
                printf("DISPLAY= %s \n", x_display);

                return display;
            }
        }
    }

    // TO BE REMOVED
    printf("No display was matching the GPU having PCI ID = %d %d \n", gpu_ids.pci_device_id, gpu_ids.bus_id);
    return display;
}

static __hwloc_inline hwloc_bitmap_t get_auto_cpuset() // port and device
{
    /* Number of PCI devices existing in the topology */
    int device_count;

    /* Returns a list of all the PCI devices in this topology */
    pci_dev_info* device_list = getPciDeviceInfo(device_count);

    /* GPU ID quering the displays */
    const gpu_info gpu_ids = queryDevices();

    // TO BE REMOVED
    printf("Device Count is %d \n", device_count);

    /* Auto- CPU set */
    hwloc_bitmap_t auto_cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_zero(auto_cpuset);


    // Matching GPU IDs
    for (int i = 0; i < device_count; ++i)
    {
        if (device_list[i].device_id != gpu_ids.pci_device_id) {
            // TO BE REMOVED
            printf("Not existing .... \n");

            continue;
        }
        else {
            // TO BE REMOVED
            printf("Existing .... \n");

            auto_cpuset = hwloc_bitmap_dup(device_list[i].socket_cpuset);
            return auto_cpuset;
        }
    }

    free(device_list);

    return auto_cpuset;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_AUTO_AFFINITY_H */
