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
 * returns the bus ID of the GPU connected to it.
 ****************************************************************/
static int query_display(char* displayName)
{
  /* Bus ID of the GPU connected to the input display */
  int bus_id = -1;

#ifdef HWLOC_HAVE_GL
  Display* display = XOpenDisplay(displayName);
  if (display == 0) {
      return bus_id;
  }
  /* Check for NV-CONTROL extension */
  int opcode, event, error;
  if( !XQueryExtension(display, "NV-CONTROL", &opcode, &event, &error))
  {
      XCloseDisplay( display);
      return bus_id;
  }

  /* Default screen number */
  const int default_screen_number = DefaultScreen(display);

  /* Gets the GPU number attached to the default screen.
   * For further details, see the <NVCtrl/NVCtrlLib.h> */
  unsigned int *ptr_binary_data;
  int data_lenght;

  const int sucess = XNVCTRLQueryTargetBinaryData (display, NV_CTRL_TARGET_TYPE_X_SCREEN, default_screen_number, 0,
                                                   NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                                                   (unsigned char **) &ptr_binary_data, &data_lenght);
  if (!sucess) {
      fprintf(stderr, "Failed to query the GPUs attached to the default screen \n");

      /* Closing the connection */
      XCloseDisplay(display);
      return bus_id;
  }

  const int success = XNVCTRLQueryTargetBinaryData (display, NV_CTRL_TARGET_TYPE_X_SCREEN, default_screen_number, 0,
                                                    NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                                                    (unsigned char **) &ptr_binary_data, &data_lenght);
  if (success) {
      const int gpu_number = ptr_binary_data[1];
      free(ptr_binary_data);

      /* Gets the bus ID of the GPU defined by gpu_number
       * For further details, see the <NVCtrl/NVCtrlLib.h> */
      int nvCtrlPciBusId;
      const int success_bus = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                                          NV_CTRL_PCI_BUS, &nvCtrlPciBusId);
      if (success_bus) {
        bus_id = nvCtrlPciBusId;
      }
  else
    fprintf(stderr, "Failed to query the GPUs attached to the default screen \n");
  }

  /* Closing the connection */
  XCloseDisplay(display);
  return bus_id;
#else
  fprintf(stderr, "GL module is not supported \n");
  return bus_id;
#endif
}

/*****************************************************************
 * Returns a DISPLAY for a given GPU.
 * The returned structure should have the formatget_gpu_display
 * "[:][port][.][device]"
 ****************************************************************/
struct display_info get_gpu_display(const int bus_id)
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
          snprintf(x_display,sizeof(x_display),":%d.%d", i, j);


          const query_bus_id = query_display(x_display);
          if (query_bus_id == bus_id) {
            display.port = i;
            display.device = j;
            return display;
          }

          /* Connection failure */
          if (query_bus_id == -1) /* No X server on port/device */
            break;
      }
  }
  return display;
}

/*****************************************************************
 * Returns a cpuset of the socket attached to the host bridge
 * where the GPU defined by gpu_ids is connected in the topology.
 ****************************************************************/
hwloc_bitmap_t get_pci_cpuset(const hwloc_topology_t topology, const int bus_id)
{
    /* The number of PCI devices in the topology */
    const int pci_dev_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);

    int i;
    for (i = 0; i < pci_dev_count; ++i) {
        /* PCI device object */
        const hwloc_obj_t pci_dev_object = hwloc_get_obj_by_type (topology, HWLOC_OBJ_PCI_DEVICE, i);

        /* PCI device ID */
        if (bus_id == pci_dev_object->attr->pcidev.bus) {

             /* Host bridge of the PCI device */
            const hwloc_obj_t host_bridge = hwloc_get_hostbridge_by_pcibus (topology,
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
    snprintf(x_display,sizeof(x_display),":%d.%d", port, device);

    const int gpu_bus_id = query_display(x_display);
    const hwloc_bitmap_t cpuset = get_pci_cpuset(topology, gpu_bus_id);

    return hwloc_bitmap_dup(cpuset);
}

