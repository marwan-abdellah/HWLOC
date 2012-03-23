/*
 * Copyright © 2010 inria.  All rights reserved.
 * Copyright © 2010-2011 Université Bordeaux 1
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief For getting the display attached to a certain GPU ...
 *
 * Later
 *
 */

#ifndef HWLOC_GL_H
#define HWLOC_GL_H

#include <hwloc.h>
#include <hwloc/autogen/config.h>
#include <hwloc/linux.h>
#include <hwloc/helper.h>

#ifdef HWLOC_HAVE_GL
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#endif

#ifdef HWLOC_HAVE_GL
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
    int is_gpu;

    /* Display parameters "port & device" */
    struct display_info display;

    /* CPU set of the socket connected to the PCI device */
    hwloc_bitmap_t socket_cpuset;
};



static struct gpu_info queryDisplay(char* displayName)
{
    /* PCI ID of the GPU connected to the input display */
    // int gpu_pci_id = -1;
    struct gpu_info gpu_ids;
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
    int sucess = XNVCTRLQueryTargetCount
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
    // fprintf(stderr, "Total number of screens of X screens : %d\n", number_screens);


    /* Default screen number */
    const int default_screen_number = DefaultScreen(display);

    // TO BE REMOVED
    // fprintf(stderr, "Default screen number is : %d \n", default_screen_number);

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
    // printf("Connected GPU number is : %d \n", gpu_number);

    /* For further details, see the NVCtrlLib.h */
    int nvCtrlPciId;
    int sucess_1 = XNVCTRLQueryTargetAttribute
                   (display,
                    NV_CTRL_TARGET_TYPE_GPU,
                    gpu_number,
                    0,
                    NV_CTRL_PCI_ID,
                    &nvCtrlPciId);

     int nvCtrlPciBusId;
     int sucess_2 = XNVCTRLQueryTargetAttribute
                        (display,
                         NV_CTRL_TARGET_TYPE_GPU,
                         gpu_number,
                         0,
                         NV_CTRL_PCI_BUS,
                         &nvCtrlPciBusId);

     // printf("--- Connected GPU nvCtrlPciBusId is : %d \n", nvCtrlPciBusId);

    if (sucess_1 && sucess_2) {
        /* GPU device ID */
        gpu_ids.pci_device_id = (int) nvCtrlPciId & 0x0000FFFF;
        gpu_ids.bus_id = nvCtrlPciBusId;

        // TO BE REMOVED
        // printf("Obtaining GPU PCI ID from the NV_control extension : %d, %d\n", gpu_ids.pci_device_id, gpu_ids.bus_id);

        /* Closing the connection */
        XCloseDisplay(display);
        return gpu_ids;
    }

    /* Closing the connection */
    XCloseDisplay(display);
    return gpu_ids;
}

static struct display_info get_gpu_display(const struct gpu_info gpu_ids)
{
    struct display_info display;
    display.port = -1;
    display.device = -1;

    int x_server_max = 10;
    int x_screen_max = 10;

    int i,j;
    for (i = 0; i < x_server_max; ++i) {
        for (j = 0; j < x_screen_max; ++j) {

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
                // printf("DISPLAY= %s \n", x_display);

                return display;
            }
        }
    }

    // TO BE REMOVED
    // printf("No display was matching the GPU having PCI ID = %d %d \n", gpu_ids.pci_device_id, gpu_ids.bus_id);
    return display;
}

#endif

#endif /* HWLOC_GL_H */

