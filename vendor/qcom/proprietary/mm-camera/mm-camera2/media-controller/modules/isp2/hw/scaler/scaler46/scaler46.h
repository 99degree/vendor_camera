/* scaler46.h
 *
 * Copyright (c) 2012-2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __SCALER46_H__
#define __SCALER46_H__

/* isp headers */
#include "module_scaler46.h"

boolean scaler46_init(scaler46_t *mod);

boolean scaler46_handle_isp_private_event(scaler46_t *scaler,
  void *data, uint32_t identity);

void scaler46_calculate_dis(scaler46_t *scaler);
boolean scaler46_config(scaler46_t *scaler);

void scaler46_reset(scaler46_t *mod);

boolean scaler46_trigger_update(scaler46_t *mod);

boolean scaler46_handle_set_chromatix_ptr(scaler46_t *scaler,
  mct_event_t *event);

boolean scaler46_parm_zoom(mct_module_t *module, scaler46_t *scaler,
  int32_t *zoom_val, uint32_t identity);

boolean scaler46_parm_crop_region(mct_module_t *module, scaler46_t *scaler,
  cam_crop_region_t *crop_window, uint32_t identity);

#endif //__SCALER46_H__
