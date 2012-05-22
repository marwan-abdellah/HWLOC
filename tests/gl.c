/*
 * Copyright © 2012 Blue Brain Project, BBP/EPFL. All rights reserved.
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <hwloc/gl.h>
#include <hwloc/helper.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

int main(void)
{
  hwloc_topology_t topology;
  unsigned long loading_flags;
  hwloc_obj_t pcidev_obj;
  int success;
  char* cpuset_string;
  unsigned number_pci_devices;
  unsigned number_gpus;
  unsigned i;

  hwloc_topology_init(&topology); /* Topology initialization */

  /* Flags used for loading the I/O devices, bridges and their relevant info */
  loading_flags = HWLOC_TOPOLOGY_FLAG_IO_BRIDGES | HWLOC_TOPOLOGY_FLAG_IO_DEVICES;

  /* Set discovery flags */
  success = hwloc_topology_set_flags(topology, loading_flags);

  /* If flags not set */
  if (success < 0)
    printf("hwloc_topology_set_flags() failed, PCI devices will not be loaded in the topology \n");

  /* Perform topology detection */
  hwloc_topology_load(topology);

  /* Case 1: Get the cpusets of the sockets connecting the PCI devices in the topology */
  number_pci_devices = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);

  for (i = 0; i < number_pci_devices; ++i) {
      hwloc_obj_t pcidev_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PCI_DEVICE, i);

      hwloc_bitmap_t cpuset ;
      //cpuset = hwloc_bitmap_alloc();
      //hwloc_bitmap_zero(cpuset);
      int err = hwloc_get_pcidevice_cpuset(topology, pcidev_obj, &cpuset);

      /* Print the cpuset corresponding to each pci device */
      hwloc_bitmap_asprintf(&cpuset_string, cpuset);

      printf(" %s | %s \n", cpuset_string, pcidev_obj->name);
      free(cpuset_string);
    }

  /* Case 2: Get the number of connected GPUs in the topology and their attached displays */
  number_gpus = 0;
  for (i = 0; i < number_pci_devices; ++i) {
      hwloc_obj_t pcidev_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PCI_DEVICE, i);
      unsigned port, device;
      int err;

      /* Check the returning port and devices */
      err = hwloc_gl_get_gpu_display(topology, pcidev_obj, &port, &device);

      if (!err) {
          number_gpus++;
          printf("GPU # %d is connected to DISPLAY:%u.%u \n", number_gpus, port, device);
        }
    }

  /* Case 3: Get the GPU connected to a valid display defined by its port and device */
  pcidev_obj = hwloc_gl_get_gpu_by_display(topology, 0, 0);
  if (pcidev_obj != NULL)
    printf("GPU %s is connected to DISPLAY:%u.%u \n", pcidev_obj->name, 0, 0);
  else
    printf("No GPU connected to DISPLAY:%u.%u \n", 0, 0);

  return 0;
}
