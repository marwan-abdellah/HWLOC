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

struct hwloc_gl_display_info
{
  int port;   /* Port (X-server or ignored) */
  int device; /* Device (X-screen, later Windows GPU affinity device) */
} ;

typedef struct hwloc_gl_display_info* hwloc_gl_display_info_t;

/** \brief Queries a display defined by its port and device in the
 * string format ":[port].[device]", and returns a hwloc_obj_t of
 * type HWLOC_OBJ_PCI_DEVICE containg the desired pci parameters
 * (bus,device id, domain, function) representing the GPU connected
 * to this display.
 */
HWLOC_DECLSPEC hwloc_obj_t hwloc_gl_query_display(hwloc_topology_t topology, char* displayName);

/** \brief Returns a cpuset of the socket attached to the host bridge
 * where the GPU defined by pcidev_obj is connected in the topology.
 */
HWLOC_DECLSPEC hwloc_bitmap_t hwloc_gl_get_pci_cpuset(hwloc_topology_t topology, const hwloc_obj_t pcidev_obj);

/** \brief Returns a DISPLAY for a given GPU defined by pcidev_obj.
 */
HWLOC_DECLSPEC hwloc_gl_display_info_t hwloc_gl_get_gpu_display(hwloc_topology_t topology, const hwloc_obj_t pcidev_obj);

/** \brief Returns the DISPLAY parameters for a given pcidev_obj.
 * Note: This function doesn't need to have an input topology and
 * is just used for adding the display parameters in the topology
 * created by running the "lstop" utility.
 */
hwloc_gl_display_info_t hwloc_gl_get_gpu_display_private(const hwloc_obj_t pcidev_obj);

/** \brief Returns an object of type HWLOC_OBJ_PCI_DEVICE
 * representing the GPU connected to the display defined by
 * its port and device.
 */
HWLOC_DECLSPEC hwloc_obj_t hwloc_gl_get_gpu_by_display(hwloc_topology_t topology, const int port, const int device);

/** \brief Returns the cpuset of the socket connected to the
 * host bridge connecting the GPU attached to the display
 * defined by its input port and device.
 */
HWLOC_DECLSPEC hwloc_bitmap_t hwloc_gl_get_display_cpuset(hwloc_topology_t topology, const int port, const int device);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_GL_H */

