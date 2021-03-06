/*
 * Copyright (c) 2015 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include "sunny_q5v41b_eeprom.h"

/** sunny_q5v41b_get_calibration_items:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Get calibration capabilities and mode items.
 *
 * This function executes in eeprom module context
 *
 * Return: void.
 **/
void sunny_q5v41b_get_calibration_items(void *e_ctrl)
{
  sensor_eeprom_data_t *ectrl = (sensor_eeprom_data_t *)e_ctrl;
  eeprom_calib_items_t *e_items = &(ectrl->eeprom_data.items);
  e_items->is_insensor = TRUE;
  e_items->is_afc = FALSE;
  e_items->is_wbc = FALSE;
  e_items->is_lsc = FALSE;
  e_items->is_dpc = FALSE;

  SLOW("is_wbc:%d,is_afc:%d,is_lsc:%d,is_dpc:%d,is_insensor:%d,\
       is_ois:%d",e_items->is_wbc,e_items->is_afc,
       e_items->is_lsc,e_items->is_dpc,e_items->is_insensor,
       e_items->is_ois);

}

/** sunny_q5v41b_update_awb:
 *
 * Calculate and apply white balance calibration data
 *
 * This function executes in eeprom module context
 *
 * Return: void.
 **/
static void sunny_q5v41b_update_awb()
{
  uint16_t R_gain, G_gain, B_gain;
  uint16_t G_gain_R, G_gain_B ;
  uint16_t nR_G_gain, nB_G_gain, nG_G_gain;
  uint16_t nBase_gain;


  if(otp_data.light_rg) {
    otp_data.rg_ratio = otp_data.rg_ratio * (otp_data.light_rg +512) / 1024;
  }
  if(otp_data.light_bg){
    otp_data.bg_ratio = otp_data.bg_ratio * (otp_data.light_bg +512) / 1024;
  }
  SLOW("rg_ratio=0x%x,bg_ratio=0x%x,light_rg=0x%x,light_bg=0x%x",
    otp_data.rg_ratio,otp_data.bg_ratio,otp_data.light_rg,otp_data.light_bg) ;


  nR_G_gain = (RG_RATIO_TYPICAL_VALUE*1000) / otp_data.rg_ratio;
  nB_G_gain = (BG_RATIO_TYPICAL_VALUE*1000) / otp_data.bg_ratio;
  nG_G_gain = 1000;
  if (nR_G_gain < 1000 || nB_G_gain < 1000)
  {
    if (nR_G_gain < nB_G_gain)
      nBase_gain = nR_G_gain;
    else
      nBase_gain = nB_G_gain;
  }
  else
  {
    nBase_gain = nG_G_gain;
  }
  R_gain = 0x400 * nR_G_gain / (nBase_gain);
  B_gain = 0x400 * nB_G_gain / (nBase_gain);
  G_gain = 0x400 * nG_G_gain / (nBase_gain);

  SLOW("R_gain=0x%x,G_gain=0x%x,B_gain=0x%x",R_gain,G_gain,B_gain) ;

  if (R_gain > 0x400) {
    g_reg_array[g_reg_setting.size].reg_addr = 0x5032;
    g_reg_array[g_reg_setting.size].reg_data = R_gain >> 8;
    g_reg_setting.size++;
    g_reg_array[g_reg_setting.size].reg_addr = 0x5033;
    g_reg_array[g_reg_setting.size].reg_data = R_gain & 0x00ff;
    g_reg_setting.size++;
  }
  if (G_gain > 0x400) {
    g_reg_array[g_reg_setting.size].reg_addr = 0x5034;
    g_reg_array[g_reg_setting.size].reg_data = G_gain >> 8;
    g_reg_setting.size++;
    g_reg_array[g_reg_setting.size].reg_addr = 0x5035;
    g_reg_array[g_reg_setting.size].reg_data = G_gain & 0x00ff;
    g_reg_setting.size++;
  }
  if (B_gain > 0x400) {
    g_reg_array[g_reg_setting.size].reg_addr = 0x5036;
    g_reg_array[g_reg_setting.size].reg_data = B_gain >> 8;
    g_reg_setting.size++;
    g_reg_array[g_reg_setting.size].reg_addr = 0x5037;
    g_reg_array[g_reg_setting.size].reg_data = B_gain & 0x00ff;
    g_reg_setting.size++;
  }
}

/** sunny_q5v41b_get_group_index:
 *    @mid: group index register value
 *
 * Get which group is used
 *
 * This function executes in eeprom module context
 *
 * Return: int to indicate group index.
 **/
static int sunny_q5v41b_get_group_index(uint8_t mid)
{
  int8_t group_index = -1 ;
  mid = mid & 0xFC ;
  if((mid&0xC0) == 0x40){
    group_index = 0 ;
  }else if((mid&0x30) == 0x10){
    group_index = 1 ;
  }else if((mid&0x0C) == 0x04){
    group_index = 2 ;
  }else{
    group_index = -1 ;
  }
  return group_index ;
}

/** sunny_q5v41b_read_info:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Read the data structure of product information like product date
 *
 * This function executes in eeprom module context
 *
 * Return: int to indicate read information success or not.
 **/
static int sunny_q5v41b_read_info(sensor_eeprom_data_t *e_ctrl)
{
  uint8_t mid,temp,reg_val;
  int i = 0, group_index,addr_offset = 0,group_offset=5;

  SDBG("Enter");
  mid = (uint8_t)(e_ctrl->eeprom_params.buffer[0]);
  if((group_index=sunny_q5v41b_get_group_index(mid))==-1){
    SERR("%s:invalid or empty opt data",__func__) ;
    return -1 ;
  }
  addr_offset = group_offset*group_index ;

  otp_data.module_integrator_id =
    (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 1]);
  otp_data.lens_id =
    (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 2]);
  otp_data.production_year =
    (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 3]);
  otp_data.production_month =
    (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 4]);
  otp_data.production_day =
    (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 5]);

  return 0;
}

/** sunny_q5v41b_read_wbdata:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Read the data structure of white balance calibration
 *
 * This function executes in eeprom module context
 *
 * Return: int to indicate read white balance success or not.
 **/
static int sunny_q5v41b_read_wbdata(sensor_eeprom_data_t *e_ctrl)
{
  uint8_t mid ;
  uint8_t temp ;
  uint8_t reg_val  ;
  int addr_offset = -1 ;
  int group_index  = -1 ;
  int group_offset = 3 ;
  SDBG("Enter");
  mid = (uint8_t)(e_ctrl->eeprom_params.buffer[WB_OFFSET]);

  if((group_index=sunny_q5v41b_get_group_index(mid))==-1){
    SERR("%s:invalid or empty awb data",__func__) ;
    return -1 ;
  }

  addr_offset =WB_OFFSET+group_index*group_offset ;
  temp = (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 3]);
  SLOW("temp=0x%x",temp) ;

  reg_val = (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 1]);
  SLOW("reg_val=0x%x",reg_val) ;
  otp_data.rg_ratio = (reg_val<<2) + ((temp>>6)&0x03) ;

  reg_val = (uint8_t)(e_ctrl->eeprom_params.buffer[addr_offset + 2]);
  SLOW("reg_val=0x%x",reg_val) ;
  otp_data.bg_ratio = (reg_val<<2) + ((temp>>4)&0x03) ;

  return 0;
}

/** sunny_q5v41b_format_wbdata:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Format the data structure of white balance calibration
 *
 * This function executes in eeprom module context
 *
 * Return: void.
 **/
static void sunny_q5v41b_format_wbdata(sensor_eeprom_data_t *e_ctrl)
{
  SDBG("Enter");
  int rc = 0;

  rc = sunny_q5v41b_read_wbdata(e_ctrl);
  if(rc < 0)
    SERR("read wbdata failed");
  sunny_q5v41b_update_awb();

  SDBG("Exit");
}

/** sunny_q5v41b_format_calibration_data:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Format all the data structure of calibration
 *
 * This function executes in eeprom module context and generate
 *   all the calibration registers setting of the sensor.
 *
 * Return: void.
 **/
void sunny_q5v41b_format_calibration_data(void *e_ctrl) {
  SDBG("Enter");
  sensor_eeprom_data_t *ectrl = (sensor_eeprom_data_t *)e_ctrl;
  uint8_t *data = ectrl->eeprom_params.buffer;

  g_reg_setting.addr_type = CAMERA_I2C_WORD_ADDR;
  g_reg_setting.data_type = CAMERA_I2C_BYTE_DATA;
  g_reg_setting.reg_setting = &g_reg_array[0];
  g_reg_setting.size = 0;
  g_reg_setting.delay = 0;
  sunny_q5v41b_read_info(e_ctrl) ;
  sunny_q5v41b_format_wbdata(ectrl);
  SDBG("Exit");
}

/** sunny_q5v41b_get_raw_data:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *    @data: point to the destination msm_camera_i2c_reg_setting
 *
 * Get the all the calibration registers setting of the sensor
 *
 * This function executes in eeprom module context.
 *
 * Return: void.
 **/
int sunny_q5v41b_get_raw_data(void *e_ctrl, void *data)
{

  RETURN_ERR_ON_NULL(e_ctrl, SENSOR_FAILURE);
  RETURN_ERR_ON_NULL(data, SENSOR_FAILURE);

  memcpy(data, &g_reg_setting, sizeof(g_reg_setting));
  return SENSOR_SUCCESS;
}


/** sunny_q5v41b_eeprom_open_lib:
 *
 * Get the funtion pointer of this lib.
 *
 * This function executes in eeprom module context.
 *
 * Return: eeprom_lib_func_t point to the function pointer.
 **/
void* sunny_q5v41b_eeprom_open_lib(void) {
  return &sunny_q5v41b_lib_func_ptr;
}

