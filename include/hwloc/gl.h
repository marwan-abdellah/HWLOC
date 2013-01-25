/*
 * Copyright © 2012 Blue Brain Project, EPFL. All rights reserved.
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_GL_H
#define HWLOC_GL_H

#include <hwloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Queries a display defined by its port and device in the
 * string format ":[port].[device]", and returns a hwloc_obj_t of
 * type HWLOC_OBJ_PCI_DEVICE containg the desired pci parameters
 * (bus,device id, domain, function) representing the GPU connected
 * to this display.
 */
HWLOC_DECLSPEC hwloc_obj_t hwloc_gl_query_display(hwloc_topology_t topology, char* displayName);

/** \brief Retrieves a cpuset of the socket attached to the host bridge
 * where the GPU defined by pcidev_obj is connected in the topology.
 * It returns 0 if a valid cpuset is retrieved, else -1.
 */
HWLOC_DECLSPEC int hwloc_gl_get_pci_cpuset(hwloc_topology_t topology, hwloc_obj_t pcidev_obj, hwloc_bitmap_t* cpuset);

/** \brief Returns a DISPLAY for a given GPU defined by pcidev_obj.
 */
HWLOC_DECLSPEC int hwloc_gl_get_gpu_display(hwloc_topology_t topology, hwloc_obj_t pcidev_obj, unsigned *port, unsigned *device);

/** \brief Returns the DISPLAY parameters for a given pcidev_obj.
 * Note: This function doesn't need to have an input topology and
 * is just used for adding the display parameters in the topology
 * created by running the "lstop" utility.
 */
HWLOC_DECLSPEC int hwloc_gl_get_gpu_display_private(hwloc_obj_t pcidev_obj, unsigned *port, unsigned *device);

/** \brief Returns an object of type HWLOC_OBJ_PCI_DEVICE
 * representing the GPU connected to the display defined by
 * its port and device.
 */
HWLOC_DECLSPEC hwloc_obj_t hwloc_gl_get_gpu_by_display(hwloc_topology_t topology, int port, int device);

/** \brief Retrieves the cpuset of the socket connected to
 * the host bridge connecting the pci device defined by
 * pcidev_obj.
 * It returns 0 if a valid cpuset is retrieved, else -1.
 */
HWLOC_DECLSPEC int hwloc_gl_get_display_cpuset(hwloc_topology_t topology, int port, int device, hwloc_bitmap_t* cpuset);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_GL_H */
