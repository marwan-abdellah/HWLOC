/*
 * Copyright © 2012 Blue Brain Project, EPFL. All rights reserved.
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

struct display_info
{
  int port;   /* Port or X-server */
  int device; /* Device or X-screen */
};

struct gpu_info
{
  int pci_device_id; /* PCI device ID */
  int bus_id; /* PCI device bus ID*/
};

/* A struct holding info about the PCI device */
struct pci_dev_info
{
  char* name; /* Name of the PCI device */
  char* type; /* Type of the PCI device */
  int device_id;  /* ID of the PCI device */
  int bus_id; /* PCI Bus ID of the device */
  int is_gpu; /* Flag used for printing the corresponding display */
  struct display_info display; /* Display parameters "port & device" */
  hwloc_bitmap_t socket_cpuset; /* CPU set of the socket connected to the PCI device */
};

/*****************************************************************
 * Queries a display defined by its port and device numbers, and
 * returns the PCI ID and bus ID of the GPU connected to it.
 ****************************************************************/
HWLOC_DECLSPEC struct gpu_info query_display(char* displayName);


/*****************************************************************
 * Returns a DISPLAY for a given GPU.
 * The returned structure should have the format
 * "[:][port][.][device]"
 ****************************************************************/
HWLOC_DECLSPEC struct display_info get_gpu_display(const struct gpu_info gpu_ids);


/*****************************************************************
 * Returns a cpuset of the socket attached to the host bridge
 * where the GPU defined by gpu_ids is connected in the topology.
 ****************************************************************/
HWLOC_DECLSPEC hwloc_bitmap_t get_gpu_cpuset(const hwloc_topology_t topology, const struct gpu_info gpu_ids);

/*****************************************************************
 * Returns the cpuset of the socket connected to the host bridge
 * connecting the GPU attached to the display defined by the
 * input port and device integers and having the format
 * [:][port][.][device] under X systems
 ****************************************************************/
HWLOC_DECLSPEC hwloc_bitmap_t get_display_cpuset(const hwloc_topology_t topology, const int port, const int device);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_GL_H */
