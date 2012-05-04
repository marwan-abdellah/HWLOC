/*
 * Copyright © 2012 Blue Brain Project, EPFL. All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_GL_H
#define HWLOC_GL_H

#include <hwloc.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct struct_display_info
{
    int port;   /* Port (X-server or ignored) */
    int device; /* Device (X-screen, later Windows GPU affinity device) */
};

typedef struct struct_display_info display_info;

struct struct_pci_dev_info
{
    int pci_device; /* The PCI device number the specified device is using */
    int pci_bus; /* The PCI bus number the specified device is using */
    int pci_function; /* the PCI function number the specified device is using*/
    int pci_domain; /* The PCI domain number the specified device is using */
};

typedef struct struct_pci_dev_info pci_dev_info;

/** \brief Returns a DISPLAY for a given GPU defined by its pci_dev_info..
 *
 * The returned structure should have the format
 * "[:][port][.][device]"
 */
HWLOC_DECLSPEC display_info get_gpu_display(const pci_dev_info pci_info);

/** \brief Returns the cpuset of the socket connected to the
 * host bridge connecting the GPU attached to the display
 * defined by the input port and device integers and having
 * the format "[:][port][.][device]" under X systems
 *
 * The returned structure will have the format
 * "[:][port][.][device]"
 */
HWLOC_DECLSPEC hwloc_bitmap_t get_display_cpuset(const hwloc_topology_t topology, const int port, const int device);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_GL_H */

