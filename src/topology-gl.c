/*
 * Copyright © 2012 Blue Brain Project, BBP/EPFL. All rights reserved.
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <hwloc/helper.h>
#include <stdarg.h>
#include <errno.h>
#include <hwloc/gl.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#ifdef HWLOC_HAVE_GL
static hwloc_obj_t _hwloc_gl_query_display(hwloc_topology_t, Display*, const int);
#endif
/*****************************************************************
 * Allocates a hwloc_obj_t of type HWLOC_OBJ_PCI_DEVICE and returns a
 * pointer to this object.
 ****************************************************************/
static hwloc_obj_t hwloc_gl_alloc_pcidev_object(void)
{
  struct hwloc_obj *obj = malloc(sizeof(*obj));
  memset(obj, 0, sizeof(*obj));
  obj->type = HWLOC_OBJ_PCI_DEVICE;
  obj->os_level = -1;
  obj->attr = malloc(sizeof(*obj->attr));
  memset(obj->attr, 0, sizeof(*obj->attr));
  return obj;
}

/*****************************************************************
 * Queries a display defined by its port and device numbers in the
 * string format ":[port].[device]", and
 * returns a hwloc_obj_t containg the desired pci parameters (bus,
 * device id, domain, function)
 ****************************************************************/
hwloc_obj_t hwloc_gl_query_display(hwloc_topology_t topology, char* displayName)
{
  hwloc_obj_t display_obj = NULL;
#ifdef HWLOC_HAVE_GL
  Display* display = XOpenDisplay(displayName);
  if (!display) {
    return display_obj;
  }

  display_obj = _hwloc_gl_query_display(topology, display, -1);
  XCloseDisplay(display);
#else
  printf("GL module is not loaded \n");
#endif
  return display_obj;
}

hwloc_obj_t _hwloc_gl_query_display(hwloc_topology_t topology, Display* display, const int screen)
{
  hwloc_obj_t display_obj = NULL;
#ifdef HWLOC_HAVE_GL
  int opcode, event, error;
  int default_screen_number = screen;
  unsigned int *ptr_binary_data;
  int data_lenght;
  int gpu_number;

  int num_pci_devices;
  int nv_ctrl_pci_bus;
  int nv_ctrl_pci_device;
  int nv_ctrl_pci_domain;
  int nv_ctrl_pci_func;

  int i;

  if (display == 0) {
    return display_obj;
  }

  /* Check for NV-CONTROL extension */
  if (!XQueryExtension(display, "NV-CONTROL", &opcode, &event, &error) )
    goto error;

  if( default_screen_number < 0 )
    default_screen_number = DefaultScreen(display);

  /* Gets the GPU number attached to the default screen. */
  /* For further details, see the <NVCtrl/NVCtrlLib.h> */
  if (!XNVCTRLQueryTargetBinaryData (display, NV_CTRL_TARGET_TYPE_X_SCREEN, default_screen_number, 0,
                                     NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                                     (unsigned char **) &ptr_binary_data, &data_lenght) )
    goto error;

  gpu_number = ptr_binary_data[1];
  free(ptr_binary_data);

  /* Gets the ID's of the GPU defined by gpu_number
   * For further details, see the <NVCtrl/NVCtrlLib.h> */
  if (!XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                   NV_CTRL_PCI_BUS, &nv_ctrl_pci_bus) ||
      !XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                   NV_CTRL_PCI_ID, &nv_ctrl_pci_device) ||
      !XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                   NV_CTRL_PCI_DOMAIN, &nv_ctrl_pci_domain) ||
      !XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
                                   NV_CTRL_PCI_FUNCTION, &nv_ctrl_pci_func) ) {
      goto error;
  }
  if (topology != NULL) {
    /* This part only works if the I/O and BRIDGES flags are set */
    num_pci_devices = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);
    if (num_pci_devices > 0)
    {
      for (i = 0; i < num_pci_devices; ++i)
      {
        display_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PCI_DEVICE, i);

        /* For further details, see the <NVCtrl/NVCtrlLib.h> */
        if ((int) display_obj->attr->pcidev.bus == (int) nv_ctrl_pci_bus &&
            (int) display_obj->attr->pcidev.device_id == ((int) nv_ctrl_pci_device & 0x0000FFFF) &&
            (int) display_obj->attr->pcidev.domain == (int) nv_ctrl_pci_domain &&
            (int) display_obj->attr->pcidev.func == (int) nv_ctrl_pci_func) {
            goto success;
        }
      }
    }
  }
  else {
    display_obj = hwloc_gl_alloc_pcidev_object();

    display_obj->attr->pcidev.bus = nv_ctrl_pci_bus;
    display_obj->attr->pcidev.device_id = nv_ctrl_pci_device & 0x0000FFFF;
    display_obj->attr->pcidev.domain = nv_ctrl_pci_domain;
    display_obj->attr->pcidev.func = nv_ctrl_pci_func;
    goto success;
  }

  error:
  fprintf(stderr, "Failed to query the attached GPUs\n");

  success:
#else
  printf("GL module is not loaded \n");
#endif
  return display_obj;
}

/*****************************************************************
 * Returns a DISPLAY for a given GPU defined by its pcidev_obj
 * which contains the pci info required for doing the matching a
 * pci device in the topology.
 ****************************************************************/
int hwloc_gl_get_gpu_display(hwloc_topology_t topology, hwloc_obj_t pcidev_obj, unsigned *port, unsigned *device)
{
#ifdef HWLOC_HAVE_GL
  hwloc_obj_t query_display_obj;

  /* Try the first 10 servers with 10 screens */
  /* For each x server, if the first x screen fails move to the next x server */
  unsigned x_server_max = 10;
  unsigned x_screen_max = 10;
  unsigned i,j;

  for (i = 0; i < x_server_max; ++i) {
    char x_display [8];
    Display* display = NULL;

    snprintf(x_display,sizeof(x_display),":%d", i);
    display = XOpenDisplay(displayName);
    if (!display)
      continue;

    for (j = 0; j < x_screen_max; ++j) {

      /* Retrieve an object matching the x_display */
      query_display_obj = _hwloc_gl_query_display(topology, display, j);

      if (query_display_obj == NULL)
        break;

      if (query_display_obj->attr->pcidev.bus == pcidev_obj->attr->pcidev.bus &&
          query_display_obj->attr->pcidev.device_id == pcidev_obj->attr->pcidev.device_id &&
          query_display_obj->attr->pcidev.domain == pcidev_obj->attr->pcidev.domain &&
          query_display_obj->attr->pcidev.func == pcidev_obj->attr->pcidev.func) {

        *port = i;
        *device = j;

        return 0;
      }
    }
    XCloseDisplay( display );
  }
#endif
  return -1;
}

/*****************************************************************
 * Returns the DISPLAY parameters for a given pcidev_obj.
 * Note: This function doesn't need to have an input topology and
 * is just used for adding the display parameters in the topology
 * created by running the "lstop" utility.
 ****************************************************************/
int hwloc_gl_get_gpu_display_private(hwloc_obj_t pcidev_obj, unsigned *port, unsigned *device)
{
  return hwloc_gl_get_gpu_display(NULL, pcidev_obj, port, device);
}

/*****************************************************************
 * Returns a hwloc_obj_t HWLOC_OBJ_PCI_DEVICE representing the GPU
 * connected to a display defined by its port and device
 * parameters.
 * Returns NULL if no GPU was connected to the give port and
 * device or for non exisiting display.
 ****************************************************************/
hwloc_obj_t hwloc_gl_get_gpu_by_display(hwloc_topology_t topology, int port, int device)
{
  char x_display [10];
  hwloc_obj_t display_obj;

  /* Formulate the display string */
  snprintf(x_display, sizeof(x_display), ":%d.%d", port, device);
  display_obj = hwloc_gl_query_display(topology, x_display);

  if (display_obj != NULL)
    return display_obj;
  else
    return NULL;
}

/*****************************************************************
 * Returns a cpuset of the socket attached to the host bridge
 * where the GPU defined by the pcidev_obj is connected in the
 * topology.
 ****************************************************************/
int hwloc_gl_get_pci_cpuset(hwloc_topology_t topology, hwloc_obj_t pcidev_obj, hwloc_bitmap_t* cpuset)
{
  int i;
  int pci_dev_count; /* The number of PCI devices in the topology */

  pci_dev_count = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);
  *cpuset = hwloc_bitmap_alloc();

  for (i = 0; i < pci_dev_count; ++i) {
    /* PCI device object */
    hwloc_obj_t pci_dev_object;
    pci_dev_object = hwloc_get_obj_by_type (topology, HWLOC_OBJ_PCI_DEVICE, i);

    /* PCI device ID */
    if (pcidev_obj->attr->pcidev.bus == pci_dev_object->attr->pcidev.bus &&
        pcidev_obj->attr->pcidev.device_id == pci_dev_object->attr->pcidev.device_id &&
        pcidev_obj->attr->pcidev.domain == pci_dev_object->attr->pcidev.domain &&
        pcidev_obj->attr->pcidev.func == pci_dev_object->attr->pcidev.func) {

      /* Host bridge of the PCI device */
      hwloc_obj_t host_bridge;
      host_bridge = hwloc_get_hostbridge_by_pcibus (topology,
                                                    pci_dev_object->attr->pcidev.domain,
                                                    pci_dev_object->attr->pcidev.bus);

      /* Get the cpuset of the socket attached to host
       * bridge at which the PCI device is connected */
      *cpuset = hwloc_bitmap_dup(host_bridge->prev_sibling->cpuset);
      return 0;
    }
  }

  /* If not existing in the topology */
  hwloc_bitmap_zero(*cpuset);
  return -1;
}

/*****************************************************************
 * Returns the cpuset of the socket connected to the host bridge
 * connecting the GPU attached to the display defined by the
 * input port and device integers and having the generic format
 * [:][port][.][device] or the format [:][server][.][screen] under
 * X systems.
 * It returns empty (zero) cpuset for an invalid display.
 ****************************************************************/
int hwloc_gl_get_display_cpuset(hwloc_topology_t topology, int port, int device, hwloc_bitmap_t* cpuset)
{
  char x_display [10];
  hwloc_obj_t display_obj;
  int err;

  /* Formulate the display string */
  snprintf(x_display, sizeof(x_display), ":%d.%d", port, device);
  display_obj = hwloc_gl_query_display(topology, x_display);

  if (display_obj != NULL) {
    err = hwloc_gl_get_pci_cpuset(topology, display_obj, cpuset);

    if (!err)
      return 0;
  }
  else
    /* If the gl module was not enabled or wrong display */
    /* Allocate the cpuset and initialize it to zeros in case of failure */
    *cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_zero(*cpuset);
    return -1;
}

/*****************************************************************
 * Retrieves the cpuset of the socket connected to the host bridge
 * connecting the pci device defined by pcidev_obj.
 * It returns 0 if a valid cpuset is retrieved, else -1.
 ****************************************************************/
int hwloc_get_pcidevice_cpuset(hwloc_topology_t topology, hwloc_obj_t pcidev_obj, hwloc_bitmap_t* cpuset)
{
  int err;
  err = hwloc_gl_get_pci_cpuset(topology, pcidev_obj, cpuset);

  if (!err)
    return 0;
  else
    return -1;
}
