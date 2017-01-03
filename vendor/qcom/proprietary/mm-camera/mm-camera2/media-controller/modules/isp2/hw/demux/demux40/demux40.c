/* demux40.c
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */


/* std headers */
#include <unistd.h>
#include <math.h>

/* mctl headers */
#include "modules.h"

#undef ISP_DBG
#define ISP_DBG(fmt, args...) \
  ISP_DBG_MOD(ISP_LOG_DEMUX, fmt, ##args)
#undef ISP_HIGH
#define ISP_HIGH(fmt, args...) \
  ISP_HIGH_MOD(ISP_LOG_DEMUX, fmt, ##args)

/* isp headers */
#include "isp_sub_module_log.h"
#include "isp_sub_module_util.h"
#include "module_demux40.h"

/* #define DMUX_DEBUG */
#ifdef DMUX_DEBUIG
#undef ISP_HIGH
#define ISP_HIGH ISP_ERR
#undef ISP_DBG
#define ISP_DBG ISP_ERR
#endif

/* TODO: need to clean this, 3D used.*/
#define VFE_CMD_DEMUX_R_CHANNEL_GAIN_CONFIG 200

#define MAX_DEMUX_GAIN 31.9  /* max 32x, should we use 32? */

#define INIT_GAIN(p_gain, val) ( {\
  p_gain.blue = val; \
  p_gain.red = val; \
  p_gain.green_even = val; \
  p_gain.green_odd = val; \
})

/** demux40_debug:
 *    @pcmd: configuration command
 *
 *  This function runs in ISP HW thread context.
 *  This function dumps demux configuration
 *
 *  Return: None
 **/
static void demux40_debug(ISP_DemuxConfigCmdType* pcmd)
{
  ISP_DBG("ISP_DemuxConfigCmd.ch0OddGain = %d",
    pcmd->ch0OddGain);
  ISP_DBG("ISP_DemuxConfigCmd.ch0EvenGain = %d",
    pcmd->ch0EvenGain);
  ISP_DBG("ISP_DemuxConfigCmd.ch1Gain = %d",
    pcmd->ch1Gain);
  ISP_DBG("ISP_DemuxConfigCmd.ch2Gain = %d",
    pcmd->ch2Gain);
  ISP_DBG("ISP_DemuxConfigCmd.period = %d",
    pcmd->period);
  ISP_DBG("ISP_DemuxConfigCmd.evenCfg = %d",
    pcmd->evenCfg);
  ISP_DBG("ISP_DemuxConfigCmd.oddCfg = %d",
    pcmd->oddCfg);
} /* demux40_debug */

/** demux40_gain_debug:
 *    @pcmd: configuration command
 *
 *  This function runs in ISP HW thread context.
 *  This function dumps demux gain configuration
 *
 *  Return: None
 **/
static void demux40_gain_debug(ISP_DemuxGainCfgCmdType* pcmd)
{
  ISP_DBG("ISP_DemuxGainCfgCmdType.ch0OddGain = %d",
    pcmd->ch0OddGain);
  ISP_DBG("ISP_DemuxGainCfgCmdType.ch0EvenGain = %d",
    pcmd->ch0EvenGain);
  ISP_DBG("ISP_DemuxGainCfgCmdType.ch1Gain = %d",
    pcmd->ch1Gain);
  ISP_DBG("ISP_DemuxGainCfgCmdType.ch2Gain = %d",
    pcmd->ch2Gain);
} /* demux40_gain_debug */

/** demux40_stats_aec_update:
 *
 * @mod: demux module
 * @data: handle to stats_update_t
 *
 * Handle AEC update event
 *
 * Return TRUE on success and FALSE on failure
 **/
boolean demux40_set_digital_gain(mct_module_t *module,
  isp_sub_module_t *isp_sub_module, mct_event_t *event)
{
  demux40_t            *demux = NULL;
  float                 new_dig_gain= 0.0f;
  float                *cur_dig_gain;

  if (!module || !event || !isp_sub_module) {
    ISP_ERR("failed: %p %p %p", module, event, isp_sub_module);
    return FALSE;
  }

  demux = (demux40_t *)isp_sub_module->private_data;
  if (!demux) {
    ISP_ERR("failed: mod %p", demux);
    return FALSE;
  }

  new_dig_gain = *(float *)event->u.module_event.module_event_data;
  ISP_DBG(": dig gain %5.3f", new_dig_gain);
  if (new_dig_gain < 1.0)
    new_dig_gain = 1.0;

  PTHREAD_MUTEX_LOCK(&isp_sub_module->mutex);

  cur_dig_gain = &demux->dig_gain;

  if (isp_sub_module->trigger_update_pending == FALSE) {
    if (!F_EQUAL(*cur_dig_gain, new_dig_gain)) {
      isp_sub_module->trigger_update_pending = TRUE;
    }
  }
  /* Store Digital gain in module private */
  *cur_dig_gain = new_dig_gain;

  PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
  return TRUE;
}

/** demux40_do_hw_update:
 *    @demux_mod: demux module instance
 *
 *  This function runs in ISP HW thread context.
 *
 *  This function checks and sends configuration update to kernel
 *
 *  Return:   0 - Success
 *           -1 - configuration error
 **/
static int demux40_store_hw_update(isp_sub_module_t *isp_sub_module,
  demux40_t *demux)
{
  boolean ret = TRUE;
  int i, rc = 0;
  struct msm_vfe_cfg_cmd_list   *hw_update = NULL;
  struct msm_vfe_cfg_cmd2       *cfg_cmd = NULL;
  struct msm_vfe_reg_cfg_cmd    *reg_cfg_cmd = NULL;
  ISP_DemuxConfigCmdType        *reg_cmd = NULL;

#ifdef PRINT_REG_VAL_SET
  uint32_t *data = NULL;
  uint32_t len = 0;
#endif

  if (!demux) {
    ISP_ERR("failed: %p", demux);
    return FALSE;
  }

  hw_update = (struct msm_vfe_cfg_cmd_list *)malloc(sizeof(*hw_update));
  if (!hw_update) {
    ISP_ERR("failed: hw_update %p", hw_update);
    return FALSE;
  }
  memset(hw_update, 0, sizeof(*hw_update));
  cfg_cmd = &hw_update->cfg_cmd;
  reg_cfg_cmd = (struct msm_vfe_reg_cfg_cmd *)
    malloc(sizeof(struct msm_vfe_reg_cfg_cmd));
  if (!reg_cfg_cmd) {
    ISP_ERR("failed: reg_cfg_cmd %p", reg_cfg_cmd);
    goto ERROR_REG_CFG_MALLOC;
  }
  memset(reg_cfg_cmd, 0, (sizeof(struct msm_vfe_reg_cfg_cmd)));

  reg_cmd = (ISP_DemuxConfigCmdType *)malloc(sizeof(*reg_cmd));
  if (!reg_cmd) {
    ISP_ERR("failed: reg_cfg_cmd %p", reg_cfg_cmd);
    goto ERROR_REG_CMD_MALLOC;
  }
  memset(reg_cmd, 0, sizeof(*reg_cmd));
  *reg_cmd = demux->ISP_DemuxConfigCmd;

  cfg_cmd->cfg_data = (void *) reg_cmd;
  cfg_cmd->cmd_len = sizeof(*reg_cmd);
  cfg_cmd->cfg_cmd = (void *) reg_cfg_cmd;
  cfg_cmd->num_cfg = 1;

  reg_cfg_cmd[0].u.rw_info.cmd_data_offset = 0;
  reg_cfg_cmd[0].cmd_type = VFE_WRITE;
  reg_cfg_cmd[0].u.rw_info.reg_offset = ISP_DEMUX40_OFF;
  reg_cfg_cmd[0].u.rw_info.len = ISP_DEMUX40_LEN * sizeof(uint32_t);

  demux40_debug(&demux->ISP_DemuxConfigCmd);

#ifdef PRINT_REG_VAL_SET
  ISP_HIGH("hw_reg_offset %x, len %d", reg_cfg_cmd[0].u.rw_info.reg_offset,
    reg_cfg_cmd[0].u.rw_info.len);
  data = (uint32_t *)cfg_cmd->cfg_data;
  for (len = 0; len < (cfg_cmd->cmd_len / 4); len++) {
    ISP_HIGH("data[%d] %x", len, data[len]);
  }
#endif

  ISP_LOG_LIST("hw_update %p cfg_cmd %p", hw_update, cfg_cmd->cfg_cmd);
  ret = isp_sub_module_util_store_hw_update(isp_sub_module, hw_update);
  if (ret == FALSE) {
    ISP_ERR("failed: isp_sub_module_util_store_hw_update %d", ret);
    goto ERROR_APPEND;
  }

  isp_sub_module->trigger_update_pending = FALSE;
  demux->applied_cmd = demux->ISP_DemuxConfigCmd;

  return ret;

ERROR_APPEND:
  free(reg_cmd);
ERROR_REG_CMD_MALLOC:
  free(reg_cfg_cmd);
ERROR_REG_CFG_MALLOC:
  free(hw_update);

  return ret;
} /* demux40_do_hw_update */

/** demux40_r_image_gain_update:
 *    @demux: demux module instance
 *
 *  This function runs in ISP HW thread context.
 *
 *  This function dumps demux gain configuration
 *
 *  Return: 0
 **/
static int demux40_r_image_gain_update(demux40_t *demux)
{
  ISP_DemuxGainCfgCmdType* p_gaincmd = &demux->ISP_RImageGainConfigCmd;
  p_gaincmd->ch0OddGain = DEMUX_GAIN(demux->r_gain.green_odd);
  p_gaincmd->ch0EvenGain = DEMUX_GAIN(demux->r_gain.green_even);
  p_gaincmd->ch1Gain = DEMUX_GAIN(demux->r_gain.blue);
  p_gaincmd->ch2Gain = DEMUX_GAIN(demux->r_gain.red);

  return 0;
} /* demux40_r_image_gain_update */

/** demux40_set_cfg_params:
 *    @pcmd: configuration commands
 *    @fmt: pixel pattern
 *
 *  This function runs in ISP HW thread context.
 *  This function prepares demux configuration parameters
 *
 *  Return: TRUE  - Success
 *          FALSE - Unsupported pixel pattern
 **/
static boolean demux40_set_cfg_params(ISP_DemuxConfigCmdType* pcmd,
  cam_format_t fmt)
{
  boolean ret = TRUE;
  enum ISP_START_PIXEL_PATTERN pix_pattern;

  /* Configure VFE input format, Send Input format command */
  pix_pattern = isp_sub_module_fmt_to_pix_pattern(fmt);
  ISP_DBG("format %d pix_pattern %d",fmt, pix_pattern);

  switch (pix_pattern) {
  /* bayer patterns */
  case ISP_BAYER_GBGBGB:
    pcmd->period = 1;
    pcmd->evenCfg = 0xAC;
    pcmd->oddCfg = 0xC9;
    break;

  case ISP_BAYER_BGBGBG:
    pcmd->period = 1;
    pcmd->evenCfg = 0xCA;
    pcmd->oddCfg = 0x9C;
    break;

  case ISP_BAYER_GRGRGR:
    pcmd->period = 1;
    pcmd->evenCfg = 0x9C;
    pcmd->oddCfg = 0xCA;
    break;

  case ISP_BAYER_RGRGRG:
    pcmd->period = 1;
    pcmd->evenCfg = 0xC9;
    pcmd->oddCfg = 0xAC;
    break;

    /* YCbCr Patterns */
  case ISP_YUV_YCbYCr:
    pcmd->period = 3;
    pcmd->evenCfg = 0x9CAC;
    pcmd->oddCfg = 0x9CAC;
    break;

  case ISP_YUV_YCrYCb:
    pcmd->period = 3;
    pcmd->evenCfg = 0xAC9C;
    pcmd->oddCfg = 0xAC9C;
    break;

  case ISP_YUV_CbYCrY:
    pcmd->period = 3;
    pcmd->evenCfg =0xC9CA;
    pcmd->oddCfg = 0xC9CA;
    break;

  case ISP_YUV_CrYCbY:
    pcmd->period = 3;
    pcmd->evenCfg =0xCAC9;
    pcmd->oddCfg =0xCAC9;
    break;

  default:
    ISP_ERR("Error ISP input not configured!!!");
    ret = FALSE;
    break;
  }

  return ret;
}/* demux40_set_cfg_params */

static void demux40_ez_isp_update(demux40_t *demux_module,
  demuxchannelgain_t *gain)
{
  ISP_DemuxConfigCmdType *demuxchgainCfg;
  demuxchgainCfg = &(demux_module->applied_cmd);
  gain->greenEvenRow = demuxchgainCfg->ch0EvenGain;
  gain->greenOddRow = demuxchgainCfg->ch0OddGain;
  gain->blue = demuxchgainCfg->ch1Gain;
  gain->red = demuxchgainCfg->ch2Gain;
}

/** demux40_fill_vfe_diag_data:
 *
 *  @mod: demux module instance
 *
 *  This function fills vfe diagnostics information
 *
 *  Return: TRUE success
 **/
static boolean demux40_fill_vfe_diag_data(demux40_t *mod,
  isp_sub_module_t *isp_sub_module, isp_sub_module_output_t *sub_module_output)
{
  boolean              ret = TRUE;
  demuxchannelgain_t  *gain= NULL;
  vfe_diagnostics_t   *vfe_diag = NULL;

  if (sub_module_output->frame_meta) {
    sub_module_output->frame_meta->vfe_diag_enable =
      isp_sub_module->vfe_diag_enable;
    vfe_diag = &sub_module_output->frame_meta->vfe_diag;
    gain = &vfe_diag->prev_demuxchannelgain;
    demux40_ez_isp_update(mod, gain);
  }
  return ret;
}

/** demux40_fetch_demux_gain:
 *
 *  @module: mct module
 *  @event: mct event
 *  @isp_sub_module: isp_sub_module_t
 *  @isp_sub_module:
 *  This function Fills the demux gain
 *  Return: TRUE success
 */
boolean demux40_fetch_demux_gain(mct_module_t *module,
  isp_sub_module_t *isp_sub_module, mct_event_t *event)
{
  demux40_t                              *demux = NULL;
  chromatix_channel_balance_gains_type   *gain = NULL;
  isp_private_event_t                    *private_event = NULL;

  if (!module || !isp_sub_module || !event) {
    ISP_ERR("failed: %p %p %p", module, isp_sub_module, event);
    return FALSE;
  }

  PTHREAD_MUTEX_LOCK(&isp_sub_module->mutex);
  if (!isp_sub_module->private_data ||
      !event->u.module_event.module_event_data) {
    ISP_ERR("failed: mod %p module event data %p", isp_sub_module->private_data,
      event->u.module_event.module_event_data);
    PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
    return FALSE;
  }

  demux = (demux40_t *)isp_sub_module->private_data;
  private_event =
    (isp_private_event_t *)event->u.module_event.module_event_data;

  if (private_event->data_size != sizeof(chromatix_channel_balance_gains_type)) {
    /* size mismatch */
    ISP_ERR("size mismatch, expecting = %zu, received = %u",
      sizeof(chromatix_channel_balance_gains_type), private_event->data_size);
    PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
    return FALSE;
  }
  gain = (chromatix_channel_balance_gains_type *)private_event->data;
  if (!gain) {
    ISP_ERR("failed: gain %p", gain);
    PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
    return FALSE;
  }
  *gain = demux->gain;
  PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);

  return TRUE;
}

/** demux40_trigger_update:
 *    @demux_mod: demux module instance
 *    @trigger_params: module trigger update params
 *    @in_param_size: enable parameter size
 *
 *  This function runs in ISP HW thread context.
 *
 *  This function checks and initiates triger update of module
 *
 *  Return:   0 - Success
 *           -1 - Parameters size mismatch
 **/
boolean demux40_trigger_update(mct_module_t *module,
  isp_sub_module_t *isp_sub_module, mct_event_t *event)
{
  boolean                               ret = TRUE;
  isp_private_event_t                  *private_event = NULL;
  isp_sub_module_priv_t                *isp_sub_module_priv = NULL;
  float                                 max_gain_final = 0.0;
  float                                 cc_global_gain = 0.0;
  float                                 gain = 0.0;
  float                                 new_dig_gain = 1.0;
  float                                 max_ch_gain = 0.0;
  demux40_t                            *demux = NULL;
  ISP_DemuxConfigCmdType               *p_cmd = NULL;
  isp_sub_module_output_t              *output = NULL;
  chromatix_parms_type                 *chromatix_ptr = NULL;
  chromatix_channel_balance_gains_type *chromatix_channel_balance_gains = NULL;
  demux_info_t                         *demux_info = NULL;

  if (!module || !isp_sub_module || !event) {
    ISP_ERR("failed: %p %p %p", module, isp_sub_module, event);
    return FALSE;
  }

  isp_sub_module_priv = (isp_sub_module_priv_t *)MCT_OBJECT_PRIVATE(module);
  if (!isp_sub_module_priv) {
    ISP_ERR("failed: isp_sub_module_priv %p", isp_sub_module_priv);
    return FALSE;
  }

  demux = (demux40_t *)isp_sub_module->private_data;
  if (!demux) {
    ISP_ERR("failed: mod %p", demux);
    return FALSE;
  }
  p_cmd = &demux->ISP_DemuxConfigCmd;

  private_event =
    (isp_private_event_t *)event->u.module_event.module_event_data;
  if (!private_event) {
    ISP_ERR("failed: private_event %p", private_event);
    return FALSE;
  }

  output = (isp_sub_module_output_t *)private_event->data;
  if (!output) {
    ISP_ERR("failed: output %p", output);
    return FALSE;
  }

  chromatix_ptr = isp_sub_module->chromatix_ptrs.chromatixPtr;
  if (!chromatix_ptr) {
    ISP_ERR("failed: chromatix_ptr %p", chromatix_ptr);
    return FALSE;
  }

  if (!demux->ext_func_table || !demux->ext_func_table->get_global_gain) {
    ISP_ERR("failed: %p", demux->ext_func_table);
    return FALSE;
  }

  PTHREAD_MUTEX_LOCK(&isp_sub_module->mutex);

  if ((isp_sub_module->submod_enable == FALSE) ||
      (isp_sub_module->submod_trigger_enable == FALSE)) {
    ISP_DBG("No trigger update for demux: enable = %d, trigger_en = %d",
      isp_sub_module->submod_enable, isp_sub_module->submod_trigger_enable);

    ret = TRUE;
    goto FILL_METADATA;
  }

  /* set to unitity all frame before calculation*/
  demux->remaining_digital_gain = 1.0;
  chromatix_channel_balance_gains =
    &chromatix_ptr->chromatix_VFE.chromatix_channel_balance_gains;
  cc_global_gain = demux->ext_func_table->get_global_gain(
    demux, isp_sub_module, output);

  if (isp_sub_module->trigger_update_pending == TRUE) {
    max_ch_gain = MAX(chromatix_channel_balance_gains->green_odd,
      MAX(chromatix_channel_balance_gains->green_even,
      MAX(chromatix_channel_balance_gains->red,
      chromatix_channel_balance_gains->blue)));

    max_gain_final = (max_ch_gain * demux->dig_gain * cc_global_gain);

    if (max_gain_final > MAX_DEMUX_GAIN) {
      demux->remaining_digital_gain = max_gain_final / MAX_DEMUX_GAIN;
      max_gain_final = MAX_DEMUX_GAIN;
    }

    if (cc_global_gain && max_ch_gain) {
      new_dig_gain = (max_gain_final / (cc_global_gain * max_ch_gain));
    } else {
      ISP_ERR("invalid gain value %f %f, devide by 0!",
        cc_global_gain, max_ch_gain);
      goto FILL_METADATA;
    }

    demux->dig_gain = new_dig_gain;

    gain = cc_global_gain * demux->dig_gain;
    demux->gain.green_odd =
      gain * chromatix_channel_balance_gains->green_odd;
    demux->gain.green_even =
      gain * chromatix_channel_balance_gains->green_even;
    demux->gain.red =
      gain * chromatix_channel_balance_gains->red;
    demux->gain.blue =
      gain * chromatix_channel_balance_gains->blue;

    p_cmd->ch0EvenGain = DEMUX_GAIN(demux->gain.green_even);
    p_cmd->ch0OddGain = DEMUX_GAIN(demux->gain.green_odd);
    p_cmd->ch1Gain = DEMUX_GAIN(demux->gain.blue);
    p_cmd->ch2Gain = DEMUX_GAIN(demux->gain.red);

    ISP_DBG("max_ch_gain %5.3f glob %5.3f max_gain_final %5.3f",
      max_ch_gain, cc_global_gain, max_gain_final);
    ISP_DBG("gain: gr_odd = %5.2f, gr_even = %5.2f, red = %5.2f blue = %5.2f ",
      demux->gain.green_odd, demux->gain.green_even, demux->gain.red,
      demux->gain.blue);

    /* 3D is not supported. */
    if (demux->is_3d)
      demux40_r_image_gain_update(demux);
    ret = demux40_store_hw_update(isp_sub_module, demux);
    if (ret == FALSE) {
      ISP_ERR("failed: demux40_store_hw_update");
    }
  }

  ret = isp_sub_module_util_append_hw_update_list(isp_sub_module, output);
  if (ret == FALSE) {
    ISP_ERR("failed: isp_sub_module_util_append_hw_update_list");
  }

FILL_METADATA:
  demux_info = &(output->stats_params->be_info.demux_info);
  demux_info->gain_g_odd = demux->gain.green_odd;
  demux_info->gain_g_even = demux->gain.green_even;
  demux_info->gain_b = demux->gain.blue;
  demux_info->gain_r = demux->gain.red;
  demux_info->sensor_format = demux->fmt;

  if (output && isp_sub_module->vfe_diag_enable) {
    demux40_fill_vfe_diag_data(demux, isp_sub_module, output);
  }
  PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);

  return ret;
}/*demux40_trigger_update*/

/** demux40_config:
 *    @demux: demux module instance
 *    @in_params: configuration parameters
 *    @size: configuration parameters size
 *
 *  This function runs in ISP HW thread context.
 *
 *  This function makes initial configuration of demux module
 *
 *  Return:   0 - Success
 *           -1 - Parameters size mismatch
 **/
static boolean demux40_config(isp_sub_module_t *isp_sub_module,
  demux40_t *demux, chromatix_parms_type *chromatix_ptr)
{
  boolean                         ret = TRUE;
  boolean                         is_bayer_sensor = TRUE;
  ISP_DemuxConfigCmdType         *p_cmd  = &demux->ISP_DemuxConfigCmd;
  float                           color_correct_global_gain = 0.0;

  if (!isp_sub_module || !demux || !chromatix_ptr) {
    ISP_ERR("failed: %p %p %p", isp_sub_module, demux, chromatix_ptr);
    return FALSE;
  }

  color_correct_global_gain =
    chromatix_ptr->exposure_data.color_correction_global_gain;

  is_bayer_sensor = isp_sub_module_isp_bayer_fmt(demux->fmt);

  if (is_bayer_sensor) {
    INIT_GAIN(demux->gain, color_correct_global_gain);
  } else {
    INIT_GAIN(demux->gain, 1.0);
  }

  ret = demux40_set_cfg_params(p_cmd, demux->fmt);
  if (ret == FALSE) {
    ISP_ERR("failed: to configure demux");
    return ret;
  }

  p_cmd->ch0EvenGain = DEMUX_GAIN(demux->gain.green_even);
  p_cmd->ch0OddGain = DEMUX_GAIN(demux->gain.green_odd);
  p_cmd->ch1Gain = DEMUX_GAIN(demux->gain.blue);
  p_cmd->ch2Gain = DEMUX_GAIN(demux->gain.red);

  /* 3D is not supported. */
  if (demux->is_3d) {
    demux40_r_image_gain_update(demux);
  }
  isp_sub_module->trigger_update_pending = TRUE;

  ISP_DBG("sensor input format : %d ", demux->fmt);
  ISP_DBG("gain: gr_odd = %5.2f, gr_even = %5.2f, red = %5.2f blue = %5.2f ",
    demux->gain.green_odd, demux->gain.green_even, demux->gain.red,
    demux->gain.blue);

  return ret;
} /* demux40_config */

/** demux40_set_stream_config:
 *
 *  @module: mct module handle
 *  @isp_sub_module: isp sub module handle
 *  @event: mct event handle
 *
 *  Store sensor fmt in module private
 *
 *  Return: TRUE on success and FALSE on failure
 **/
boolean demux40_set_stream_config(mct_module_t *module,
  isp_sub_module_t *isp_sub_module, mct_event_t *event)
{
  sensor_out_info_t *sensor_out_info = NULL;
  demux40_t         *demux = NULL;

  if (!module || !isp_sub_module || !event) {
    ISP_ERR("failed: %p %p %p", module, isp_sub_module, event);
    return FALSE;
  }

  sensor_out_info =
    (sensor_out_info_t *)event->u.module_event.module_event_data;
  if (!sensor_out_info) {
    ISP_ERR("failed: sensor_out_info %p", sensor_out_info);
    return FALSE;
  }

  demux = (demux40_t *)isp_sub_module->private_data;
  if (!demux) {
    ISP_ERR("failed: demux %p", demux);
    return FALSE;
  }

  PTHREAD_MUTEX_LOCK(&isp_sub_module->mutex);

  demux->fmt = sensor_out_info->fmt;

  PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);

  return TRUE;
} /* demux40_set_stream_config */

/** demux_reset:
 *  @module: mct module handle
 *  @isp_sub_module: isp sub module handle
 *  @event: mct event handle
 *
 *  This function runs in ISP HW thread context.
 *
 *  This function resets demux module
 *
 *  Return: None
 **/
boolean demux40_reset(mct_module_t *module,
  isp_sub_module_t *isp_sub_module, mct_event_t *event)
{
  demux40_t     *demux = NULL;
  if (!module || !event || !isp_sub_module) {
    ISP_ERR("failed: %p %p %p", module, event, isp_sub_module);
    return FALSE;
  }

  demux = (demux40_t *)isp_sub_module->private_data;
  if (!demux) {
    ISP_ERR("failed: mod %p", demux);
    return FALSE;
  }

  memset(&demux->ISP_RImageGainConfigCmd, 0,
    sizeof(demux->ISP_RImageGainConfigCmd));
  memset(&demux->ISP_DemuxConfigCmd, 0, sizeof(demux->ISP_DemuxConfigCmd));
  memset(&demux->gain, 0, sizeof(demux->gain));
  memset(&demux->r_gain, 0, sizeof(demux->r_gain));
  demux->trigger_enable = 0;
  demux->dig_gain = 1.0;
  demux->is_3d =0;
  demux->remaining_digital_gain = 0.0;
  return TRUE;
} /* demux_reset */

/** demux40_streamon:
 *
 *  @module: mct module handle
 *  @isp_sub_module: isp sub module handle
 *  @event: mct event handle
 *
 *  This function makes initial configuration during first
 *  stream ON
 *
 *  Return: TRUE on success and FALSE on failure
 **/
boolean demux40_streamon(mct_module_t *module,
  isp_sub_module_t *isp_sub_module, mct_event_t *event)
{
  boolean                ret = TRUE;
  demux40_t                *mod = NULL;
  isp_sub_module_priv_t *isp_sub_module_priv = NULL;

  if (!module || !isp_sub_module || !event) {
    ISP_ERR("failed: %p %p %p", module, isp_sub_module, event);
    return FALSE;
  }

  isp_sub_module_priv = (isp_sub_module_priv_t *)MCT_OBJECT_PRIVATE(module);
  if (!isp_sub_module_priv) {
    ISP_ERR("failed: isp_sub_module_priv %p", isp_sub_module_priv);
    return FALSE;
  }

  mod = (demux40_t *)isp_sub_module->private_data;
  if (!mod) {
    ISP_ERR("failed: mod %p", mod);
    return FALSE;
  }

  PTHREAD_MUTEX_LOCK(&isp_sub_module->mutex);
  ISP_DBG("%s: Demux stream_on_count %d", __func__, isp_sub_module->stream_on_count);
  if (isp_sub_module->stream_on_count++) {
    PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
    return TRUE;
  }

  if (isp_sub_module->submod_enable == FALSE) {
    ISP_DBG("demux enable = %d", isp_sub_module->submod_enable);
    PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
    return TRUE;
  }

  ISP_DBG("%s: config demux\n", __func__);
  ret = demux40_config(isp_sub_module, mod,
    isp_sub_module->chromatix_ptrs.chromatixPtr);
  if (ret == FALSE) {
    ISP_ERR("failed: demux40_config ret %d", ret);
    goto ERROR;
  }

  isp_sub_module->trigger_update_pending = TRUE;

  PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);

  return ret;

ERROR:
  PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
  return ret;
} /* demux40_config */

/** demux40_streamoff:
 *
 *  @module: mct module handle
 *  @isp_sub_module: isp sub module handle
 *  @event: mct event handle
 *
 *  This function resets configuration during last stream OFF
 *
 *  Return: TRUE on success and FALSE on failure
 **/
boolean demux40_streamoff(mct_module_t *module,
  isp_sub_module_t *isp_sub_module, mct_event_t *event)
{
  demux40_t *mod = NULL;
  if (!module || !isp_sub_module || !event) {
    ISP_ERR("failed: %p %p %p", module, isp_sub_module, event);
    return FALSE;
  }

  mod = (demux40_t *)isp_sub_module->private_data;
  if (!mod) {
    ISP_ERR("failed: mod %p", mod);
    return FALSE;
  }

  PTHREAD_MUTEX_LOCK(&isp_sub_module->mutex);

  ISP_DBG("%s: Demux stream_on_count %d", __func__,
           isp_sub_module->stream_on_count);

  if (isp_sub_module->stream_on_count == 0 ||
      --isp_sub_module->stream_on_count > 0 ) {
    PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);
    return TRUE;
  }

  ISP_DBG("%s: reset demux\n", __func__);

  /* reset stream on count */
  isp_sub_module->stream_on_count = 0;

  demux40_reset(module, isp_sub_module, event);
  isp_sub_module->trigger_update_pending = FALSE;
  FILL_FUNC_TABLE(mod); /* Keep func table */
  PTHREAD_MUTEX_UNLOCK(&isp_sub_module->mutex);

  return TRUE;
} /* demux40_streamoff */

/** demux_init:
 *
 *  @module: mct module handle
 *  @isp_sub_module: isp sub module handle
 *
 *  Initialize the demux module
 *
 *  Return TRUE on Success, FALSE on failure
 **/
boolean demux40_init(mct_module_t *module, isp_sub_module_t *isp_sub_module)
{
  demux40_t *demux = NULL;

  if (!module || !isp_sub_module) {
    ISP_ERR("failed: module %p isp_sub_module %p", module, isp_sub_module);
    return FALSE;
  }

  demux = (demux40_t *)malloc(sizeof(demux40_t));
  if (!demux) {
    ISP_ERR("failed: demux %p", demux);
    return FALSE;
  }

  memset(demux, 0, sizeof(*demux));
  demux->fmt = CAM_FORMAT_MAX;
  demux->dig_gain = 1.0;
  demux->remaining_digital_gain = 1.0;
  INIT_GAIN(demux->r_gain, 1.0);

  isp_sub_module->private_data = (void *)demux;
  FILL_FUNC_TABLE(demux);
  return TRUE;
}/* demux_init */

/** demux40_destroy:
 *
 *  @module: mct module handle
 *  @isp_sub_module: isp sub module handle
 *
 *  Destroy dynamic resources
 *
 *  Return none
 **/
void demux40_destroy(mct_module_t *module, isp_sub_module_t *isp_sub_module)
{
  if (!module || !isp_sub_module) {
    ISP_ERR("failed: module %p isp_sub_module %p", module, isp_sub_module);
    return;
  }

  free(isp_sub_module->private_data);
  return;
} /* demux40_destroy */

#if !OVERRIDE_FUNC
/** demux40_get_global_gain:
 *
 *  @isp_sub_module: isp sub module handle
 *
 *  global gain changes
 *
 *  Return none
 **/
float demux40_get_global_gain(void *data1,
  void *data2, void *data3)
{
  demux40_t               *demux = NULL;
  isp_sub_module_t        *isp_sub_module = NULL;
  float                    global_gain = 0.0f;
  chromatix_parms_type    *chromatix_ptr = NULL;
  boolean                  ret = TRUE;
  AEC_algo_struct_type    *AEC_algo_data = NULL;
  isp_sub_module_output_t *output = NULL;

  if (!data1|| !data2 || !data3) {
    ISP_ERR("failed: %p %p %p", data1, data2, data3);
    return FALSE;
  }
  demux = (demux40_t *)data1;
  isp_sub_module = (isp_sub_module_t *)data2;
  output = (isp_sub_module_output_t *)data3;

  chromatix_ptr = isp_sub_module->chromatix_ptrs.chromatixPtr;
  if (!chromatix_ptr) {
    ISP_ERR("failed: chromatix_ptr %p", chromatix_ptr);
    return FALSE;
  }

 /*didnt see color correct gain in 306*/
  global_gain =
    chromatix_ptr->exposure_data.color_correction_global_gain;

  return global_gain;
}
static ext_override_func demux_override_func = {
  .get_global_gain         = demux40_get_global_gain,
};

boolean demux40_fill_func_table(demux40_t *demux)
{
  demux->ext_func_table = &demux_override_func;
  return TRUE;
}
#endif
