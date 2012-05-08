/*
 * Copyright © 2012 Blue Brain Project, BBP/EPFL. All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <hwloc/gl.h>
#include <hwloc/helper.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

int main()
{
  hwloc_topology_t topology;
  unsigned long loading_flags;
  int number_pci_devices;
  int success;
  char* cpuset_string;

  hwloc_topology_init(&topology); /* Topology initialization */

  /* Flags used for loading the I/O devices, bridges and their relevant info */
  loading_flags = 0x0000000000000000 ^ HWLOC_TOPOLOGY_FLAG_IO_BRIDGES ^ HWLOC_TOPOLOGY_FLAG_IO_DEVICES;

  /* Set discovery flags */
  success = hwloc_topology_set_flags(topology, loading_flags);

  /* If flags not set */
  if (success < 0)
    printf("hwloc_topology_set_flags() failed, PCI devices will not be loaded in the topology \n");

  /* Perform topology detection */
  hwloc_topology_load(topology);

  /* Case 1: Get the cpusets of the sockets connecting the PCI devices in the topology */
  number_pci_devices = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);

  for (int i = 0; i < number_pci_devices; ++i) {
      hwloc_obj_t pcidev_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PCI_DEVICE, i);

      hwloc_bitmap_t cpuset ;
      cpuset = hwloc_bitmap_alloc();
      hwloc_bitmap_zero(cpuset);
      cpuset = hwloc_get_pcidevice_cpuset(topology, pcidev_obj);

      /* Print the cpuset corresponding to each pci device */
      hwloc_bitmap_asprintf(&cpuset_string, cpuset);

      printf(" %s | %s \n", cpuset_string, pcidev_obj->name);
      free(cpuset_string);
    }

  /* Case 2: Get the number of connected GPUs in the topology and their attached displays */
  int number_gpus = 0;
  for (int i = 0; i < number_pci_devices; ++i) {
      hwloc_obj_t pcidev_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PCI_DEVICE, i);

      /* Check the returning port and devices */
      hwloc_gl_display_info_t display_info = hwloc_gl_get_gpu_display(topology, pcidev_obj);

      if (display_info->port > -1 && display_info->device > -1) {
          number_gpus++;
          printf("GPU # %d is connected to DISPLAY:%d.%d \n", number_gpus, display_info->port, display_info->device);
        }
    }

  /* Case 3: Get the GPU connected to a valid display defined by its port and device */
  hwloc_obj_t pcidev_obj = hwloc_gl_get_gpu_by_display(topology, 0, 0);
  if (pcidev_obj != NULL)
    printf("GPU %s is connected to DISPLAY:%d.%d \n", pcidev_obj->name, 0, 0);
  else
    printf("No GPU connected to DISPLAY:%d.%d \n", 0, 0);

  return 0;
}
