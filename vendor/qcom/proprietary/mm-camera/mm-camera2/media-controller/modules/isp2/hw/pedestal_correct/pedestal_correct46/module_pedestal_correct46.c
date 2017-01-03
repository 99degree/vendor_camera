/* module_pedestal_correct46.c
 *
 * Copyright (c) 2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/* std headers */
#include <stdio.h>

/* mctl headers */
#include "media_controller.h"
#include "mct_list.h"
#include "mct_module.h"
#include "mct_port.h"
#include "chromatix.h"

#undef ISP_DBG
#define ISP_DBG(fmt, args...) \
  ISP_DBG_MOD(ISP_LOG_PEDESTAL, fmt, ##args)
#undef ISP_HIGH
#define ISP_HIGH(fmt, args...) \
  ISP_HIGH_MOD(ISP_LOG_PEDESTAL, fmt, ##args)

/* isp headers */
#include "isp_sub_module_common.h"
#include "isp_common.h"
#include "isp_sub_module_log.h"
#include "isp_sub_module.h"
#include "isp_sub_module_port.h"
#include "pedestal_correct46.h"

/* TODO pass from Android.mk */
#define PEDESTAL_CORRECT46_VERSION "46"

#define PEDESTAL_CORRECT46_MODULE_NAME(n) \
  "pedestal_correct"n

static isp_sub_module_private_func_t pedestal_correct46_private_func = {
  .isp_sub_module_init_data              = pedestal46_init,
  .isp_sub_module_destroy                = pedestal46_destroy,

  .control_event_handler = {
    [ISP_CONTROL_EVENT_STREAMON]         = pedestal46_streamon,
    [ISP_CONTROL_EVENT_STREAMOFF]        = pedestal46_streamoff,
    [ISP_CONTROL_EVENT_SET_PARM]         = isp_sub_module_port_set_param,
  },

  .module_event_handler = {
    [ISP_MODULE_EVENT_SET_CHROMATIX_PTR] = pedestal46_set_chromatix_ptr,
    [ISP_MODULE_EVENT_STATS_AEC_UPDATE]  = pedestal46_stats_aec_update,
    [ISP_MODULE_EVENT_SET_STREAM_CONFIG] = pedestal46_set_stream_config,
    [ISP_MODULE_EVENT_ISP_PRIVATE_EVENT] =
      isp_sub_module_port_isp_private_event,
  },

  .isp_private_event_handler = {
    [ISP_PRIVATE_SET_MOD_ENABLE]         = isp_sub_module_port_enable,
    [ISP_PRIVATE_SET_TRIGGER_ENABLE]     = isp_sub_module_port_trigger_enable,
    [ISP_PRIVATE_SET_TRIGGER_UPDATE]     = pedestal46_trigger_update,
    [ISP_PRIVATE_SET_STRIPE_INFO]        = pedestal46_set_stripe_info,
    [ISP_PRIVATE_SET_STREAM_SPLIT_INFO]  = pedestal46_set_split_info,
  },

  .set_param_handler = {
    [ISP_SET_PARM_SET_VFE_COMMAND] = isp_sub_module_port_set_vfe_command,
    [ISP_SET_PARM_UPDATE_DEBUG_LEVEL]   = isp_sub_module_port_set_log_level,
    [ISP_SET_META_BLACK_LEVEL_LOCK] = isp_sub_module_set_manual_controls,
    [ISP_SET_PARM_SENSOR_HDR_MODE] = pedestal46_set_hdr_mode,
  },
};

/** module_pedestal_correct46_init:
 *
 *  @name: name of ISP module - "pedestal_correct46"
 *
 *  Initializes new instance of ISP module
 *
 *  create mct module for pedestal_correct
 *
 *  Return mct module handle on success or NULL on failure
 **/
static mct_module_t *module_pedestal_correct46_init(const char *name)
{
  boolean                ret = TRUE;
  mct_module_t          *module = NULL;
  isp_sub_module_priv_t *isp_sub_module_priv = NULL;

  ISP_HIGH("name %s", name);

  if (!name) {
    ISP_ERR("failed: name %s", name);
    return NULL;
  }

  if (strncmp(name, PEDESTAL_CORRECT46_MODULE_NAME(PEDESTAL_CORRECT46_VERSION), strlen(name))) {
    ISP_ERR("failed: invalid name %s expected %s", name,
      PEDESTAL_CORRECT46_MODULE_NAME(PEDESTAL_CORRECT46_VERSION));
    return NULL;
  }

  module = isp_sub_module_init(name, NUM_SINK_PORTS, NUM_SOURCE_PORTS,
    &pedestal_correct46_private_func, ISP_MOD_PEDESTAL, "pedestal",
    ISP_LOG_PEDESTAL);
  if (!module) {
    ISP_ERR("module is NULL");
    goto ERROR2;
  }
  if(!MCT_OBJECT_PRIVATE(module)){
    ISP_ERR("MCT_OBJECT_PRIVATE(module) is NULL");
    goto ERROR1;
  }

  return module;

ERROR1:
  mct_module_destroy(module);
ERROR2:
  ISP_ERR("failed");
  return NULL;
}

/** module_pedestal_correct46_deinit:
 *
 *  @module: isp module handle
 *
 *  Deinit isp module
 *
 *  Returns: void
 **/
static void module_pedestal_correct46_deinit(mct_module_t *module)
{
  if (!module) {
    ISP_ERR("failed: module %p", module);
    return;
  }

  isp_sub_module_deinit(module);
}

static isp_submod_init_table_t submod_init_table = {
  .module_init = module_pedestal_correct46_init,
  .module_deinit = module_pedestal_correct46_deinit,
};

/** module_open:
 *
 *  Return handle to isp_submod_init_table_t
 **/
isp_submod_init_table_t *module_open(void)
{
  return &submod_init_table;
}
