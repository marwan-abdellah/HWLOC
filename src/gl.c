/*
 * Copyright © 2012 Blue Brain Project, EPFL. All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <hwloc/helper.h>
#include <stdarg.h>
#include <errno.h>

#ifdef HWLOC_HAVE_GL
#include <hwloc/gl.h>
#endif

/*****************************************************************
 * Queries a display defined by its port and device numbers, and
 * returns a   having unique identifiers for the GPU
 * connected to it.
 ****************************************************************/

static pci_dev_info query_display(char* displayName)
{
#ifdef HWLOC_HAVE_GL
    Display* display;
    int opcode, event, error;
    int default_screen_number;  /* Default screen number */
    unsigned int *ptr_binary_data;
    int data_lenght;
    int sucess;
    int gpu_number;

    int nv_ctrl_pci_bus;
    int nv_ctrl_pci_device;
    int nv_ctrl_pci_domain;
    int nv_ctrl_pci_func;

    int success;
    int success_info;
#endif

    /* Initializing the pci_dev_info to -1's in case of failure */
    pci_dev_info dev_info;
    dev_info.pci_device = -1;
    dev_info.pci_function = -1;
    dev_info.pci_bus = -1;
    dev_info.pci_domain = -1;

#ifdef HWLOC_HAVE_GL
    display = XOpenDisplay(displayName);
    if (display == 0) {
        return dev_info;
    }

    /* Check for NV-CONTROL extension */
    if( !XQueryExtension(display, "NV-CONTROL", &opcode, &event, &error))
    {
        XCloseDisplay( display);
        return dev_info;
    }

    default_screen_number = DefaultScreen(display);

    /* Gets the GPU number attached to the default screen.
     * For further details, see the <NVCtrl/NVCtrlLib.h> */
    sucess = XNVCTRLQueryTargetBinaryData(display, NV_CTRL_TARGET_TYPE_X_SCREEN, default_screen_number, 0,
                                          NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                                          (unsigned char **) &ptr_binary_data, &data_lenght);
    if (!sucess) {
        fprintf(stderr, "Failed to query the GPUs attached to the default screen \n");

        /* Closing the connection */
        XCloseDisplay(display);
        return dev_info;
    }

    success = XNVCTRLQueryTargetBinaryData (display, NV_CTRL_TARGET_TYPE_X_SCREEN, default_screen_number, 0,
                                            NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                                            (unsigned char **) &ptr_binary_data, &data_lenght);
    if (success) {
        gpu_number = ptr_binary_data[1];
        free(ptr_binary_data);

        /* Gets the ID's of the GPU defined by gpu_number
* For further details, see the <NVCtrl/NVCtrlLib.h> */
        success_info = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                                   NV_CTRL_PCI_BUS, &nv_ctrl_pci_bus);
        if (success_info)
        {
            success_info = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                                       NV_CTRL_PCI_ID, &nv_ctrl_pci_device);
            if (success_info)
            {
                success_info = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                                           NV_CTRL_PCI_DOMAIN, &nv_ctrl_pci_domain);
                if (success_info)
                {
                    success_info = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                                               NV_CTRL_PCI_FUNCTION, &nv_ctrl_pci_func);
                    dev_info.pci_bus = (int) nv_ctrl_pci_bus;
                    /* For further details, see the <NVCtrl/NVCtrlLib.h> */
                    dev_info.pci_device = (int) nv_ctrl_pci_device & 0x0000FFFF;
                    dev_info.pci_domain = (int) nv_ctrl_pci_domain;
                    dev_info.pci_function = (int) nv_ctrl_pci_func;

                }
                else
                {
                    XCloseDisplay(display);
                    return dev_info;
                }
            }
            else
            {
                XCloseDisplay(display);
                return dev_info;
            }
        }
        else
        {
            XCloseDisplay(display);
            return dev_info;
        }
    }
    else
        fprintf(stderr, "Failed to query the GPUs attached to the default screen \n");

    XCloseDisplay(display);
    return dev_info;

#else
    fprint("GL module is not loaded \n");
    return dev_info;
#endif
}


/*****************************************************************
 * Returns a DISPLAY for a given GPU defined by its pci_dev_info.
 * The returned  ure should have the formatget_gpu_display
 * "[:][port][.][device]"
 ****************************************************************/
display_info get_gpu_display(const pci_dev_info pci_info)
{
    display_info display;
    int x_server_max;
    int x_screen_max;
    int i,j;
    pci_dev_info query_pci_info;

    /* Return -1's in case of failure */
    display.port = -1;
    display.device = -1;

    /* Try the first 10 servers with 10 screens */
    x_server_max = 10;
    x_server_max = 10;

    for (i = 0; i < x_server_max; ++i) {
        for (j = 0; j < x_screen_max; ++j) {

            /* Set the display name with the format "[:][x_server][.][x_screen]" */
            char x_display [10];
            snprintf(x_display,sizeof(x_display),":%d.%d", i, j);

            query_pci_info = query_display(x_display);
            if (query_pci_info.pci_bus == pci_info.pci_bus &&
                    query_pci_info.pci_device == pci_info.pci_device &&
                    query_pci_info.pci_domain == pci_info.pci_domain &&
                    query_pci_info.pci_function == pci_info.pci_function
                    ) {
                display.port = i;
                display.device = j;
                return display;
            }

            /* Connection failure This is a double check */
            if (query_pci_info.pci_bus == -1 ||
                    query_pci_info.pci_device == -1 ||
                    query_pci_info.pci_domain == -1 ||
                    query_pci_info.pci_function == -1
                    ) /* No X server on port/device */
                break;
        }
    }
    return display;
}

/*****************************************************************
 * Returns a cpuset of the socket attached to the host bridge
 * where the GPU defined by defined by its pci_dev_info is
 * connected in the topology.
 ****************************************************************/
hwloc_bitmap_t get_pci_cpuset(const hwloc_topology_t topology, const pci_dev_info pci_info)
{
    int i;
    int pci_dev_count; /* The number of PCI devices in the topology */
    pci_dev_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);

    for (i = 0; i < pci_dev_count; ++i) {
        /* PCI device object */
        hwloc_obj_t pci_dev_object;
        pci_dev_object = hwloc_get_obj_by_type (topology, HWLOC_OBJ_PCI_DEVICE, i);

        /* PCI device ID */
        if (pci_info.pci_bus == pci_dev_object->attr->pcidev.bus &&
                pci_info.pci_device == pci_dev_object->attr->pcidev.device_id &&
                pci_info.pci_domain == pci_dev_object->attr->pcidev.domain &&
                pci_info.pci_function == pci_dev_object->attr->pcidev.func
                ) {

            /* Host bridge of the PCI device */
            hwloc_obj_t host_bridge;
            host_bridge = hwloc_get_hostbridge_by_pcibus (topology,
                                                          pci_dev_object->attr->pcidev.domain,
                                                          pci_dev_object->attr->pcidev.bus);

            /* Get the cpuset of the socket attached to host bridge
            * at which the PCI device is connected */
            return hwloc_bitmap_dup(host_bridge->prev_sibling->cpuset);
        }
    }

    /* If not existing in the topology */
    return NULL;
}

/*****************************************************************
 * Returns the cpuset of the socket connected to the host bridge
 * connecting the GPU attached to the display defined by the
 * input port and device integers and having the format
 * [:][port][.][device] under X systems
 ****************************************************************/
hwloc_bitmap_t get_display_cpuset(const hwloc_topology_t topology, const int port, const int device)
{
    char x_display [10];
    pci_dev_info pci_info;
    hwloc_bitmap_t cpuset;

    snprintf(x_display,sizeof(x_display),":%d.%d", port, device);
    pci_info = query_display(x_display);
    cpuset = get_pci_cpuset(topology, pci_info);
    return hwloc_bitmap_dup(cpuset);
}

