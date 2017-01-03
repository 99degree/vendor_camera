/* awb_port.c
 *
 * Copyright (c) 2013-2015 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "awb_port.h"
#include "awb.h"

#include "q3a_thread.h"
#include "q3a_port.h"
#include "mct_stream.h"
#include "mct_module.h"
#include "modules.h"
#include "stats_util.h"
#include "stats_event.h"
#include "awb_biz.h"
#include "awb_ext.h"

/* Every AWB sink port ONLY corresponds to ONE session */

static q3a_thread_aecawb_msg_t* awb_port_malloc_msg(int msg_type,
  int param_type)
{
  q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
    malloc(sizeof(q3a_thread_aecawb_msg_t));

  if (awb_msg == NULL) {
    return NULL;
  }
  memset(awb_msg, 0 , sizeof(q3a_thread_aecawb_msg_t));

  awb_msg->type = msg_type;
  if (msg_type == MSG_AWB_SET || msg_type == MSG_AWB_SEND_EVENT) {
    awb_msg->u.awb_set_parm.type = param_type;
  } else if (msg_type == MSG_AWB_GET) {
    awb_msg->u.awb_get_parm.type = param_type;
  }
  return awb_msg;
}

/* Every AWB sink port ONLY corresponds to ONE session */


boolean awb_port_dummy_set_params(awb_set_parameter_t *param,
    awb_output_data_t *output, void *awb_obj)
{
  (void)param;
  (void)output;
  (void)awb_obj;
  AWB_ERR("Error: Uninitialized interface been use");
  return FALSE;
}

boolean awb_port_dummy_get_params(awb_get_parameter_t *param,
  void *awb_obj)
{
  (void)param;
  (void)awb_obj;
  AWB_ERR("Error: Uninitialized interface been use");
  return FALSE;
}

void awb_port_dummy_process(stats_t *stats,
  void *awb_obj, awb_output_data_t *output)
{
  (void)stats;
  (void)awb_obj;
  (void)output;
  AWB_ERR("Error: Uninitialized interface been use");
  return;
}

boolean awb_port_dummy_estimate_cct_by_gains(void *awb_obj,
  float r_gain, float g_gain, float b_gain, float *cct)
{
  (void)awb_obj;
  (void)r_gain;
  (void)g_gain;
  (void)b_gain;
  (void)cct;
  AWB_ERR("Error: Uninitialized interface been use");
  return FALSE;
}

boolean awb_port_dummy_estimate_ccm_by_cct (void *awb_obj,
  float cct, awb_ccm_type* ccm)
{
  (void)awb_obj;
  (void)cct;
  (void)ccm;
  AWB_ERR("Error: Uninitialized interface been use");
  return FALSE;
}

boolean awb_port_dummy_estimate_gains_by_cct(void *awb_obj,
  float* r, float* g, float* b, float cct)
{
  (void)awb_obj;
  (void)r;
  (void)g;
  (void)b;
  (void)cct;
  AWB_ERR("Error: Uninitialized interface been use");
  return FALSE;
}

void *awb_port_dummy_init(void *lib)
{
  (void)lib;
  AWB_ERR("Error: Uninitialized interface been use");
  return NULL;
}

void awb_port_dummy_deinit(void *awb_obj)
{
  (void)awb_obj;
  AWB_ERR("Error: Uninitialized interface been use");
}

/**
 * awb_port_load_dummy_default_func
 *
 * @awb_ops: structure with function pointers to be assign
 *
 * Return: TRUE on success
 **/
boolean awb_port_load_dummy_default_func(awb_ops_t *awb_ops)
{
  boolean rc = FALSE;
  if (awb_ops) {
    awb_ops->set_parameters = awb_port_dummy_set_params;
    awb_ops->get_parameters = awb_port_dummy_get_params;
    awb_ops->process = awb_port_dummy_process;
    awb_ops->estimate_cct = awb_port_dummy_estimate_cct_by_gains;
    awb_ops->estimate_gains = awb_port_dummy_estimate_gains_by_cct;
    awb_ops->estimate_ccm = awb_port_dummy_estimate_ccm_by_cct;
    awb_ops->init = awb_port_dummy_init;
    awb_ops->deinit = awb_port_dummy_deinit;
    rc = TRUE;
  }
  return rc;
}

/** awb_port_load_function
 *
 *    @aec_object: structure with function pointers to be assign
 *
 * Return: Handler to AWB interface library
 **/
void * awb_port_load_function(awb_ops_t *awb_ops)
{
  if (!awb_ops) {
    return NULL;
  }

  return awb_biz_load_function(awb_ops);
}

/** awb_port_unload_function
 *
 *    @private: Port private structure
 *
 *  Free resources allocated by awb_port_load_function
 *
 * Return: void
 **/
void awb_port_unload_function(awb_port_private_t *private)
{
  if (!private) {
    return;
  }

  awb_biz_unload_function(&private->awb_object.awb_ops, private->awb_iface_lib);
  awb_port_load_dummy_default_func(&private->awb_object.awb_ops);
  if (private->awb_iface_lib) {
    private->awb_iface_lib = NULL;
  }
  return;
}

/** awb_port_set_session_data:
 *    @port: awb's sink port to be initialized
 *    @q3a_lib_info: Q3A session data information
 *    @cam_position: Camera position
 *    @sessionid: session identity
 *
 *  Provide session data information for algo library set-up.
 **/
boolean awb_port_set_session_data(mct_port_t *port, void *q3a_lib_info,
  cam_position_t cam_position, unsigned int *sessionid)
{
  awb_port_private_t *private = NULL;
  boolean rc = FALSE;
  unsigned int session_id = (((*sessionid) >> 16) & 0x00ff);
  mct_pipeline_session_data_q3a_t *q3a_session_data = NULL;

  if (!port || !port->port_private || strcmp(MCT_OBJECT_NAME(port), "awb_sink")) {
    return rc;
  }

  q3a_session_data = (mct_pipeline_session_data_q3a_t *)q3a_lib_info;

  AWB_HIGH("awb_libptr %p session_id %d", q3a_session_data->awb_libptr, session_id);

  private = port->port_private;
  /* Query to verify if extension use is required and if using default algo */
  private->awb_extension_use =
    awb_port_ext_is_extension_required(&q3a_session_data->awb_libptr,
      cam_position, &private->use_default_algo);
  if (FALSE == private->awb_extension_use) {
    AWB_HIGH("Load AWB interface functions");
    private->awb_iface_lib = awb_port_load_function(&private->awb_object.awb_ops);
  } else { /* Use extension */
    AWB_HIGH("Load AWB EXTENSION interface functions");
    private->awb_iface_lib = awb_port_ext_load_function(&private->awb_object.awb_ops,
      q3a_session_data->awb_libptr, cam_position, private->use_default_algo);
  }
  /* Verify that all basic fields were populater by OEM */
  if (!(private->awb_iface_lib && private->awb_object.awb_ops.init &&
    private->awb_object.awb_ops.deinit &&
    private->awb_object.awb_ops.set_parameters &&
    private->awb_object.awb_ops.get_parameters &&
    private->awb_object.awb_ops.process)) {
    AWB_ERR("Error: loading functions");
    /* Resetting default interface to clear things */
    if (FALSE == private->awb_extension_use) {
      awb_port_unload_function(private);
    } else {
      awb_port_ext_unload_function(private);
    }
    return FALSE;
  }

  private->awb_object.awb =
    private->awb_object.awb_ops.init(private->awb_iface_lib);
  rc = private->awb_object.awb ? TRUE : FALSE;
  if (FALSE == rc) {
    AWB_ERR("Error: fail to init AWB algo");
    return rc;
  }

  if (private->awb_extension_use) {
    rc = awb_port_ext_update_func_table(private);
    if (rc && private->func_tbl.ext_init) {
      stats_ext_return_type ret = STATS_EXT_HANDLING_FAILURE;
      ret = private->func_tbl.ext_init(port, session_id);
      if (ret != STATS_EXT_HANDLING_FAILURE) {
        rc = TRUE;
      }
    }
  }

  AWB_HIGH("awb: %p", private->awb_object.awb);
  return rc;
}

/** awb_port_print_log
 *
 **/
static inline void awb_port_print_log(awb_update_t *awb_update, char* event_name,
  uint32 sof_id)
{
  if (awb_update) {
    AWB_HIGH("%s: SOFID=%d,R=%f,G=%f,B=%f,CCT=%d,LOW"
      " L1=%d,L2=%d,HIGH L1=%d,L2=%d,CCM En=%d,Ovr=%d,c00=%f,c01=%f,"
      "c02=%f,c10=%f,c11=%f,c12=%f,c20=%f,c21=%f,c22=%f",
      event_name, sof_id, awb_update->gain.r_gain, awb_update->gain.g_gain,
      awb_update->gain.b_gain, awb_update->color_temp,
      awb_update->dual_led_setting.led1_low_setting,
      awb_update->dual_led_setting.led2_low_setting,
      awb_update->dual_led_setting.led1_high_setting,
      awb_update->dual_led_setting.led2_high_setting,
      awb_update->ccm_update.awb_ccm_enable,
      awb_update->ccm_update.ccm_update_flag,
      awb_update->ccm_update.ccm[0][0], awb_update->ccm_update.ccm[0][1],
      awb_update->ccm_update.ccm[0][2], awb_update->ccm_update.ccm[1][0],
      awb_update->ccm_update.ccm[1][1], awb_update->ccm_update.ccm[1][2],
      awb_update->ccm_update.ccm[2][0], awb_update->ccm_update.ccm[2][1],
      awb_update->ccm_update.ccm[2][2]);
  }
}

/** awb_port_send_event
 *
 **/
static void awb_port_send_event(mct_port_t *port, int evt_type,
  int sub_evt_type, void *data, uint32_t sof_id)
{
  awb_port_private_t *private = (awb_port_private_t *)(port->port_private);
  mct_event_t        event;

    /* Pack into an mct_event object */
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = private->reserved_id;
  event.type = evt_type;
  event.u.module_event.current_frame_id = sof_id;
  event.u.module_event.type = sub_evt_type;
  event.u.module_event.module_event_data = data;

  MCT_PORT_EVENT_FUNC(port)(port, &event);
  return;
}

/** awb_port_send_awb_info_to_metadata
 *  update awb info which required by eztuing
 **/

static void awb_port_send_awb_info_to_metadata(
  mct_port_t  *port,
  awb_output_data_t *output)
{
  mct_event_t               event;
  mct_bus_msg_t             bus_msg;
  awb_output_eztune_data_t  awb_info;
  awb_port_private_t        *private;
  int                       size;

  if (!output || !port) {
    AWB_ERR("input error");
    return;
  }

  /* If eztune is not running, no need to send eztune metadata */
  if (FALSE == output->eztune_data.ez_running) {
    return;
  }

  private = (awb_port_private_t *)(port->port_private);
  bus_msg.sessionid = (private->reserved_id >> 16);
  bus_msg.type = MCT_BUS_MSG_AWB_EZTUNING_INFO;
  bus_msg.msg = (void *)&awb_info;
  size = (int)sizeof(awb_output_eztune_data_t);
  bus_msg.size = size;

  memcpy(&awb_info, &output->eztune_data,
    sizeof(awb_output_eztune_data_t));
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = private->reserved_id;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_STATS_POST_TO_BUS;
  event.u.module_event.module_event_data = (void *)(&bus_msg);
  MCT_PORT_EVENT_FUNC(port)(port, &event);
}

/** awb_send_bus_msg
 *
 **/
void awb_send_bus_msg(
  mct_port_t *port,
  mct_bus_msg_type_t bus_msg_type,
  void *payload,
  int size,
  int sof_id,
  q3a_stats_stream_type isp_stream_type)
{
  awb_port_private_t *awb_port = (awb_port_private_t *)(port->port_private);
  mct_event_t        event;
  mct_bus_msg_t      bus_msg;
  memset(&bus_msg, 0, sizeof(mct_bus_msg_t));

  bus_msg.sessionid = (awb_port->reserved_id >> 16);
  bus_msg.type = bus_msg_type;
  bus_msg.msg = payload;
  bus_msg.size = size;
  bus_msg.metadata_collection_type = isp_stream_type;

  /* pack into an mct_event object*/
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = awb_port->reserved_id;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_STATS_POST_TO_BUS;
  event.u.module_event.module_event_data = (void *)(&bus_msg);
  event.u.module_event.current_frame_id = sof_id;

  MCT_PORT_EVENT_FUNC(port)(port, &event);

  return;
}
/** awb_send_batch_bus_msg
 *
 *
 **/
boolean awb_send_batch_bus_msg(mct_port_t *port, uint32_t urgent_sof_id,
  uint32_t regular_sof_id, awb_update_t *awb_update, boolean offline)
{
  awb_port_private_t * private = (awb_port_private_t *)(port->port_private);
  mct_bus_msg_awb_t awb_msg;
  mct_bus_msg_awb_immediate_t urgent_msg;
  cam_awb_params_t *awb_info =  &urgent_msg.awb_info;
  cam_awb_ccm_update_t *ccm_out = &awb_info->ccm_update;
  mct_bus_metadata_collection_type_t meta_type = MCT_BUS_ONLINE_METADATA;

  memset(&urgent_msg, 0, sizeof(mct_bus_msg_awb_immediate_t));

  /* Update regular awb metadata */
  awb_msg.awb_roi = private->awb_roi;
  awb_msg.awb_lock = private->awb_locked;

  /* Update immediate awb metadata */
  urgent_msg.awb_mode = private->current_wb;
  urgent_msg.awb_state = private->awb_state;

  if (awb_update) {
    awb_info->cct_value = awb_update->color_temp;
    awb_info->rgb_gains.r_gain = awb_update->gain.r_gain;
    awb_info->rgb_gains.g_gain = awb_update->gain.g_gain;
    awb_info->rgb_gains.b_gain = awb_update->gain.b_gain;
    urgent_msg.awb_decision = awb_update->decision;

    if (awb_update->ccm_update.awb_ccm_enable) {
      awb_ccm_update_t *ccm_in = &awb_update->ccm_update;
      const int32 ccm_in_size = sizeof(ccm_in->ccm);
      const int32 ccm_in_offset_size = sizeof(ccm_in->ccm_offset);

      if ((ccm_in_size == sizeof(ccm_out->ccm)) &&
        (ccm_in_offset_size == sizeof(ccm_out->ccm_offset))) {
        memcpy(&ccm_out->ccm, ccm_in->ccm, ccm_in_size);
        memcpy(&ccm_out->ccm_offset, ccm_in->ccm_offset, ccm_in_offset_size);

        /* Hard ccm flag is used to set the unity ccm matrix in ISP.
         * Currently set the value as false, since awb is not having any such use case */
        ccm_out->hard_awb_ccm_flag = FALSE;
        ccm_out->awb_ccm_enable = TRUE;
        ccm_out->ccm_update_flag = ccm_in->ccm_update_flag;
      } else {
        AWB_ERR("CCM src dst size mismatch");
      }
    }
  }

  if (TRUE == offline) {
    meta_type   = MCT_BUS_OFFLINE_METADATA;
  }

  /* Print metadata */
  AWB_LOW("Meta:Off=%d,SOF=%d,Lock=%d,CCT=%d,Gains:R=%f,G=%f,B=%f,Dec=%d,"
  "CCM:En=%d,Ovr=%d,c00=%f,c01=%f,c02=%f,c10=%f,c11=%f,c12=%f,c20=%f,c21=%f,c22=%f",
    offline, regular_sof_id, awb_msg.awb_lock, awb_info->cct_value,
    awb_info->rgb_gains.r_gain, awb_info->rgb_gains.g_gain,
    awb_info->rgb_gains.b_gain, urgent_msg.awb_decision,
    ccm_out->awb_ccm_enable, ccm_out->ccm_update_flag,
    ccm_out->ccm[0][0], ccm_out->ccm[0][1], ccm_out->ccm[0][2],
    ccm_out->ccm[1][0], ccm_out->ccm[1][1], ccm_out->ccm[1][2],
    ccm_out->ccm[2][0], ccm_out->ccm[2][1], ccm_out->ccm[2][2]);

  /* Send metadata */
  awb_send_bus_msg(port, MCT_BUS_MSG_AWB_IMMEDIATE, (void *)&urgent_msg,
    sizeof(mct_bus_msg_awb_immediate_t), urgent_sof_id, meta_type);
  awb_send_bus_msg(port, MCT_BUS_MSG_AWB, (void *)&awb_msg,
    sizeof(mct_bus_msg_awb_t), regular_sof_id ,meta_type);

  return TRUE;
}

/** awb_port_set_awb_mode:
 *  @awb_meta_mode: OFF/AUTO/SCENE_MODE- Main 3a switch
 *  @awb_on_off_mode: AWB OFF/ON switch
 *
 *
 **/
static void awb_port_set_awb_mode(awb_port_private_t * private) {
  uint8_t awb_meta_mode = private->awb_meta_mode;
  uint8_t awb_on_off_mode = private->awb_on_off_mode;
  switch(awb_meta_mode){
    case CAM_CONTROL_OFF:
      private->awb_auto_mode = FALSE;
      break;
    case CAM_CONTROL_AUTO:
      if(awb_on_off_mode)
        private->awb_auto_mode = TRUE;
      else
        private->awb_auto_mode = FALSE;
      break;
    case CAM_CONTROL_USE_SCENE_MODE:
      private->awb_auto_mode = TRUE;
      break;
    default:{
      private->awb_auto_mode = TRUE;
    }
  }
}

/** awb_port_set_bestshot_mode:
 *    @awb_mode:     scene mode to be set
 *    @mode: scene mode coming from HAL
 *
 * Set the bestshot mode for algo
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean awb_port_set_bestshot_mode(
  awb_bestshot_mode_type_t *awb_mode, cam_scene_mode_type mode)
{
  boolean rc = TRUE;
  *awb_mode = AWB_BESTSHOT_OFF;
  AWB_LOW("Set scene mode: %d", mode);

  /* We need to translate Android scene mode to the one
   * AWB algorithm understands.
   **/
  switch (mode) {
  case CAM_SCENE_MODE_OFF: {
    *awb_mode = AWB_BESTSHOT_OFF;
  }
    break;

  case CAM_SCENE_MODE_AUTO: {
    *awb_mode = AWB_BESTSHOT_AUTO;
  }
    break;

  case CAM_SCENE_MODE_LANDSCAPE: {
    *awb_mode = AWB_BESTSHOT_LANDSCAPE;
  }
    break;

  case CAM_SCENE_MODE_SNOW: {
    *awb_mode = AWB_BESTSHOT_SNOW;
  }
    break;

  case CAM_SCENE_MODE_BEACH: {
    *awb_mode = AWB_BESTSHOT_BEACH;
  }
    break;

  case CAM_SCENE_MODE_SUNSET: {
    *awb_mode = AWB_BESTSHOT_SUNSET;
  }
    break;

  case CAM_SCENE_MODE_NIGHT: {
    *awb_mode = AWB_BESTSHOT_NIGHT;
  }
    break;

  case CAM_SCENE_MODE_PORTRAIT: {
    *awb_mode = AWB_BESTSHOT_PORTRAIT;
  }
    break;

  case CAM_SCENE_MODE_BACKLIGHT: {
    *awb_mode = AWB_BESTSHOT_BACKLIGHT;
  }
    break;

  case CAM_SCENE_MODE_SPORTS: {
    *awb_mode = AWB_BESTSHOT_SPORTS;
  }
    break;

  case CAM_SCENE_MODE_ANTISHAKE: {
    *awb_mode = AWB_BESTSHOT_ANTISHAKE;
  }
    break;

  case CAM_SCENE_MODE_FLOWERS: {
    *awb_mode = AWB_BESTSHOT_FLOWERS;
  }
    break;

  case CAM_SCENE_MODE_CANDLELIGHT: {
    *awb_mode = AWB_BESTSHOT_CANDLELIGHT;
  }
    break;

  case CAM_SCENE_MODE_FIREWORKS: {
    *awb_mode = AWB_BESTSHOT_FIREWORKS;
  }
    break;

  case CAM_SCENE_MODE_PARTY: {
    *awb_mode = AWB_BESTSHOT_PARTY;
  }
    break;

  case CAM_SCENE_MODE_NIGHT_PORTRAIT: {
    *awb_mode = AWB_BESTSHOT_NIGHT_PORTRAIT;
  }
    break;

  case CAM_SCENE_MODE_THEATRE: {
    *awb_mode = AWB_BESTSHOT_THEATRE;
  }
    break;

  case CAM_SCENE_MODE_ACTION: {
    *awb_mode = AWB_BESTSHOT_ACTION;
  }
    break;

  case CAM_SCENE_MODE_AR: {
    *awb_mode = AWB_BESTSHOT_AR;
  }
    break;
  case CAM_SCENE_MODE_FACE_PRIORITY: {
    *awb_mode = AWB_BESTSHOT_FACE_PRIORITY;
  }
    break;
  case CAM_SCENE_MODE_BARCODE: {
    *awb_mode = AWB_BESTSHOT_BARCODE;
  }
    break;
  case CAM_SCENE_MODE_HDR: {
    *awb_mode = AWB_BESTSHOT_HDR;
  }
    break;
  default: {
    rc = FALSE;
  }
    break;
  }

  return rc;
} /* awb_port_set_bestshot_mode */

/** awb_port_is_awb_locked:
 *  @awb_port_private_t: awb private data
 *
 **/
static boolean awb_port_is_awb_locked(
  awb_port_private_t *private)
{
  if(private->awb_locked) {
    return TRUE;
  }

  return FALSE;
}

static boolean awb_port_is_converged(awb_port_private_t *private,
  awb_output_data_t *output)
{
  boolean is_converged = FALSE;
  awb_port_converge_t *awb_converge = &private->awb_converge;

  /* Verify that we have new stats data */
  if (awb_converge->prev_stats_id != private->cur_stats_id) {
    awb_converge->prev_stats_id = private->cur_stats_id;

    if(awb_converge->previous_color_temp != output->color_temp) {
      /* Reset converge */
      awb_converge->frame_counter = 0;
      awb_converge->previous_color_temp = output->color_temp;
      is_converged = FALSE;
    } else if (awb_converge->frame_counter != awb_converge->frames_req) {
      /* Towards convergance */
      awb_converge->frame_counter++;
    }
  }

  if (awb_converge->frame_counter == awb_converge->frames_req) {
    is_converged = TRUE;
  }

  return is_converged;
}

static void awb_port_update_wb(awb_port_private_t *private,
  awb_output_data_t *output)
{
  /* Convert back to HAL3 wb mode*/
  int32_t wb_mode;
  switch(output->wb_mode){
  case CAMERA_WB_AUTO:
    wb_mode = CAM_WB_MODE_AUTO;
    break;
  case CAMERA_WB_CUSTOM:
    wb_mode = CAM_WB_MODE_CUSTOM;
    break;
  case CAMERA_WB_INCANDESCENT:
    wb_mode = CAM_WB_MODE_INCANDESCENT;
    break;
  case CAMERA_WB_FLUORESCENT:
    wb_mode = CAM_WB_MODE_FLUORESCENT;
    break;
  case CAMERA_WB_WARM_FLUORESCENT:
    wb_mode = CAM_WB_MODE_WARM_FLUORESCENT;
    break;
  case CAMERA_WB_DAYLIGHT:
    wb_mode =  CAM_WB_MODE_DAYLIGHT;
    break;
  case CAMERA_WB_CLOUDY_DAYLIGHT:
    wb_mode = CAM_WB_MODE_CLOUDY_DAYLIGHT;
    break;
  case CAMERA_WB_TWILIGHT:
    wb_mode = CAM_WB_MODE_TWILIGHT;
    break;
  case CAMERA_WB_SHADE:
    wb_mode = CAM_WB_MODE_SHADE;
    break;
  case CAMERA_WB_OFF:
    wb_mode = CAM_WB_MODE_OFF;
    break;
  case CAMERA_WB_MANUAL:
    wb_mode = CAM_WB_MODE_MANUAL;
    break;
  default:
    wb_mode = CAM_WB_MODE_OFF;
    break;
  }
  private->current_wb = wb_mode;
}

static void awb_port_update_state(awb_port_private_t *private,
  awb_output_data_t *output)
{
  int last_state = private->awb_state;

  /* state transition logic */
  switch(private->awb_last_state) {
  case CAM_AWB_STATE_INACTIVE: {
    if(awb_port_is_awb_locked(private)) {
      private->awb_state = CAM_AWB_STATE_LOCKED;
    } else if (!awb_port_is_converged(private, output)) {
      private->awb_state = CAM_AWB_STATE_SEARCHING;
    } else {
      //no change
    }
  }
    break;

  case CAM_AWB_STATE_SEARCHING: {
    if(awb_port_is_awb_locked(private)) {
      private->awb_state = CAM_AWB_STATE_LOCKED;
    } else if (awb_port_is_converged(private, output)) {
      private->awb_state = CAM_AWB_STATE_CONVERGED;
    } else {
      //no change
    }
  }
    break;

  case CAM_AWB_STATE_CONVERGED: {
    if(awb_port_is_awb_locked(private)) {
      private->awb_state = CAM_AWB_STATE_LOCKED;
    } else if(!awb_port_is_converged(private, output)) {
      private->awb_state = CAM_AWB_STATE_SEARCHING;
    } else {
      //no change
    }
  }
    break;

  case CAM_AWB_STATE_LOCKED: {
    if(!awb_port_is_awb_locked(private)) {
      if(awb_port_is_converged(private, output)) {
        private->awb_state = CAM_AWB_STATE_CONVERGED;
      } else {
        private->awb_state = CAM_AWB_STATE_SEARCHING;
      }
    }
  }
    break;

  default: {
    AWB_ERR("Error, AWB last state is unknown: %d",
      private->awb_last_state);
  }
    break;
  }

  private->awb_last_state = last_state;
}

/** awb_port_pack_output
 *
 **/
static void awb_port_pack_output(awb_output_data_t *output,
  awb_gain_t *rgb_gain, awb_port_private_t *private)
{
  output->stats_update.flag = STATS_UPDATE_AWB;

  /* memset the output struct */
  memset(&output->stats_update.awb_update, 0,
    sizeof(awb_update_t));

  output->stats_update.awb_update.stats_frm_id = output->frame_id;

  /*RGB gain*/
  rgb_gain->r_gain = output->r_gain;
  rgb_gain->g_gain = output->g_gain;
  rgb_gain->b_gain = output->b_gain;

  /* color_temp */
  output->stats_update.awb_update.gain = *rgb_gain;
  output->stats_update.awb_update.unadjusted_gain.r_gain = output->unadjusted_r_gain;
  output->stats_update.awb_update.unadjusted_gain.g_gain = output->unadjusted_g_gain;
  output->stats_update.awb_update.unadjusted_gain.b_gain = output->unadjusted_b_gain;
  output->stats_update.awb_update.color_temp = output->color_temp;
  output->stats_update.awb_update.wb_mode = output->wb_mode;
  output->stats_update.awb_update.best_mode = output->best_mode;
  output->stats_update.awb_update.decision = output->decision;

  /* Copy ccm output to awb_update */
  output->stats_update.awb_update.ccm_update.awb_ccm_enable =
    output->awb_ccm_enable;
  private->awb_ccm_enable = output->awb_ccm_enable;
  output->stats_update.awb_update.ccm_update.ccm_update_flag =
    output->ccm.override_ccm;
  memcpy(&output->stats_update.awb_update.ccm_update.ccm, &output->ccm.ccm,
    sizeof(output->ccm.ccm));
  memcpy(&output->stats_update.awb_update.ccm_update.ccm_offset,
    &output->ccm.ccm_offset, sizeof(output->ccm.ccm_offset));

  /* TBD: grey_world_stats is always true for bayer. For YUV change later */
  output->stats_update.awb_update.grey_world_stats = TRUE;
  /* Dual led driving currents and corresponding AEC flux */
  output->stats_update.awb_update.dual_led_setting = output->dual_led_settings;
  output->stats_update.awb_update.dual_led_flux_gain = output->dual_led_flux_gain;
  memcpy(output->stats_update.awb_update.sample_decision,
    output->samp_decision, sizeof(int) * 64);
  /* Handle Post bus msgs*/
  private->awb_roi.rect.left   = output->awb_roi_info.roi[0].x;
  private->awb_roi.rect.top    = output->awb_roi_info.roi[0].y;
  private->awb_roi.rect.width  = output->awb_roi_info.roi[0].dx;
  private->awb_roi.rect.height = output->awb_roi_info.roi[0].dy;
  private->awb_roi.weight      = output->awb_roi_info.weight[0];
  private->op_mode             = output->op_mode;
  private->awb_locked = output->awb_lock;

  memcpy(&private->awb_output, output, sizeof(awb_output_data_t));
  awb_port_update_state(private, output);
  awb_port_update_wb(private, output);

  /* Save the awb update to stored param for next use */
  if (private->stored_params && private->stored_params->enable && !private->flash_on) {
    awb_stored_params_type *stored_params = private->stored_params;
    stored_params->gains.r_gain = output->stats_update.awb_update.gain.r_gain;
    stored_params->gains.g_gain = output->stats_update.awb_update.gain.g_gain;
    stored_params->gains.b_gain = output->stats_update.awb_update.gain.b_gain;
    stored_params->color_temp = output->stats_update.awb_update.color_temp;
  }
}

/** awb_port_send_exif_debug_data:
 * right now,just update cct value
 * Return nothing
 **/
static void awb_port_send_exif_debug_data(mct_port_t *port)
{
  mct_event_t           event;
  mct_bus_msg_t         bus_msg;
  cam_awb_exif_debug_t  *awb_info;
  awb_port_private_t    *private;
  int                    size;

  if (!port) {
    AWB_ERR("input error");
    return;
  }
  private = (awb_port_private_t *)(port->port_private);
  if (private == NULL) {
    return;
  }

  /* Send exif data if data size is valid */
  if (!private->awb_debug_data_size) {
    AWB_LOW("awb_port: Debug data not available");
    return;
  }
  awb_info = (cam_awb_exif_debug_t *) malloc(sizeof(cam_awb_exif_debug_t));
  if (!awb_info) {
    AWB_ERR("Failure allocating memory for debug data");
    return;
  }
  memset(&bus_msg, 0, sizeof(mct_bus_msg_t));
  bus_msg.sessionid = (private->reserved_id >> 16);
  bus_msg.type = MCT_BUS_MSG_AWB_EXIF_DEBUG_INFO;
  bus_msg.msg = (void *)awb_info;
  size = (int)sizeof(cam_awb_exif_debug_t);
  bus_msg.size = size;
  memset(awb_info, 0, size);
  awb_info->awb_debug_data_size = private->awb_debug_data_size;

  AWB_LOW("awb_debug_data_size: %d", private->awb_debug_data_size);
  memcpy(&(awb_info->awb_private_debug_data[0]), private->awb_debug_data_array,
    private->awb_debug_data_size);
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = private->reserved_id;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_STATS_POST_TO_BUS;
  event.u.module_event.module_event_data = (void *)(&bus_msg);
  MCT_PORT_EVENT_FUNC(port)(port, &event);
  if (awb_info) {
    free(awb_info);
  }
}

static void awb_port_stats_done_callback(void* p, void* stats)
{
  mct_port_t         *port = (mct_port_t *)p;
  awb_port_private_t *private = NULL;
  stats_t            *awb_stats = (stats_t *)stats;
  if (!port) {
    AWB_ERR("input error");
    return;
  }

  private = (awb_port_private_t *)(port->port_private);
  if (!private) {
    return;
  }

  AWB_HIGH("DONE AWB STATS ACK back");
  if (awb_stats) {
    circular_stats_data_done(awb_stats->ack_data,port,
                             private->reserved_id, private->cur_sof_id);
  }
}

static void awb_port_configure_stats(awb_output_data_t *output,
  mct_port_t *port)
{
  awb_port_private_t *private = NULL;
  mct_event_t        event;
  awb_config_t       awb_config;

  private = (awb_port_private_t *)(port->port_private);

  awb_config = output->config;
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = private->reserved_id;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_STATS_AWB_CONFIG_UPDATE;
  event.u.module_event.module_event_data = (void *)(&awb_config);

  AWB_HIGH("Send MCT_EVENT_MODULE_STATS_AWB_CONFIG_UPDATE: is_valid: %d",
    awb_config.bg_config.is_valid);

  MCT_PORT_EVENT_FUNC(port)(port, &event);
}


/** awb_port_handle_offline_metadata_req
 *  Handle offline metadata request.
 **/
static void awb_port_handle_offline_metadata_req(
  mct_port_t *port, awb_output_data_t *output)
{
  awb_port_private_t *private = (awb_port_private_t *)(port->port_private);
  AWB_LOW("Wait for offline metadata to be available!");
  sem_wait(&private->sem_offline_proc);
  AWB_LOW("Post Offline metadata!");
  awb_send_batch_bus_msg(port, STATS_REPORT_IMMEDIATE, STATS_REPORT_IMMEDIATE,
    &output->stats_update.awb_update, TRUE);
}

/** awb_port_callback_offline_proc
 *  Handle output of offline AWB stats processing.
 **/
static void awb_port_callback_offline_proc(
  mct_port_t *port, awb_output_data_t *output)
{
  if (port && output) {
    /* Copy selected data which be used for offline metadata */
    awb_port_private_t * private = (awb_port_private_t *)(port->port_private);
    awb_update_t *awb_offline = &private->offline_output.stats_update.awb_update;
    awb_offline->gain.r_gain = output->r_gain;
    awb_offline->gain.g_gain = output->g_gain;
    awb_offline->gain.b_gain = output->b_gain;
    awb_offline->color_temp = output->color_temp;
    awb_offline->decision = output->decision;

    if (output->awb_ccm_enable) {
      awb_offline->ccm_update.awb_ccm_enable = output->awb_ccm_enable;
      awb_offline->ccm_update.ccm_update_flag = output->ccm.override_ccm;
      memcpy(&awb_offline->ccm_update.ccm, &output->ccm.ccm, sizeof(output->ccm.ccm));
      memcpy(&awb_offline->ccm_update.ccm_offset, &output->ccm.ccm_offset,
        sizeof(output->ccm.ccm_offset));
    }

    /* Wait for offline metadata request from MCT before posting.
     * Signal the waiting thread we've updated offline metadata */
    AWB_LOW("Post Offline Semaphore!")
    sem_post(&private->sem_offline_proc);
  }
}

/** awb_port_callback
 *
 **/
static void awb_port_callback(awb_output_data_t *output, void *p)
{
  mct_port_t         *port = (mct_port_t *)p;
  awb_gain_t         rgb_gain;
  awb_port_private_t *private = NULL;

  if (!output || !port) {
    AWB_ERR("input error");
    return;
  }
  private = (awb_port_private_t *)(port->port_private);
  if (!private) {
    return;
  }

  /* First handle callback in extension if available */
  if (private->func_tbl.ext_callback) {
    stats_ext_return_type ret;
    ret = private->func_tbl.ext_callback(
      port, output, &output->stats_update.awb_update);
    if (STATS_EXT_HANDLING_COMPLETE == ret) {
      AWB_LOW("Callback handled. Skipping rest!");
      return;
    }
  }

  /* populate the stats_upate object to be sent out*/
  AWB_LOW("STATS_UPDATE_AWB");
  if (AWB_UPDATE_OFFLINE == output->type) {
    AWB_LOW("Offline AWB status update!");
    awb_port_callback_offline_proc(port, output);
  } else if (AWB_UPDATE == output->type) {
    MCT_OBJECT_LOCK(port);
    private->awb_update_flag = TRUE;
    MCT_OBJECT_UNLOCK(port);
  } else if ((AWB_SEND_OUTPUT_EVENT == output->type) &&
    (FALSE == private->stats_frame_capture.frame_capture_mode)) {
    awb_port_pack_output(output, &rgb_gain, private);
    awb_port_print_log(&output->stats_update.awb_update, "CB-AWB_UP",
      private->cur_sof_id);
    awb_port_send_event(port, MCT_EVENT_MODULE_EVENT,
      MCT_EVENT_MODULE_STATS_AWB_UPDATE,
      (void *)(&output->stats_update),output->sof_id);
    if (output->need_config && private->dual_cam_sensor_info == CAM_TYPE_MAIN) {
      awb_port_configure_stats(output, port);
    }
    output->need_config = 0;

    if (output->eztune_data.ez_running) {
        awb_port_send_awb_info_to_metadata(port, output);
    }
    /* Save the awb debug data in private data struct to be sent out later */
    private->awb_debug_data_size = output->awb_debug_data_size;
    if (output->awb_debug_data_size) {
      memcpy(private->awb_debug_data_array, output->awb_debug_data_array,
        output->awb_debug_data_size);
    }
  }
  return;
}

/** port_stats_check_identity
 *    @data1: port's existing identity;
 *    @data2: new identity to compare.
 *
 *  Return TRUE if two session index in the dentities are equalequal,
 *  Stats modules are linked ONLY under one session.
 **/
static boolean awb_port_check_identity(void *data1, void *data2)
{
  return (((*(unsigned int *)data1) ==
          (*(unsigned int *)data2)) ? TRUE : FALSE);
}

/** awb_port_check_session
 *
 **/
static boolean awb_port_check_session(void *data1, void *data2)
{
  return (((*(unsigned int *)data1) & 0xFFFF0000) ==
    ((*(unsigned int *)data2) & 0xFFFF0000) ? TRUE : FALSE);
}

/** awb_port_check_stream
 *
 **/
static boolean awb_port_check_stream(void *data1, void *data2)
{
  return ( ((*(unsigned int *)data1) & 0x0000FFFF ) ==
    ((*(unsigned int *)data2) & 0x0000FFFF) ? TRUE : FALSE);
}

/** awb_port_check_port_availability
 *
 *
 *
 **/
boolean awb_port_check_port_availability(mct_port_t *port,
  unsigned int *session)
{
  if (port->port_private == NULL) {
    return TRUE;
  }

  if (mct_list_find_custom(MCT_OBJECT_CHILDREN(port), session,
    awb_port_check_session) != NULL) {
    return TRUE;
  }

  return FALSE;
}

static boolean awb_port_event_sof( mct_port_t *port,
  mct_event_t *event)
{
  int                     rc =  TRUE;
  mct_bus_msg_isp_sof_t *sof_event;
  awb_port_private_t *port_private = (awb_port_private_t *)(port->port_private);
  sof_event =(mct_bus_msg_isp_sof_t *)(event->u.ctrl_event.control_event_data);
  uint32_t cur_sof_id = 0;
  uint32_t cur_stats_id = 0;
  awb_update_t *p_awb_update_for_meta =
    &port_private->awb_output.stats_update.awb_update;
  awb_output_data_t *output_p = &port_private->output_buffer;

  MCT_OBJECT_LOCK(port);
  cur_sof_id = port_private->cur_sof_id = sof_event->frame_id;
  cur_stats_id = port_private->cur_stats_id;
  MCT_OBJECT_UNLOCK(port);
  /* if manual mode is set then send immediately and dont enqueue sof msg*/
  if (!port_private->awb_auto_mode) {
    if (port_private->manual.valid) {
      awb_ccm_type ccm;
      awb_update_t *awb_update = &output_p->stats_update.awb_update;
      memset(&output_p->stats_update, 0, sizeof(stats_update_t));

      /* Get ccm from core */
      rc = port_private->awb_object.awb_ops.estimate_ccm(
        port_private->awb_object.awb, port_private->manual.u.cct, &ccm);

      /* Prepare manual output */
      output_p->stats_update.flag = STATS_UPDATE_AWB;
      awb_update->gain.r_gain = port_private->manual.u.manual_gain.r_gain;
      awb_update->gain.g_gain = port_private->manual.u.manual_gain.g_gain;
      awb_update->gain.b_gain = port_private->manual.u.manual_gain.b_gain;
      awb_update->color_temp  = port_private->manual.u.cct;
      awb_update->ccm_update.awb_ccm_enable = port_private->awb_ccm_enable;
      awb_update->ccm_update.ccm_update_flag = ccm.override_ccm;
      memcpy(&awb_update->ccm_update.ccm, &ccm.ccm, sizeof(ccm.ccm));
      memcpy(&awb_update->ccm_update.ccm_offset, &ccm.ccm_offset, sizeof(ccm.ccm_offset));

      /* Print and send the output */
      awb_port_print_log(awb_update, "SOF-MAN_UP", cur_sof_id);
      awb_port_send_event(port, MCT_EVENT_MODULE_EVENT,
        MCT_EVENT_MODULE_STATS_AWB_MANUAL_UPDATE,
        (void *)(&output_p->stats_update), sof_event->frame_id);

      q3a_thread_aecawb_msg_t *awb_msg =
        (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
      if (awb_msg == NULL) {
        return rc;
      }
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SET;
      awb_msg->u.awb_set_parm.type = AWB_SET_MANUAL_AUTO_SKIP;
      awb_msg->u.awb_set_parm.u.current_sof_id = sof_event->frame_id;
      rc = q3a_aecawb_thread_en_q_msg(port_private->thread_data, awb_msg);
      p_awb_update_for_meta = awb_update;
    }/* if(manual.valid)*/
    /* Update awb port states */
    port_private->awb_state         = CAM_AWB_STATE_INACTIVE;
    port_private->awb_last_state    = CAM_AWB_STATE_INACTIVE;
    port_private->awb_converge.frame_counter = 0; /* Reset convergence counter if setting INACTIVE */
    port_private->current_wb        = CAM_WB_MODE_OFF;
    awb_output_data_t output;
    memset(&output, 0, sizeof(awb_output_data_t));
    /* passing dummy output for Klocwork checking */
    awb_port_update_state(port_private, &output);
  } else if (TRUE == port_private->stats_frame_capture.frame_capture_mode) {
    uint8_t  current_batch_count =
      port_private->stats_frame_capture.current_batch_count;
    awb_update_t *awb_update = &output_p->stats_update.awb_update;
    awb_capture_frame_info_t *frame_batch =
      port_private->stats_frame_capture.frame_info.frame_batch;
    memcpy(output_p, &port_private->awb_output, sizeof(awb_output_data_t));
    output_p->stats_update.flag = STATS_UPDATE_AWB;
    awb_update->gain.r_gain = frame_batch[current_batch_count].r_gain;
    awb_update->gain.b_gain = frame_batch[current_batch_count].b_gain;
    awb_update->gain.g_gain = frame_batch[current_batch_count].g_gain;
    awb_update->color_temp = frame_batch[current_batch_count].color_temp;
    if (port_private->awb_ccm_enable) {
      awb_update->ccm_update.awb_ccm_enable = port_private->awb_ccm_enable;
      awb_update->ccm_update.ccm_update_flag =
        frame_batch[current_batch_count].ccm.override_ccm;
      memcpy(&awb_update->ccm_update.ccm, &frame_batch[current_batch_count].ccm.ccm,
        sizeof(awb_update->ccm_update.ccm));
      memcpy(&awb_update->ccm_update.ccm_offset, &frame_batch[current_batch_count].ccm.ccm_offset,
        sizeof(awb_update->ccm_update.ccm_offset));
      AWB_HIGH("Frame_capture_mode ccm %f %f %f %f %f %f %f %f %f",
        awb_update->ccm_update.ccm[0][0], awb_update->ccm_update.ccm[0][1],
        awb_update->ccm_update.ccm[0][2], awb_update->ccm_update.ccm[1][0],
        awb_update->ccm_update.ccm[1][1], awb_update->ccm_update.ccm[1][2],
        awb_update->ccm_update.ccm[2][0], awb_update->ccm_update.ccm[2][1],
        awb_update->ccm_update.ccm[2][2]);
    }

    if (Q3A_CAPTURE_RESET == frame_batch[current_batch_count].capture_type) {
      port_private->stats_frame_capture.frame_capture_mode = FALSE;
    }

    awb_port_print_log(&output_p->stats_update.awb_update, "SOF-FC_AWB_UP", cur_sof_id);
    awb_port_send_event(port, MCT_EVENT_MODULE_EVENT,
      MCT_EVENT_MODULE_STATS_AWB_UPDATE,
      (void *)(&output_p->stats_update), port_private->cur_sof_id);
    p_awb_update_for_meta = awb_update;
  } else { /* Auto mode */
    /* Reset manual valid flag*/
    memset (&port_private->manual, 0, sizeof(awb_port_m_gain_t));
    port_private->manual.valid = FALSE;

    if (port_private->awb_still_capture_sof == cur_sof_id ||
        port_private->capture_intent_skip > 0) {
      /* If there is capture intent for current SOF, just send it
       * out using saved snapshot gain.
       * */
      if (port_private->awb_still_capture_sof == cur_sof_id) {
        /* Initialized capture intent skip and reset current still capture sof */
        port_private->capture_intent_skip = STATS_FLASH_ON + STATS_FLASH_DELAY;
        port_private->awb_still_capture_sof = 0;
      }
      port_private->capture_intent_skip--;

      memcpy(output_p, &port_private->awb_output, sizeof(awb_output_data_t));
      output_p->r_gain = output_p->snap_r_gain;
      output_p->g_gain = output_p->snap_g_gain;
      output_p->b_gain = output_p->snap_b_gain;
      output_p->color_temp = output_p->snap_color_temp;
      output_p->ccm = output_p->snap_ccm;
      output_p->sof_id = sof_event->frame_id;
      output_p->type = AWB_SEND_OUTPUT_EVENT;
      AWB_LOW("Intent WB skip=%d,R=%f,G=%f,B=%f,cct=%d,ovrd=%d,c00=%f,c01=%f,"
      "c02=%f,c10=%f,c11=%f,c12=%f,c20=%f,c21=%f,c22=%f",port_private->capture_intent_skip,
        output_p->r_gain,output_p->g_gain,output_p->b_gain,
        output_p->color_temp,output_p->ccm.override_ccm,
        output_p->ccm.ccm[0][0],output_p->ccm.ccm[0][1],output_p->ccm.ccm[0][2],
        output_p->ccm.ccm[1][0],output_p->ccm.ccm[1][1],output_p->ccm.ccm[1][2],
        output_p->ccm.ccm[2][0],output_p->ccm.ccm[2][1],output_p->ccm.ccm[2][2]);
      awb_port_callback(output_p, port);
      p_awb_update_for_meta = &output_p->stats_update.awb_update;
    } else if(cur_stats_id &&
      cur_sof_id == cur_stats_id + 1) {
      /* STATS data could arrive by end of current SOF or early next SOF
         port need to make sure send pack_output for stats N in SOF N + 1
         1. If stats data arrive in current SOF, then send the pack output on next SOF
         2. If stats data arrive in next SOF, then send pack output right away
         below is for case 1*/
      q3a_thread_aecawb_msg_t *awb_msg =
        (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
      if (awb_msg == NULL) {
        return rc;
      }
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SEND_EVENT;
      awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_PACK_OUTPUT;
      awb_msg->u.awb_set_parm.u.current_sof_id = sof_event->frame_id;
      rc = q3a_aecawb_thread_en_q_msg(port_private->thread_data, awb_msg);
    }
  }
  /* Send exif debug data from SOF */
  awb_port_send_exif_debug_data(port);
  awb_send_batch_bus_msg(port, STATS_REPORT_IMMEDIATE,
    sof_event->frame_id, p_awb_update_for_meta, FALSE);
  return rc;
}

static boolean awb_port_stats_skip_check(awb_port_private_t *private,
  uint32_t frame_id)
{
  boolean            rc = FALSE;

  /* if it's in fast aec mode and aec is runing, skip sending the stats to awb core. */
  if(private->fast_aec_data.enable &&
    ((private->fast_aec_data.state == Q3A_FAST_AEC_STATE_AEC_RUNNING) ||
    (frame_id < private->fast_aec_forced_cnt))) {
    AWB_HIGH("bypass awb algo, frame id %d",
      frame_id);
    rc = TRUE;
  } else if (private->is_still_capture || private->capture_intent_skip) {
    AWB_HIGH("In still capture mode, skip stats");
    rc = TRUE;
  }

  return rc;
}

/** awb_port_is_handle_stats_required
 *    @private: private awb port data
 *    @stats_mask: type of stats provided
 *
 * Return: TRUE if stats required
 **/
static boolean awb_port_is_handle_stats_required(uint32_t stats_mask)
{
  if (!((stats_mask & (1 << MSM_ISP_STATS_AWB)) ||
    (stats_mask & (1 << MSM_ISP_STATS_BG)))) {
    return FALSE;
  }
  return TRUE;
}

static boolean awb_port_event_stats_data(mct_port_t *port,
  mct_event_t *event)
{
  boolean               rc = TRUE;
  awb_port_private_t    *port_private = (awb_port_private_t *)(port->port_private);
  mct_event_module_t    *mod_evt = &(event->u.module_event);
  mct_event_stats_isp_t *stats_event ;
  mct_event_stats_ext_t *stats_ext_event;
  stats_t               *awb_stats;
  boolean               send_flag = FALSE;
  uint32_t              awb_stats_mask = 0;

  stats_ext_event = (mct_event_stats_ext_t *)(mod_evt->module_event_data);
  stats_event = stats_ext_event->stats_data;

  /* skip stats in Manual mode */
  if(!port_private->awb_auto_mode)
    return rc;

  if (stats_event) {
    awb_stats_mask = stats_event->stats_mask & port_private->required_stats_mask;
    if (!awb_port_is_handle_stats_required(awb_stats_mask)) {
      return TRUE; /* Non error case */
    }

    q3a_thread_aecawb_msg_t *awb_msg =
      (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL) {
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));

      awb_stats = (stats_t *)calloc(1, sizeof(stats_t));

      if (awb_stats == NULL) {
        free(awb_msg);
        return rc;
      }

      awb_msg->u.stats = awb_stats;
      awb_stats->frame_id = stats_event->frame_id;
      /* Filter by the stats that algo has requested */
      awb_stats->isp_stream_type =
        (stats_event->isp_streaming_type == ISP_STREAMING_OFFLINE) ?
        Q3A_STATS_STREAM_OFFLINE : Q3A_STATS_STREAM_ONLINE;
      if ((awb_stats_mask & (1 << MSM_ISP_STATS_AWB)) &&
        stats_event->stats_data[MSM_ISP_STATS_AWB].stats_buf) {
        awb_msg->type = MSG_AWB_STATS;
        awb_stats->stats_type_mask |= STATS_AWB;
        AWB_LOW(" got awb stats");
        send_flag = TRUE;
        awb_stats->yuv_stats.p_q3a_awb_stats =
          stats_event->stats_data[MSM_ISP_STATS_AWB].stats_buf;
      } else if ((awb_stats_mask & (1 << MSM_ISP_STATS_BG)) &&
        stats_event->stats_data[MSM_ISP_STATS_BG].stats_buf) {
        AWB_LOW(" got bg stats");
        awb_stats->stats_type_mask |= STATS_BG;
        awb_msg->type = MSG_BG_AWB_STATS;
        send_flag = TRUE;
        awb_stats->bayer_stats.p_q3a_bg_stats = stats_event->stats_data[MSM_ISP_STATS_BG].stats_buf;
      }

      if (awb_port_stats_skip_check(port_private, stats_event->frame_id)) {
        AWB_HIGH("skip stats required");
        send_flag = FALSE;
      }

      if (send_flag) {
        uint32_t cur_stats_id = 0;
        uint32_t cur_sof_id  = 0;

        MCT_OBJECT_LOCK(port);
        cur_stats_id = port_private->cur_stats_id = stats_event->frame_id;
        cur_sof_id = port_private->cur_sof_id;
        MCT_OBJECT_UNLOCK(port);

        AWB_LOW("msg type=%d, stats=%p, mask=0x%x mask addr=%p",
          awb_msg->type, awb_msg->u.stats,
          awb_msg->u.stats->stats_type_mask,
          &(awb_msg->u.stats->stats_type_mask));

        if (awb_msg->type == MSG_BG_AWB_STATS) {
          awb_stats->ack_data = stats_ext_event;
          circular_stats_data_use(stats_ext_event);
        }

        rc = q3a_aecawb_thread_en_q_msg(port_private->thread_data, awb_msg);

        if (rc == FALSE) {
          if (awb_msg->type == MSG_BG_AWB_STATS) {
            circular_stats_data_done(stats_ext_event, 0, 0, 0);
          }
          /* In enqueue fail, memory is free inside q3a_aecawb_thread_en_q_msg() *
           * Return back from here */
          awb_stats = NULL;
          return rc;
        }
        /* STATS data could arrive by end of current SOF or early next SOF
           port need to make sure send pack_output for stats N in SOF N + 1
           1. If stats data arrive in current SOF, then send the pack output on next SOF
           2. If stats data arrive in next SOF, then send pack output right away
           3. Doesn't need queue the pack_output for still_capture, already be sent in SOF
           4. In Fast AEC mode, no SOF command send from MCT, send pack output right away.
           below is for case 2 and 4.*/
        if (stats_event->isp_streaming_type != ISP_STREAMING_OFFLINE) {
          if((cur_stats_id + 1 == cur_sof_id &&
            port_private->awb_still_capture_sof != cur_sof_id) ||
            port_private->fast_aec_data.enable){
            q3a_thread_aecawb_msg_t *awb_msg =
             (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
            if (awb_msg == NULL) {
              return rc;
            }
            memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
            awb_msg->type = MSG_AWB_SEND_EVENT;
            awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_PACK_OUTPUT;
            awb_msg->u.awb_set_parm.u.current_sof_id = stats_event->frame_id;
            rc = q3a_aecawb_thread_en_q_msg(port_private->thread_data, awb_msg);
          }
        }
      } else {
        free(awb_stats);
        free(awb_msg);
      }
    }
  }
  return rc;
}
/** awb_port_proc_get_aec_data
 *
 *
 *
 **/
static boolean awb_port_proc_get_awb_data(mct_port_t *port,
  stats_get_data_t *stats_get_data)
{
  boolean                 rc = FALSE;
  awb_port_private_t      *private = (awb_port_private_t *)(port->port_private);

   /* If in manual mode return manual gains stored in port instead of querying */
   if (!private->awb_auto_mode&&private->manual.valid) {
     stats_get_data->flag = STATS_UPDATE_AWB;
     stats_get_data->awb_get.g_gain = private->manual.u.manual_gain.g_gain;
     stats_get_data->awb_get.r_gain = private->manual.u.manual_gain.r_gain;
     stats_get_data->awb_get.b_gain = private->manual.u.manual_gain.b_gain;

   } else {

     q3a_thread_aecawb_msg_t *awb_msg =
      (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

     if (awb_msg) {
       awb_msg->sync_flag = TRUE;
       memset(awb_msg, 0 , sizeof(q3a_thread_aecawb_msg_t));
       awb_msg->sync_flag = TRUE;
       awb_msg->type = MSG_AWB_GET;
       awb_msg->u.awb_get_parm.type = AWB_PARMS;

       rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);

       stats_get_data->flag = STATS_UPDATE_AWB;
       stats_get_data->awb_get.g_gain =
         awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.g_gain;
       stats_get_data->awb_get.r_gain =
         awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.r_gain;
       stats_get_data->awb_get.b_gain =
         awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.b_gain;

       free(awb_msg);
     } else {
       AWB_ERR("Not enough memory");
     }
  }

  AWB_MSG_HIGH("Get data, R=%f, G=%f, B=%f",
    stats_get_data->awb_get.r_gain, stats_get_data->awb_get.g_gain,
    stats_get_data->awb_get.b_gain);
  return rc;
}

/** awb_port_unifed_request_batch_data:
 *    @private:   Private data of the port
 *
 * This function request to the algoritm the data to fill the batch information.
 *
 * Return: TRUE on success
 **/
static boolean awb_port_unifed_request_batch_data_to_algo(
  awb_port_private_t *private)
{
  boolean rc = FALSE;
  int i = 0, j = 0;
  awb_frame_batch_t *priv_frame_info = &private->stats_frame_capture.frame_info;

  q3a_thread_aecawb_msg_t *awb_msg =
    (q3a_thread_aecawb_msg_t *)calloc(1, sizeof(q3a_thread_aecawb_msg_t));
  if (NULL == awb_msg) {
    AWB_ERR("Not enough memory");
    return rc;
  }
  awb_msg->sync_flag = TRUE;
  awb_msg->type = MSG_AWB_GET;
  awb_msg->u.awb_get_parm.type = AWB_UNIFIED_FLASH;
  memcpy(&awb_msg->u.awb_get_parm.u.frame_info,
    priv_frame_info, sizeof(awb_frame_batch_t));
  rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
  if (TRUE == rc) {
    for (i = 0; i < priv_frame_info->num_batch; i++) {
      priv_frame_info->frame_batch[i].capture_type =
        awb_msg->u.awb_get_parm.u.frame_info.frame_batch[i].capture_type;
      priv_frame_info->frame_batch[i].r_gain =
        awb_msg->u.awb_get_parm.u.frame_info.frame_batch[i].r_gain;
      priv_frame_info->frame_batch[i].b_gain =
        awb_msg->u.awb_get_parm.u.frame_info.frame_batch[i].b_gain;
      priv_frame_info->frame_batch[i].g_gain =
        awb_msg->u.awb_get_parm.u.frame_info.frame_batch[i].g_gain;
      priv_frame_info->frame_batch[i].color_temp =
        awb_msg->u.awb_get_parm.u.frame_info.frame_batch[i].color_temp;
      priv_frame_info->frame_batch[i].ccm =
        awb_msg->u.awb_get_parm.u.frame_info.frame_batch[i].ccm;
      AWB_HIGH("Batch=%d capture_type=%d R_gain=%f B_gain=%f, G_gain=%f",
        i,
        priv_frame_info->frame_batch[i].capture_type,
        priv_frame_info->frame_batch[i].r_gain,
        priv_frame_info->frame_batch[i].b_gain,
        priv_frame_info->frame_batch[i].g_gain);
      if (priv_frame_info->frame_batch[i].ccm.override_ccm) {
        AWB_HIGH("Batch=%d ccm %f %f %f %f %f %f %f %f %f",
          i,
          priv_frame_info->frame_batch[i].ccm.ccm[0][0],
          priv_frame_info->frame_batch[i].ccm.ccm[0][1],
          priv_frame_info->frame_batch[i].ccm.ccm[0][2],
          priv_frame_info->frame_batch[i].ccm.ccm[1][0],
          priv_frame_info->frame_batch[i].ccm.ccm[1][1],
          priv_frame_info->frame_batch[i].ccm.ccm[1][2],
          priv_frame_info->frame_batch[i].ccm.ccm[2][0],
          priv_frame_info->frame_batch[i].ccm.ccm[2][1],
          priv_frame_info->frame_batch[i].ccm.ccm[2][2]);
      }
    }
  }
  free(awb_msg);

  return rc;
}

/** awb_port_unified_flash_trigger
* @port:  mct port type containing aec port private data
*
* The first call to this function, will set-up unified capture sequence.
*
* Return: TRUE on success
**/
static boolean awb_port_unified_flash_trigger(mct_port_t *port)
{
  boolean                 rc = FALSE;
  awb_port_private_t      *private = (awb_port_private_t *)(port->port_private);

  if (0 == private->stats_frame_capture.frame_info.num_batch) {
    AWB_ERR("No. of num_batch is zero");
    return FALSE;
  }

  if (FALSE == private->stats_frame_capture.frame_capture_mode) {
    private->stats_frame_capture.frame_capture_mode = TRUE;
    private->stats_frame_capture.current_batch_count = 0;
  } else {
    rc = TRUE;
    private->stats_frame_capture.current_batch_count++;
    AWB_HIGH("Incremented Current Batch no. =%d",
      private->stats_frame_capture.current_batch_count);
    if (private->stats_frame_capture.current_batch_count >
        private->stats_frame_capture.frame_info.num_batch) {
      private->stats_frame_capture.current_batch_count =
        private->stats_frame_capture.frame_info.num_batch;
    }
  }
  return rc;
}

/** awb_process_downstream_mod_event
 *    @port:
 *    @event:
 **/
static boolean awb_process_downstream_mod_event(mct_port_t *port,
  mct_event_t *event)
{
  boolean                 rc = TRUE;
  q3a_thread_aecawb_msg_t *awb_msg = NULL;
  mct_event_module_t      *mod_evt = &(event->u.module_event);
  awb_port_private_t      *private = (awb_port_private_t *)(port->port_private);

  AWB_LOW("Proceess module event of type: %d", mod_evt->type);

  /* Check if extended handling to be performed */
  if (private->func_tbl.ext_handle_module_event) {
    stats_ext_return_type ret = STATS_EXT_HANDLING_PARTIAL;
    ret = private->func_tbl.ext_handle_module_event(port, mod_evt);
    if (STATS_EXT_HANDLING_COMPLETE == ret) {
      AWB_LOW("Module event handled in extension function!");
      return TRUE;
    }
  }

  switch (mod_evt->type) {
  case MCT_EVENT_MODULE_STATS_GET_THREAD_OBJECT: {
    q3a_thread_aecawb_data_t *data =
      (q3a_thread_aecawb_data_t *)(mod_evt->module_event_data);

    private->thread_data = data->thread_data;

    data->awb_port = port;
    data->awb_cb   = awb_port_callback;
    data->awb_stats_cb = awb_port_stats_done_callback;
    data->awb_obj  = &(private->awb_object);
    rc = TRUE;
  } /* case MCT_EVENT_MODULE_STATS_GET_THREAD_OBJECT */
    break;

  case MCT_EVENT_MODULE_STATS_EXT_DATA: {
    rc = awb_port_event_stats_data(port, event);
  } /* case MCT_EVENT_MODULE_STATS_DATA */
    break;

  case MCT_EVENT_MODULE_SET_RELOAD_CHROMATIX:
  case MCT_EVENT_MODULE_SET_CHROMATIX_PTR: {
    modulesChromatix_t *mod_chrom =
      (modulesChromatix_t *)mod_evt->module_event_data;
    awb_msg = (q3a_thread_aecawb_msg_t *)
      malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL ) {
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SET;
      awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_INIT_CHROMATIX_SENSOR;
      /*To Do: for now hard-code the stats type and op_mode for now.*/
      awb_msg->u.awb_set_parm.u.init_param.stats_type = AWB_STATS_BAYER;
      awb_msg->u.awb_set_parm.u.init_param.chromatix = mod_chrom->chromatix3APtr;
      awb_msg->u.awb_set_parm.u.init_param.stored_params = private->stored_params;
      AWB_LOW(":stream_type=%d op_mode=%d",
        private->stream_type, awb_msg->u.awb_set_parm.u.init_param.op_mode);
      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);

      chromatix_3a_parms_type *chromatix =  mod_chrom->chromatix3APtr;
      private->fast_aec_forced_cnt =
        chromatix->AWB_bayer_algo_data.awb_reserved_data.reserved_int[49];

      if (private->fast_aec_data.num_frames == 0) {
        private->fast_aec_forced_cnt = 0;
      } else if (private->fast_aec_forced_cnt == 0 ||
        private->fast_aec_forced_cnt >= private->fast_aec_data.num_frames) {
        private->fast_aec_forced_cnt = private->fast_aec_data.num_frames - 1;
      }
      else {
        private->fast_aec_forced_cnt = private->fast_aec_data.num_frames -
          private->fast_aec_forced_cnt;
      }
      AWB_HIGH("fastaec frames %d forced_cnt %d",
        private->fast_aec_data.num_frames, private->fast_aec_forced_cnt);
      AWB_LOW("Enqueing AWB message returned: %d", rc);
    } else {
      AWB_ERR("Failure allocating memory for AWB msg!");
    }
  }
    break;

  case MCT_EVENT_MODULE_STATS_AEC_UPDATE: {
    stats_update_t *stats_update = (stats_update_t *)mod_evt->module_event_data;

    awb_msg =
      (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL ) {
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SET;
      awb_msg->is_priority = TRUE;
      awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_AEC_PARM;
      awb_msg->u.awb_set_parm.u.aec_parms.average_luma =
        stats_update->aec_update.avg_luma;
      awb_msg->u.awb_set_parm.u.aec_parms.exp_index =
        stats_update->aec_update.exp_index_for_awb;
      awb_msg->u.awb_set_parm.u.aec_parms.lux_idx =
        stats_update->aec_update.lux_idx;
      awb_msg->u.awb_set_parm.u.aec_parms.aec_settled =
        stats_update->aec_update.settled;
      awb_msg->u.awb_set_parm.u.aec_parms.cur_luma =
        stats_update->aec_update.cur_luma;
      awb_msg->u.awb_set_parm.u.aec_parms.target_luma =
        stats_update->aec_update.target_luma;
      awb_msg->u.awb_set_parm.u.aec_parms.cur_line_cnt =
        stats_update->aec_update.linecount;
      awb_msg->u.awb_set_parm.u.aec_parms.cur_real_gain =
        stats_update->aec_update.real_gain;
      awb_msg->u.awb_set_parm.u.aec_parms.stored_digital_gain =
        stats_update->aec_update.stored_digital_gain;
      awb_msg->u.awb_set_parm.u.aec_parms.total_drc_gain =
        stats_update->aec_update.total_drc_gain;
      awb_msg->u.awb_set_parm.u.aec_parms.flash_sensitivity =
        stats_update->aec_update.flash_sensitivity;
      awb_msg->u.awb_set_parm.u.aec_parms.led_mode =
        stats_update->aec_update.led_mode;
      awb_msg->u.awb_set_parm.u.aec_parms.led_state =
        stats_update->aec_update.led_state;
      awb_msg->u.awb_set_parm.u.aec_parms.use_led_estimation  =
        stats_update->aec_update.use_led_estimation;
      awb_msg->u.awb_set_parm.u.aec_parms.est_state =
        stats_update->aec_update.est_state;
      awb_msg->u.awb_set_parm.u.aec_parms.exp_tbl_val =
        stats_update->aec_update.exp_tbl_val;

      awb_msg->u.awb_set_parm.u.aec_parms.Bv =
        stats_update->aec_update.Bv;
      awb_msg->u.awb_set_parm.u.aec_parms.Tv =
        stats_update->aec_update.Tv;
      awb_msg->u.awb_set_parm.u.aec_parms.Sv =
        stats_update->aec_update.Sv;
      awb_msg->u.awb_set_parm.u.aec_parms.Av =
        stats_update->aec_update.Av;
      awb_msg->u.awb_set_parm.u.aec_parms.is_hdr_drc_enabled =
        stats_update->aec_update.is_hdr_drc_enabled;
      private->flash_on = (stats_update->aec_update.est_state != AEC_EST_OFF ||
                            stats_update->aec_update.use_led_estimation);

    /* Handle custom parameters update (3a ext) */
    if (stats_update->aec_update.aec_custom_param_update.data &&
      stats_update->aec_update.aec_custom_param_update.size) {
      awb_msg->u.awb_set_parm.u.aec_parms.custom_param_awb.data =
        malloc(stats_update->aec_update.aec_custom_param_update.size);
      if (awb_msg->u.awb_set_parm.u.aec_parms.custom_param_awb.data) {
        awb_msg->u.awb_set_parm.u.aec_parms.custom_param_awb.size =
          stats_update->aec_update.aec_custom_param_update.size;
        memcpy(awb_msg->u.awb_set_parm.u.aec_parms.custom_param_awb.data,
          stats_update->aec_update.aec_custom_param_update.data,
          awb_msg->u.awb_set_parm.u.aec_parms.custom_param_awb.size);
      } else {
        AWB_ERR("Error: Fail to allocate memory for custom parameters");
        free(awb_msg);
        awb_msg = NULL;
        rc = FALSE;
        break;
      }
    }
      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
    }
  }
    break;

  case  MCT_EVENT_MODULE_STATS_GET_DATA: {
  }
    break;

  case MCT_EVENT_MODULE_SET_STREAM_CONFIG: {
    sensor_out_info_t *sensor_info =
      (sensor_out_info_t *)(mod_evt->module_event_data);

    AWB_LOW(" MCT_EVENT_MODULE_SET_STREAM_CONFIG");

    private->max_sensor_delay = sensor_info->sensor_immediate_pipeline_delay +
      sensor_info->sensor_additive_pipeline_delay;

    /* Send sensor information to AWB */
    q3a_thread_aecawb_msg_t *awb_msg_sensor = awb_port_malloc_msg(MSG_AWB_SET,
      AWB_SET_PARAM_INIT_SENSOR_INFO);
    if (NULL == awb_msg_sensor) {
      break;
    }
    awb_msg_sensor->u.awb_set_parm.u.init_param.sensor_info.sensor_res_height =
      sensor_info->request_crop.last_line -
      sensor_info->request_crop.first_line + 1;
    awb_msg_sensor->u.awb_set_parm.u.init_param.sensor_info.sensor_res_width =
      sensor_info->request_crop.last_pixel -
      sensor_info->request_crop.first_pixel + 1;
    awb_msg_sensor->u.awb_set_parm.u.init_param.sensor_info.sensor_top =
      sensor_info->request_crop.first_line;
    awb_msg_sensor->u.awb_set_parm.u.init_param.sensor_info.sensor_left =
      sensor_info->request_crop.first_pixel;

    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg_sensor);
    if (FALSE == rc) {
      AWB_ERR("Fail to queue sensor info data");
      break;
    }
    awb_msg_sensor = NULL; /* Avoid further usage in this case statement */


    /* Send operation mode to AWB */
    q3a_thread_aecawb_msg_t *awb_msg_mode = awb_port_malloc_msg(MSG_AWB_SET,
      AWB_SET_PARAM_OP_MODE);
    if (NULL == awb_msg_mode) {
      break;
    }

    switch (private->stream_type) {
    case CAM_STREAM_TYPE_VIDEO: {
       awb_msg_mode->u.awb_set_parm.u.init_param.op_mode =
         Q3A_OPERATION_MODE_CAMCORDER;
    }
      break;

    case CAM_STREAM_TYPE_CALLBACK:
    case CAM_STREAM_TYPE_PREVIEW: {
      awb_msg_mode->u.awb_set_parm.u.init_param.op_mode =
        Q3A_OPERATION_MODE_PREVIEW;
    }
      break;

    case CAM_STREAM_TYPE_RAW:
    case CAM_STREAM_TYPE_SNAPSHOT: {
      awb_msg_mode->u.awb_set_parm.u.init_param.op_mode =
        Q3A_OPERATION_MODE_SNAPSHOT;
    }
      break;

    default: {
      awb_msg_mode->u.awb_set_parm.u.init_param.op_mode =
        Q3A_OPERATION_MODE_PREVIEW;
    }
      break;
    } /* switch (private->stream_type) */

    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg_mode);
    awb_msg_mode = NULL; /* Avoid further usage in this case statement */

    /* Also send the stream dimensions for preview, required to config ISP */
    if ((private->stream_type == CAM_STREAM_TYPE_PREVIEW) ||
        (private->stream_type == CAM_STREAM_TYPE_CALLBACK)||
        (private->stream_type == CAM_STREAM_TYPE_VIDEO)) {
      awb_set_parameter_init_t *init_param = NULL;
      q3a_thread_aecawb_msg_t  *dim_msg = awb_port_malloc_msg(MSG_AWB_SET,
        AWB_SET_PARAM_UI_FRAME_DIM);
      if (NULL == dim_msg) {
        AWB_ERR(" malloc failed for dim_msg");
        break;
      }
      init_param = &(dim_msg->u.awb_set_parm.u.init_param);
      init_param->frame_dim.width = private->preview_width;
      init_param->frame_dim.height = private->preview_height;
      AWB_LOW("enqueue msg update ui width %d and height %d",
        init_param->frame_dim.width, init_param->frame_dim.height);

      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, dim_msg);
    }
  } /* MCT_EVENT_MODULE_SET_STREAM_CONFIG*/
    break;

  case MCT_EVENT_MODULE_MODE_CHANGE: {
    //Stream mode has changed
    private->stream_type =
      ((stats_mode_change_event_data*)
      (event->u.module_event.module_event_data))->stream_type;
    private->reserved_id =
      ((stats_mode_change_event_data*)
      (event->u.module_event.module_event_data))->reserved_id;
  }
    break;

  case MCT_EVENT_MODULE_PPROC_GET_AWB_UPDATE: {
    stats_get_data_t *stats_get_data =
      (stats_get_data_t *)mod_evt->module_event_data;

    if (!stats_get_data) {
      AWB_ERR("failed\n");
      break;
    }

    awb_port_proc_get_awb_data(port, stats_get_data);
  } /* MCT_EVENT_MODULE_PPROC_GET_AEC_UPDATE X*/
    break;

  case MCT_EVENT_MODULE_FACE_INFO: {
    mct_face_info_t *face_info = (mct_face_info_t *)mod_evt->module_event_data;
    if (!face_info) {
      AWB_ERR("failed\n");
      break;
    }
    q3a_thread_aecawb_msg_t *awb_msg =
      (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL) {
      uint8_t idx = 0;
      uint32_t face_count = face_info->face_count;
      if(face_count > MAX_STATS_ROI_NUM) {
        AWB_HIGH("face_count %d exceed stats roi limitation, cap to max", face_count);
        face_count = MAX_STATS_ROI_NUM;
      }
      if(face_count > MAX_ROI) {
        AWB_HIGH("face_count %d exceed max roi limitation, cap to max", face_count);
        face_count = MAX_ROI;
      }

      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SET;
      awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_FD_ROI;
      awb_msg->u.awb_set_parm.u.awb_roi_info.type = ROI_TYPE_FACE;
      awb_msg->u.awb_set_parm.u.awb_roi_info.num_roi = face_count;
      for (idx = 0; idx < awb_msg->u.awb_set_parm.u.awb_roi_info.num_roi; idx++) {
        awb_msg->u.awb_set_parm.u.awb_roi_info.roi[idx].x =
          face_info->orig_faces[idx].roi.left;
        awb_msg->u.awb_set_parm.u.awb_roi_info.roi[idx].y =
          face_info->orig_faces[idx].roi.top;
        awb_msg->u.awb_set_parm.u.awb_roi_info.roi[idx].dx =
          face_info->orig_faces[idx].roi.width;
        awb_msg->u.awb_set_parm.u.awb_roi_info.roi[idx].dy =
          face_info->orig_faces[idx].roi.height;
      }
      rc = q3a_aecawb_thread_en_q_msg(private->thread_data,awb_msg);
    }
  }
    break;

  case MCT_EVENT_MODULE_ISP_OUTPUT_DIM: {
    mct_stream_info_t *stream_info =
      (mct_stream_info_t *)(event->u.module_event.module_event_data);
    if (NULL == stream_info) {
      AWB_ERR("Error: NULL");
      break;
    }

    if (stream_info->stream_type == CAM_STREAM_TYPE_PREVIEW) {
      private->vfe_out_width  = stream_info->dim.width;
      private->vfe_out_height = stream_info->dim.height;
    }
  }
    break;

  case MCT_EVENT_MODULE_STREAM_CROP: {
    mct_bus_msg_stream_crop_t *stream_crop =
      (mct_bus_msg_stream_crop_t *)mod_evt->module_event_data;
    if(!stream_crop){
      AWB_ERR("failed");
      break;
    }

    q3a_thread_aecawb_msg_t *awb_msg =
      (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
    if(!awb_msg){
      break;
    }
    memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
    awb_msg->type = MSG_AWB_SET;
    awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_CROP_INFO;
    awb_msg->u.awb_set_parm.u.stream_crop.pp_x = stream_crop->x;
    awb_msg->u.awb_set_parm.u.stream_crop.pp_y = stream_crop->y;
    awb_msg->u.awb_set_parm.u.stream_crop.pp_crop_out_x = stream_crop->crop_out_x;
    awb_msg->u.awb_set_parm.u.stream_crop.pp_crop_out_y = stream_crop->crop_out_y;
    awb_msg->u.awb_set_parm.u.stream_crop.vfe_map_x = stream_crop->x_map;
    awb_msg->u.awb_set_parm.u.stream_crop.vfe_map_y = stream_crop->y_map;
    awb_msg->u.awb_set_parm.u.stream_crop.vfe_map_width = stream_crop->width_map;
    awb_msg->u.awb_set_parm.u.stream_crop.vfe_map_height = stream_crop->height_map;
    awb_msg->u.awb_set_parm.u.stream_crop.vfe_out_width = private->vfe_out_width;
    awb_msg->u.awb_set_parm.u.stream_crop.vfe_out_height = private->vfe_out_height;

    AWB_HIGH("Crop Event from ISP received. PP (%d %d %d %d)", stream_crop->x,
      stream_crop->y, stream_crop->crop_out_x, stream_crop->crop_out_y);
    AWB_HIGH("vfe map: (%d %d %d %d) vfe_out: (%d %d)", stream_crop->x_map,
      stream_crop->y_map, stream_crop->width_map, stream_crop->height_map,
      private->vfe_out_width, private->vfe_out_height);

    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
  }
    break;

  case MCT_EVENT_MODULE_PREVIEW_STREAM_ID: {
    mct_stream_info_t  *stream_info =
      (mct_stream_info_t *)(mod_evt->module_event_data);

    AWB_HIGH("Preview stream-id event: stream_type: %d width: %d height: %d",
      stream_info->stream_type, stream_info->dim.width, stream_info->dim.height);

    private->preview_width = stream_info->dim.width;
    private->preview_height = stream_info->dim.height;
  }
    break;

  case MCT_EVENT_MODULE_REQUEST_STATS_TYPE: {
    uint32_t required_stats_mask = 0;
    isp_rgn_skip_pattern rgn_skip_pattern = RGN_SKIP_PATTERN_MAX;
    mct_event_request_stats_type *stats_info =
      (mct_event_request_stats_type *)mod_evt->module_event_data;
    q3a_thread_aecawb_msg_t *awb_msg = awb_port_malloc_msg(MSG_AWB_GET,
      AWB_REQUIRED_STATS);
    if (NULL == awb_msg) {
      AWB_ERR("malloc failed for AWB_REQUIRED_STATS");
      rc = FALSE;
      break;
    }

    /* Fill msg with the supported stats data */
    awb_msg->u.awb_get_parm.u.request_stats.supported_stats_mask =
      stats_info->supported_stats_mask;
    awb_msg->u.awb_get_parm.u.request_stats.stats_stream_type =
      stats_info->isp_streaming_type;
    awb_msg->u.awb_get_parm.u.request_stats.supported_rgn_skip_mask =
      stats_info->supported_rgn_skip_mask;
    /* Get the list of require stats from algo library */
    awb_msg->sync_flag = TRUE;
    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
    required_stats_mask = awb_msg->u.awb_get_parm.u.request_stats.enable_stats_mask;
    rgn_skip_pattern =
      (isp_rgn_skip_pattern)(awb_msg->u.awb_get_parm.u.request_stats.enable_rgn_skip_pattern);
    free(awb_msg);
    awb_msg = NULL;
    if (!rc) {
      AWB_ERR("Error: fail to get required stats");
      break;
    }

    /* Verify if require stats are supported */
    if (required_stats_mask !=
        (stats_info->supported_stats_mask & required_stats_mask)) {
      AWB_ERR("Error: Stats not supported: 0x%x, supported stats = 0x%x",
        required_stats_mask, stats_info->supported_stats_mask);
      rc = FALSE;
      break;
    }

    /* Update query and save internally */
    stats_info->enable_rgn_skip_pattern[MSM_ISP_STATS_BG] = rgn_skip_pattern;
    stats_info->enable_stats_mask |= required_stats_mask;
    if (ISP_STREAMING_OFFLINE == stats_info->isp_streaming_type) {
      private->required_stats_mask_offline = required_stats_mask;
    } else {
      private->required_stats_mask = required_stats_mask;
    }

    if (private->required_stats_mask & (1 << MSM_ISP_STATS_BG)) {
      private->bg_stats_enabled = TRUE;
    }

    AWB_HIGH("MCT_EVENT_MODULE_REQUEST_STATS_TYPE:Required AWB stats mask = 0x%x",
      private->required_stats_mask);
  }
    break;

  case MCT_EVENT_MODULE_ISP_STATS_INFO: {
    mct_stats_info_t *stats_info =
      (mct_stats_info_t *)mod_evt->module_event_data;

    q3a_thread_aecawb_msg_t *stats_msg = awb_port_malloc_msg(MSG_AWB_SET,
      AWB_SET_PARAM_STATS_DEPTH);

    if (!stats_msg) {
      AWB_ERR("malloc failed for stats_msg");
      break;
    }
    stats_msg->u.awb_set_parm.u.stats_depth = stats_info->stats_depth;
    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, stats_msg);
  }
    break;
  case MCT_EVENT_MODULE_TRIGGER_CAPTURE_FRAME: {
     AWB_HIGH("MCT_EVENT_MODULE_TRIGGER_CAPTURE_FRAME!");
     awb_port_unified_flash_trigger(port);
   }
    break;

  default: {
  }
    break;
  } /* switch (mod_evt->type) */

  return rc;
}

static boolean awb_port_parse_set_param(awb_port_private_t  *private, awb_set_parameter_t  *awb_param)
{
  int rc = TRUE;
  switch(awb_param->type){
    /* HAL 1 & HAL3 Manual AWB */
    case AWB_SET_PARAM_MANUAL_WB: {
      private->manual.valid = TRUE;
      private->manual.manual_wb_type = awb_param->u.manual_wb_params.type;

      if (MANUAL_WB_MODE_GAIN == private->manual.manual_wb_type) {
        private->manual.u.manual_gain.r_gain =
          awb_param->u.manual_wb_params.u.gains.r_gain;
        private->manual.u.manual_gain.g_gain =
          awb_param->u.manual_wb_params.u.gains.g_gain;
        private->manual.u.manual_gain.b_gain =
          awb_param->u.manual_wb_params.u.gains.b_gain;

        rc = private->awb_object.awb_ops.estimate_cct(private->awb_object.awb,
            private->manual.u.manual_gain.r_gain,
            private->manual.u.manual_gain.g_gain,
            private->manual.u.manual_gain.b_gain,
            &private->manual.u.cct);

      } else if (MANUAL_WB_MODE_CCT == private->manual.manual_wb_type) {
        private->manual.u.cct = awb_param->u.manual_wb_params.u.cct;

        rc = private->awb_object.awb_ops.estimate_gains(private->awb_object.awb,
            &private->manual.u.manual_gain.r_gain,
            &private->manual.u.manual_gain.g_gain,
            &private->manual.u.manual_gain.b_gain,
            private->manual.u.cct);
      }
      if (!rc) {
        AWB_ERR("Fail to set manual WB");
      }

      /* Do not pass this MSG down to algorithm */
      rc = FALSE;
    }
    break;
    case AWB_SET_PARAM_WHITE_BALANCE:{
      boolean enable;
      if(awb_param->u.awb_current_wb == CAMERA_WB_OFF ||
         awb_param->u.awb_current_wb == CAMERA_WB_MANUAL) {
        /* private->awb_auto_mode will be set to false */
        enable = 0;
      } else {
        enable = 1;
      }
      private->awb_on_off_mode = enable;
      awb_port_set_awb_mode( private);

      /* change the mode right away for HAL3 */
      awb_output_data_t output;
      output.wb_mode = awb_param->u.awb_current_wb;
      awb_port_update_wb(private, &output);
    }
    break;
    case AWB_SET_PARAM_LOCK:{
      // keep it for manual mode, for other mode,
      // private->awb_locked will be automatically updated by callback
      private->awb_locked = awb_param->u.awb_lock;
    }
    break;
    case AWB_SET_PARM_FAST_AEC_DATA: {
      private->fast_aec_data = awb_param->u.fast_aec_data;
    }
    break;
    default:{
    }
  }
  return rc;
}
static boolean awb_port_proc_downstream_ctrl(mct_port_t *port,
  mct_event_t *event)
{
  boolean             rc = TRUE;
  awb_port_private_t  *private = (awb_port_private_t *)(port->port_private);
  mct_event_control_t *mod_ctrl = (mct_event_control_t *)&(event->u.ctrl_event);

  /* check if there's need for extended handling. */
  if (private->func_tbl.ext_handle_control_event) {
    stats_ext_return_type ret = STATS_EXT_HANDLING_PARTIAL;
    AWB_LOW("Handle extended control event!");
    ret = private->func_tbl.ext_handle_control_event(port, mod_ctrl);
    /* Check if this event has been completely handled. If not we'll
       process it further here. */
    if (STATS_EXT_HANDLING_COMPLETE == ret) {
      AWB_LOW("Control event %d handled by extended functionality!",
        mod_ctrl->type);
      return rc;
    }
  }

  switch (mod_ctrl->type) {
  case MCT_EVENT_CONTROL_SOF: {
    if (private->bg_stats_enabled) {
      rc = awb_port_event_sof(port, event);
    }
  }
    break;

  case MCT_EVENT_CONTROL_SET_PARM: {
    /* some logic shall be handled by stats and q3a port
     * to achieve that, we need to add the function to find the desired sub port
     * however since it is not in place, for now, handle it here
     */
    stats_set_params_type *stat_parm =
      (stats_set_params_type *)mod_ctrl->control_event_data;

    if (stat_parm->param_type == STATS_SET_Q3A_PARAM) {
      q3a_set_params_type     *q3a_param = &(stat_parm->u.q3a_param);
      q3a_thread_aecawb_msg_t *awb_msg =
        (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

      if (awb_msg != NULL ) {
        memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));

        if (q3a_param->type == Q3A_SET_AWB_PARAM) {
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm = q3a_param->u.awb_param;
          rc = awb_port_parse_set_param(private, &q3a_param->u.awb_param);
           if(!rc){
            free(awb_msg);
            awb_msg = NULL;
            break;
          }
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        } else if (q3a_param->type == Q3A_ALL_SET_PARAM) {
          switch (q3a_param->u.q3a_all_param.type) {
          case Q3A_ALL_SET_EZTUNE_RUNNIG: {
            awb_msg->type = MSG_AWB_SET;
            awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_EZ_TUNE_RUNNING;
            awb_msg->u.awb_set_parm.u.ez_running =
              q3a_param->u.q3a_all_param.u.ez_runnig;

            rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
          }
            break;

          case Q3A_ALL_SET_DO_LED_EST_FOR_AF: {
            awb_msg->type = MSG_AWB_SET;
            awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_DO_LED_EST_FOR_AF;
            awb_msg->u.awb_set_parm.u.est_for_af =
              q3a_param->u.q3a_all_param.u.est_for_af;

            rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
          }
            break;

          case Q3A_ALL_SET_DUAL_LED_CALIB_MODE: {
            awb_msg->type = MSG_AWB_SET;
            awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_DUAL_LED_CALIB_MODE;
            awb_msg->u.awb_set_parm.u.dual_led_calib_mode =
              (boolean)q3a_param->u.q3a_all_param.u.dual_led_calib_mode;

            rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
          }
            break;

          default: {
            if (awb_msg) {
              free(awb_msg);
              awb_msg = NULL;
            }
          }
            break;
          }
        } else {
          free(awb_msg);
          awb_msg = NULL;
        }
      }
    } else if (stat_parm->param_type == STATS_SET_COMMON_PARAM) {
      stats_common_set_parameter_t *common_param =
        &(stat_parm->u.common_param);

      switch (common_param->type) {
      case COMMON_SET_PARAM_BESTSHOT: {
        q3a_thread_aecawb_msg_t *awb_msg =
          (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_BESTSHOT;
          awb_port_set_bestshot_mode(&awb_msg->u.awb_set_parm.u.awb_best_shot,
            common_param->u.bestshot_mode);

          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
        break;

      case COMMON_SET_PARAM_META_MODE: {
        q3a_thread_aecawb_msg_t *awb_msg =
          (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_META_MODE;
          awb_msg->u.awb_set_parm.u.awb_meta_mode =
            common_param->u.meta_mode;
          private->awb_meta_mode = common_param->u.meta_mode;
          awb_port_set_awb_mode( private);
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
        break;

      case COMMON_SET_CAPTURE_INTENT:{
        AWB_HIGH("capture type: %d", common_param->u.capture_type);
        if (common_param->u.capture_type == CAM_INTENT_STILL_CAPTURE) {
          q3a_thread_aecawb_msg_t *awb_msg =
            (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));
          if (awb_msg != NULL) {
            memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
            awb_msg->type = MSG_AWB_SET;
            awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_CAPTURE_MODE;
            awb_msg->u.awb_set_parm.u.capture_type =
              common_param->u.capture_type;

            rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
          }
          private->awb_still_capture_sof = private->cur_sof_id + 1;
          private->is_still_capture = TRUE;
        } else {
          private->is_still_capture = FALSE;
        }
      }
        break;

      case COMMON_SET_PARAM_VIDEO_HDR: {
        q3a_thread_aecawb_msg_t *awb_msg =
          (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_VIDEO_HDR;
          awb_msg->u.awb_set_parm.u.video_hdr =
            common_param->u.video_hdr;

          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
        break;

      case COMMON_SET_PARAM_SNAPSHOT_HDR: {
        q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
          malloc(sizeof(q3a_thread_aecawb_msg_t));
        if (awb_msg != NULL ) {
          awb_snapshot_hdr_type snapshot_hdr;
          if(common_param->u.snapshot_hdr == CAM_SENSOR_HDR_IN_SENSOR)
            snapshot_hdr = AWB_SENSOR_HDR_IN_SENSOR;
          else if(common_param->u.snapshot_hdr == CAM_SENSOR_HDR_ZIGZAG)
            snapshot_hdr = AWB_SENSOR_HDR_DRC;
          else
            snapshot_hdr = AWB_SENSOR_HDR_OFF;
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_SNAPSHOT_HDR;
          awb_msg->u.awb_set_parm.u.snapshot_hdr = snapshot_hdr;
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
        break;

      case COMMON_SET_PARAM_STATS_DEBUG_MASK: {
        q3a_thread_aecawb_msg_t *awb_msg =
          (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_STATS_DEBUG_MASK;

          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
        break;
      case COMMON_SET_PARAM_STREAM_ON_OFF: {
        private->thread_data->no_stats_mode = !common_param->u.stream_on;
        AWB_HIGH(" COMMON_SET_PARAM_STREAM_ON_OFF %d", common_param->u.stream_on);
        // stream off, need to flush existing stats
        // send a sync msg to
        // AWB & AEC share the thread, so only one STATS_MODE need
        // to be set for AEC/AWB
        if (!common_param->u.stream_on) {
          q3a_thread_aecawb_msg_t awb_msg;
          memset(&awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg.type = MSG_AECAWB_STATS_MODE;
          awb_msg.sync_flag = TRUE;
          q3a_aecawb_thread_en_q_msg(private->thread_data, &awb_msg);
          AWB_LOW(" COMMON_SET_PARAM_STREAM_ON_OFF end");
        }
        break;
      }
     case COMMON_SET_PARAM_UNIFIED_FLASH: {
       if (common_param->u.frame_info.num_batch != 0 &&
           private->stats_frame_capture.frame_capture_mode == TRUE) {
         AWB_HIGH("frame_capture mode in progress, don't process");
         break;
       }
       memset(&private->stats_frame_capture.frame_info, 0, sizeof(awb_frame_batch_t));
       private->stats_frame_capture.frame_capture_mode = FALSE;
       private->stats_frame_capture.frame_info.num_batch = common_param->u.frame_info.num_batch;
       if (0 == private->stats_frame_capture.frame_info.num_batch) {
         AWB_ERR("No. of num_batch is zero");
         break;
       }
       int i = 0;
       AWB_HIGH("No. of Batch from HAL = %d", private->stats_frame_capture.frame_info.num_batch);
       for (i = 0; i < private->stats_frame_capture.frame_info.num_batch; i++) {
         AWB_HIGH("frame_batch[%d] type: %d, flash_mode: %d", i,
           common_param->u.frame_info.configs[i].type,
           common_param->u.frame_info.configs[i].flash_mode);

         private->stats_frame_capture.frame_info.frame_batch[i].capture_type =
           common_param->u.frame_info.configs[i].type;
         if (CAM_CAPTURE_FLASH == common_param->u.frame_info.configs[i].type &&
             ((common_param->u.frame_info.configs[i].flash_mode == CAM_FLASH_MODE_ON) ||
             (common_param->u.frame_info.configs[i].flash_mode == CAM_FLASH_MODE_TORCH) ||
             (common_param->u.frame_info.configs[i].flash_mode == CAM_FLASH_MODE_AUTO))) {
           private->stats_frame_capture.frame_info.frame_batch[i].flash_mode = TRUE;
         } else{
           private->stats_frame_capture.frame_info.frame_batch[i].flash_mode = FALSE;
         }
       }

       rc = awb_port_unifed_request_batch_data_to_algo(private);
       if (FALSE == rc) {
         AWB_ERR("Fail to get batch data from AWB algo");
         memset(&private->stats_frame_capture, 0, sizeof(awb_frame_capture_t));
         break;
       }
     }
     break;

     case COMMON_SET_PARAM_LONGSHOT_MODE: {
        q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
          malloc(sizeof(q3a_thread_aecawb_msg_t));
        if (awb_msg != NULL ) {
          AWB_LOW("longshot_mode: %d", common_param->u.longshot_mode);
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_LONGSHOT_MODE;
          awb_msg->u.awb_set_parm.u.longshot_mode =
            common_param->u.longshot_mode;
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
     }
     break;
     case COMMON_SET_PARAM_INSTANT_AEC_DATA: {
        q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
          malloc(sizeof(q3a_thread_aecawb_msg_t));
        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARM_INSTANT_AEC_TYPE;
          awb_msg->u.awb_set_parm.u.instant_aec_type =
            common_param->u.instant_aec_type;
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
     }
     break;
     default: {
      }
     break;
      }
    }
   }
    break;

  case MCT_EVENT_CONTROL_STREAMON: {
    mct_event_t             event;
    stats_update_t          stats_update;
    q3a_thread_aecawb_msg_t *awb_msg =
      (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

    if (awb_msg != NULL ) {
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      memset(&stats_update, 0, sizeof(stats_update_t));

      awb_msg->sync_flag = TRUE;
      awb_msg->type = MSG_AWB_GET;
      awb_msg->u.awb_get_parm.type = AWB_GAINS;

      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);

      if (awb_msg) {
        awb_update_t* awb_update = &stats_update.awb_update;
        stats_proc_awb_gains_t* awb_core = &awb_msg->u.awb_get_parm.u.awb_gains;
        event.direction = MCT_EVENT_UPSTREAM;
        event.identity = private->reserved_id;
        event.type = MCT_EVENT_MODULE_EVENT;
        event.u.module_event.type = MCT_EVENT_MODULE_STATS_AWB_UPDATE;
        event.u.module_event.current_frame_id = mod_ctrl->current_frame_id;
        event.u.module_event.module_event_data = (void *)(&stats_update);;

        stats_update.flag = STATS_UPDATE_AWB;

        /* Update the CCM enable flag */
        private->awb_ccm_enable = awb_core->awb_ccm_enable;
        awb_update->ccm_update.awb_ccm_enable = private->awb_ccm_enable;

        if (!private->awb_auto_mode && private->manual.valid) {
          awb_update->gain.r_gain = private->manual.u.manual_gain.r_gain;
          awb_update->gain.g_gain = private->manual.u.manual_gain.g_gain;
          awb_update->gain.b_gain = private->manual.u.manual_gain.b_gain;
          awb_update->color_temp  = private->manual.u.cct;

          if (TRUE == private->awb_ccm_enable) {
            awb_ccm_type manual_ccm;
            rc = private->awb_object.awb_ops.estimate_ccm(
              private->awb_object.awb, private->manual.u.cct, &manual_ccm);

            /* Copy ccm output to awb update */
            awb_update->ccm_update.ccm_update_flag = TRUE;
            memcpy(&awb_update->ccm_update.ccm, &manual_ccm.ccm, sizeof(manual_ccm.ccm));
            memcpy(&awb_update->ccm_update.ccm_offset, &manual_ccm.ccm_offset,
              sizeof(manual_ccm.ccm_offset));
          }
          awb_port_print_log(awb_update, "STREAMON-AWB_MAN_UP", 0);
        } else {
          awb_update->gain.r_gain = awb_core->curr_gains.r_gain;
          awb_update->gain.g_gain = awb_core->curr_gains.g_gain;
          awb_update->gain.b_gain = awb_core->curr_gains.b_gain;
          awb_update->color_temp = awb_core->color_temp;
          awb_update->dual_led_setting.led1_high_setting = awb_core->led1_high_setting;
          awb_update->dual_led_setting.led2_high_setting = awb_core->led2_high_setting;
          awb_update->dual_led_setting.led1_low_setting = awb_core->led1_low_setting;
          awb_update->dual_led_setting.led2_low_setting = awb_core->led2_low_setting;
          awb_update->ccm_update.ccm_update_flag = awb_core->ccm.override_ccm;
          memcpy(&awb_update->ccm_update.ccm, &awb_core->ccm.ccm,
            sizeof(awb_update->ccm_update.ccm));
          memcpy(&awb_update->ccm_update.ccm_offset, &awb_core->ccm.ccm_offset,
            sizeof(awb_update->ccm_update.ccm_offset));
          awb_port_print_log(awb_update, "STREAMON-AWB_UP", 0);
        }

        AWB_LOW("send AWB_UPDATE to port =%p, event =%p",port, &event);

        MCT_PORT_EVENT_FUNC(port)(port, &event);
        free(awb_msg);
        awb_msg = NULL;
        MCT_OBJECT_LOCK(port);
        if(private->stream_type == CAM_STREAM_TYPE_SNAPSHOT)
          private->awb_update_flag = FALSE;
        MCT_OBJECT_UNLOCK(port);
      }
    } /* if (awb_msg != NULL ) */
  }
    break;

  case MCT_EVENT_CONTROL_START_ZSL_SNAPSHOT: {
    q3a_thread_aecawb_msg_t *awb_msg =
      (q3a_thread_aecawb_msg_t *)malloc(sizeof(q3a_thread_aecawb_msg_t));

    if (awb_msg == NULL) {
      break;
    }

    awb_msg->type = MSG_AWB_SET;
    awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_ZSL_START;
    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
  } /* MCT_EVENT_CONTROL_START_ZSL_SNAPSHOT */

    break;

  case MCT_EVENT_CONTROL_STOP_ZSL_SNAPSHOT: {
    private->stats_frame_capture.frame_capture_mode = FALSE;

    q3a_thread_aecawb_msg_t *awb_msg =
      (q3a_thread_aecawb_msg_t *)calloc(1, sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg == NULL) {
      break;
    }
    awb_msg->type = MSG_AWB_SET;
    awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_ZSL_STOP;
    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
  } /* MCT_EVENT_CONTROL_STOP_ZSL_SNAPSHOT */
    break;

  case MCT_EVENT_CONTROL_STREAMOFF: {
    private->stats_frame_capture.frame_capture_mode = FALSE;

    mct_stream_info_t *stream_info =
      (mct_stream_info_t*)event->u.ctrl_event.control_event_data;

    if (stream_info && (stream_info->stream_type == CAM_STREAM_TYPE_RAW ||
        stream_info->stream_type == CAM_STREAM_TYPE_SNAPSHOT)) {

      q3a_thread_aecawb_msg_t *awb_msg =
        (q3a_thread_aecawb_msg_t *)calloc(1, sizeof(q3a_thread_aecawb_msg_t));

      if (NULL != awb_msg) {
        /* Reset exposure settings */
        awb_msg->type = MSG_AWB_SET;
        awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_LED_RESET;
        rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
      }
    }
  }
    break;

  case MCT_EVENT_CONTROL_LINK_INTRA_SESSION: {
    cam_sync_related_sensors_event_info_t *link_param = NULL;
    uint32_t                               peer_identity = 0;

    link_param = (cam_sync_related_sensors_event_info_t *)
      (event->u.ctrl_event.control_event_data);
    peer_identity = link_param->related_sensor_session_id;
    AWB_LOW("AWB got MCT_EVENT_CONTROL_LINK_INTRA_SESSION to session %x", peer_identity);
    private->intra_peer_id = peer_identity;
    private->dual_cam_sensor_info = link_param->type;
  }
    break;

  case MCT_EVENT_CONTROL_UNLINK_INTRA_SESSION: {
    private->dual_cam_sensor_info = CAM_TYPE_MAIN;
    private->intra_peer_id = 0;
    AWB_HIGH("AWB got intra unlink Command");
  }
    break;

  case MCT_EVENT_CONTROL_OFFLINE_METADATA: {
    AWB_LOW("OFFLINE METADATA request received!");
    /* Offline metadata request should be a blocking call. Until
       we process offline stats and send AWB metadata up, we should
       block */
    awb_port_handle_offline_metadata_req(port, &private->offline_output);
    AWB_LOW("offline metadata already sent!");
  }
    break;
  default: {
  }
    break;
  }
  return rc;
}


/** awb_port_process_upstream_mod_event
 *    @port:
 *    @event:
 **/
static boolean awb_port_process_upstream_mod_event(mct_port_t *port,
  mct_event_t *event)
{
  boolean                 rc = FALSE;
  q3a_thread_aecawb_msg_t *awb_msg = NULL;
  mct_event_module_t      *mod_evt = &(event->u.module_event);
  awb_port_private_t      *private = (awb_port_private_t *)(port->port_private);
  mct_port_t              *peer;

  switch (mod_evt->type) {
  case MCT_EVENT_MODULE_STATS_AWB_CONFIG_UPDATE:
  case MCT_EVENT_MODULE_STATS_AWB_UPDATE:
  case MCT_EVENT_MODULE_STATS_POST_TO_BUS:
  case MCT_EVENT_MODULE_STATS_DATA_ACK:
  case MCT_EVENT_MODULE_STATS_AWB_MANUAL_UPDATE: {
    peer = MCT_PORT_PEER(port);
    rc = MCT_PORT_EVENT_FUNC(peer)(peer, event);
  }
    break;

  default: {/*shall not get here*/
  }
    break;
  }
  return rc;
}

/** awb_port_event
 *    @port:
 *    @event:
 *
 * awb sink module's event processing function. Received events could be:
 * AEC/AWB/AF Bayer stats;
 * Gyro sensor stat;
 * Information request event from other module(s);
 * Informatin update event from other module(s);
 * It ONLY takes MCT_EVENT_DOWNSTREAM event.
 *
 * Return TRUE if the event is processed successfully.
 **/
static boolean awb_port_event(mct_port_t *port, mct_event_t *event)
{
  boolean            rc = FALSE;
  awb_port_private_t *private;

  AWB_LOW("port =%p, evt_type: %d direction: %d", port, event->type,
    MCT_EVENT_DIRECTION(event));
  /* sanity check */
  if (!port || !event) {
    AWB_ERR("port or event NULL");
    return FALSE;
  }

  private = (awb_port_private_t *)(port->port_private);
  if (!private) {
    AWB_ERR("AWB private pointer NULL");
    return FALSE;
  }

  /* sanity check: ensure event is meant for port with same identity*/
  if ((private->reserved_id & 0xFFFF0000) != (event->identity & 0xFFFF0000)) {
    AWB_ERR("AWB identity didn't match!");
    return FALSE;
  }

  switch (MCT_EVENT_DIRECTION(event)) {
  case MCT_EVENT_DOWNSTREAM: {
    switch (event->type) {
    case MCT_EVENT_MODULE_EVENT: {
      rc = awb_process_downstream_mod_event( port, event);
    } /* case MCT_EVENT_MODULE_EVENT */
      break;

    case MCT_EVENT_CONTROL_CMD: {
      rc = awb_port_proc_downstream_ctrl(port,event);
    }
      break;

    default: {
    }
      break;
    }
  } /* case MCT_EVENT_DOWNSTREAM */
    break;

  case MCT_EVENT_UPSTREAM: {
    switch (event->type) {
    case MCT_EVENT_MODULE_EVENT: {
      rc = awb_port_process_upstream_mod_event(port, event);
    } /*case MCT_EVENT_MODULE_EVENT*/
      break;

    default: {
    }
      break;
    }
  } /* MCT_EVENT_UPSTREAM */
    break ;

  default: {
    rc = FALSE;
  }
    break;
  }

  AWB_LOW("X rc:%d", rc);
  return rc;
}

/** awb_port_ext_link
 *    @identity: session id + stream id
 *    @port:  awb module's sink port
 *    @peer:  q3a module's sink port
 **/
static boolean awb_port_ext_link(unsigned int identity,
  mct_port_t *port, mct_port_t *peer)
{
  boolean             rc = FALSE;
  awb_port_private_t  *private;

  AWB_LOW("E");

  /* awb sink port's external peer is always q3a module's sink port */
  if (!port || !peer ||
    strcmp(MCT_OBJECT_NAME(port), "awb_sink") ||
    strcmp(MCT_OBJECT_NAME(peer), "q3a_sink")) {
    AWB_ERR("Invalid Port/Peer!");
    return FALSE;
  }

  private = (awb_port_private_t *)port->port_private;
  if (!private) {
    AWB_ERR("Private port NULL!");
    return FALSE;
  }

  MCT_OBJECT_LOCK(port);
  switch (private->state) {
  case AWB_PORT_STATE_RESERVED:
  case AWB_PORT_STATE_UNLINKED:
  case AWB_PORT_STATE_LINKED:
    if ( (private->reserved_id & 0xFFFF0000) != (identity & 0xFFFF0000)) {
      break;
    }
  /*No break. Fall through.*/
  case AWB_PORT_STATE_CREATED:
    rc = TRUE;
    break;

  default:
    break;
  }

  if (rc == TRUE) {
    private->state = AWB_PORT_STATE_LINKED;
    MCT_PORT_PEER(port) = peer;
    MCT_OBJECT_REFCOUNT(port) += 1;
  }
  MCT_OBJECT_UNLOCK(port);
  mct_port_add_child(identity, port);

  return rc;
}

/** awb_port_ext_unlink
 *
 **/
static void awb_port_ext_unlink(unsigned int identity,
  mct_port_t *port, mct_port_t *peer)
{
  awb_port_private_t *private;

  if (!port || !peer || MCT_PORT_PEER(port) != peer) {
    return;
  }

  private = (awb_port_private_t *)port->port_private;
  if (!private) {
    return;
  }

  MCT_OBJECT_LOCK(port);
  if ((private->state == AWB_PORT_STATE_LINKED) &&
    (private->reserved_id & 0xFFFF0000) == (identity & 0xFFFF0000)) {

    MCT_OBJECT_REFCOUNT(port) -= 1;
    if (!MCT_OBJECT_REFCOUNT(port)) {
      private->state = AWB_PORT_STATE_UNLINKED;
      private->awb_update_flag = FALSE;
    }
  }
  MCT_OBJECT_UNLOCK(port);
  mct_port_remove_child(identity, port);

  return;
}

/** awb_port_set_caps
 *
 **/
static boolean awb_port_set_caps(mct_port_t *port, mct_port_caps_t *caps)
{
  if (strcmp(MCT_PORT_NAME(port), "awb_sink")) {
    return FALSE;
  }

  port->caps = *caps;
  return TRUE;
}

/** awb_port_check_caps_reserve
 *
 *
 *  AWB sink port can ONLY be re-used by ONE session. If this port
 *  has been in use, AWB module has to add an extra port to support
 *  any new session(via module_awb_request_new_port).
 **/
static boolean awb_port_check_caps_reserve(mct_port_t *port, void *caps,
  void *stream)
{
  boolean            rc = FALSE;
  mct_port_caps_t    *port_caps;
  awb_port_private_t *private;
  mct_stream_info_t  *stream_info = (mct_stream_info_t *)stream;

  AWB_LOW(":\n");

  MCT_OBJECT_LOCK(port);
  if (!port || !caps || !stream_info ||
      strcmp(MCT_OBJECT_NAME(port), "awb_sink")) {
    rc = FALSE;
    goto reserve_done;
  }

  port_caps = (mct_port_caps_t *)caps;
  if (port_caps->port_caps_type != MCT_PORT_CAPS_STATS) {
    rc = FALSE;
    goto reserve_done;
  }

  private = (awb_port_private_t *)port->port_private;
  switch (private->state) {
  case AWB_PORT_STATE_LINKED:
    if ((private->reserved_id & 0xFFFF0000) ==
      (stream_info->identity & 0xFFFF0000))
      rc = TRUE;
    break;

  case AWB_PORT_STATE_CREATED:
  case AWB_PORT_STATE_UNRESERVED: {
    private->reserved_id = stream_info->identity;
    private->stream_type = stream_info->stream_type;
    private->state       = AWB_PORT_STATE_RESERVED;
    rc = TRUE;
  }
    break;

  case AWB_PORT_STATE_RESERVED:
    if ((private->reserved_id & 0xFFFF0000) ==
      (stream_info->identity & 0xFFFF0000))
      rc = TRUE;
    break;

  default:
    rc = FALSE;
    break;
  }

reserve_done:
  MCT_OBJECT_UNLOCK(port);
  return rc;
}

/** awb_port_check_caps_unreserve:
 *
 *
 *
 **/
static boolean awb_port_check_caps_unreserve(mct_port_t *port,
  unsigned int identity)
{
  awb_port_private_t *private;

  if (!port || strcmp(MCT_OBJECT_NAME(port), "awb_sink")) {
    return FALSE;
  }

  private = (awb_port_private_t *)port->port_private;
  if (!private) {
    return FALSE;
  }

  MCT_OBJECT_LOCK(port);
  if ((private->state == AWB_PORT_STATE_UNLINKED   ||
     private->state == AWB_PORT_STATE_LINKED ||
     private->state == AWB_PORT_STATE_RESERVED) &&
    ((private->reserved_id & 0xFFFF0000) == (identity & 0xFFFF0000))) {

    if (!MCT_OBJECT_REFCOUNT(port)) {
      private->state       = AWB_PORT_STATE_UNRESERVED;
      private->reserved_id = (private->reserved_id & 0xFFFF0000);
    }
  }
  MCT_OBJECT_UNLOCK(port);

  return TRUE;
}

/** awb_port_find_identity
 *
 **/
boolean awb_port_find_identity(mct_port_t *port, unsigned int identity)
{
  awb_port_private_t *private;

  if ( !port || strcmp(MCT_OBJECT_NAME(port), "awb_sink")) {
    return FALSE;
  }

  private = port->port_private;

  if (private) {
    return ((private->reserved_id & 0xFFFF0000) == (identity & 0xFFFF0000) ?
      TRUE : FALSE);
  }

  return FALSE;
}

/** awb_port_deinit
 *    @port:
 **/
void awb_port_deinit(mct_port_t *port)
{
  awb_port_private_t *private;

  if (!port || strcmp(MCT_OBJECT_NAME(port), "awb_sink")) {
    return;
  }

  private = port->port_private;
  if (private) {
    /* Destroy offline processing semaphore*/
    sem_destroy(&private->sem_offline_proc);
    AWB_DESTROY_LOCK((&private->awb_object));
    private->awb_object.awb_ops.deinit(private->awb_object.awb);
    if (private->func_tbl.ext_deinit) {
      private->func_tbl.ext_deinit(port);
    }
    if (FALSE == private->awb_extension_use) {
      awb_port_unload_function(private);
    } else {
      awb_port_ext_unload_function(private);
    }
    free(private);
    private = NULL;
  }
}

/** awb_port_update_func_table:
 *    @private: pointer to internal aec pointer object
 *
 * Update extendable function pointers, with default values.
 *
 * Return: TRUE on success
 **/
boolean awb_port_update_func_table(awb_port_private_t *private)
{
  private->func_tbl.ext_init = NULL;
  private->func_tbl.ext_deinit = NULL;
  private->func_tbl.ext_callback = NULL;
  private->func_tbl.ext_handle_module_event = NULL;
  private->func_tbl.ext_handle_control_event = NULL;
  return TRUE;
}

/** awb_port_init:
 *    @port: awb's sink port to be initialized
 *
 *  awb port initialization entry point. Becase AWB module/port is
 *  pure software object, defer awb_port_init when session starts.
 **/
boolean awb_port_init(mct_port_t *port, unsigned int *session_id)
{
  boolean            rc = TRUE;
  mct_port_caps_t    caps;
  unsigned int       *session;
  mct_list_t         *list;
  awb_port_private_t *private;

  if (!port || strcmp(MCT_OBJECT_NAME(port), "awb_sink")) {
    return FALSE;
  }

  private = (void *)malloc(sizeof(awb_port_private_t));
  if (!private) {
    return FALSE;
  }
  memset(private, 0 , sizeof(awb_port_private_t));
  AWB_INITIALIZE_LOCK(&private->awb_object);

  private->reserved_id       = *session_id;
  private->state             = AWB_PORT_STATE_CREATED;
  private->awb_state         = CAM_AWB_STATE_INACTIVE;
  private->awb_last_state    = CAM_AWB_STATE_INACTIVE;
  private->awb_locked        = FALSE;
  private->op_mode           = Q3A_OPERATION_MODE_NONE;
  private->awb_auto_mode     = TRUE;
  private->awb_meta_mode     = CAM_CONTROL_AUTO;
  private->dual_cam_sensor_info = CAM_TYPE_MAIN;

  private->awb_converge.frames_req = AWB_PORT_CONVERGE_REQ;
  memset(&(private->awb_roi), 0, sizeof(private->awb_roi));

  /* Initialize offline stats semaphore */
  if (0 != sem_init(&private->sem_offline_proc, 0, 0)) {
    AWB_HIGH("Failure initializing offline stats semaphore!");
  }

  port->port_private  = private;
  port->direction     = MCT_PORT_SINK;
  caps.port_caps_type = MCT_PORT_CAPS_STATS;
  caps.u.stats.flag   = (MCT_PORT_CAP_STATS_Q3A | MCT_PORT_CAP_STATS_CS_RS);

  /* Set default functions to keep clean & bug free code*/
  rc &= awb_port_load_dummy_default_func(&private->awb_object.awb_ops);
  rc &= awb_port_update_func_table(private);

  /* this is sink port of awb module */
  mct_port_set_event_func(port, awb_port_event);
  mct_port_set_ext_link_func(port, awb_port_ext_link);
  mct_port_set_unlink_func(port, awb_port_ext_unlink);
  mct_port_set_set_caps_func(port, awb_port_set_caps);
  mct_port_set_check_caps_reserve_func(port, awb_port_check_caps_reserve);
  mct_port_set_check_caps_unreserve_func(port, awb_port_check_caps_unreserve);

  if (port->set_caps) {
    port->set_caps(port, &caps);
  }
  return rc;
}

/** awb_port_set_stored_parm:
 *    @port: AWB port pointer
 *    @stored_params: Previous session stored parameters.
 *
 * This function stores the previous session parameters.
 *
 **/
void awb_port_set_stored_parm(mct_port_t *port, awb_stored_params_type* stored_params)
{
  awb_port_private_t *private =(awb_port_private_t *)port->port_private;

  if (!stored_params || !private) {
    AWB_ERR("awb port or init param pointer NULL");
    return;
  }

  private->stored_params = stored_params;
}

