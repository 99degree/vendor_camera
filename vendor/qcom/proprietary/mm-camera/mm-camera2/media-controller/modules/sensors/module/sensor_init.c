/* sensor_init.c
 *
 * Copyright (c) 2013-2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "sensor_init.h"
#include "sensor_util.h"
#include "eebin_interface.h"
#include "server_debug.h"
#include <libxml/parser.h>
#include "sensor_util_xml.h"

#define MAX_CAMERA_CONFIG 20

#define MAX_SENSOR_LIB_NAME    128
#define MAX_SENSOR_SLOT_NUMBER 4

#define CSIDTG_CONFIG_XML "csidtg_camera.xml"
#if defined(MSM8952_SENSORS)
#define CONFIG_XML "msm8952_camera.xml"
#elif defined(MSM8976_SENSORS)
#define CONFIG_XML "msm8976_camera.xml"
#elif defined(MSM8996_SENSORS)
#define CONFIG_XML "msm8996_camera.xml"
#elif defined(MSM8937_SENSORS)
#define CONFIG_XML "msm8937_camera.xml"
#elif defined(TITANIUM_SENSORS)
#define CONFIG_XML "msm8953_camera.xml"
#else
#define CONFIG_XML "msm8996_camera.xml"
#endif

/** sensor_create_sbundle: create sensor bundle
 *
 *  @module_ctrl: sensor ctrl pointer
 *  @sensor_name: sensor name
 *  @session_id: sensor's session id from kernel
 *  @from_external : indicate where library exists
 *
 *  This function creates sensor bundle structure per session
 *  and initializes it
 **/
static void sensor_create_sbundle(module_sensor_ctrl_t *module_ctrl,
  struct msm_sensor_info_t *probed_info, const char *entity_name,
  camera_module_config_t *camera_cfg, sensor_lib_params_t *sensor_lib_params)
{
  uint32_t                     i = 0;
  int32_t                      rc = 0;
  module_sensor_bundle_info_t *sensor_bundle = NULL;
  pthread_mutex_t             *lens_move_done_sig;

  /* Validate input parameters */
  if (!module_ctrl || !probed_info) {
    SERR("failed: invalid params %p %p", module_ctrl, probed_info);
    return;
  }

  sensor_bundle = malloc(sizeof(module_sensor_bundle_info_t));
  if (!sensor_bundle) {
    SERR("failed");
    return;
  }
  memset(sensor_bundle, 0, sizeof(module_sensor_bundle_info_t));

  /* Copy the camera config read from xml */
  sensor_bundle->sensor_common_info.camera_config = *camera_cfg;

  sensor_bundle->sensor_info = malloc(sizeof(struct msm_sensor_info_t));
  if (!sensor_bundle->sensor_info) {
    free(sensor_bundle);
    return;
  }
  memset(sensor_bundle->sensor_info, 0, sizeof(struct msm_sensor_info_t));

  /* Fill sensor info structure in sensor bundle */
  memcpy(sensor_bundle->sensor_info, probed_info,
    sizeof(struct msm_sensor_info_t));

  SLOW("sensor name %s session %d",
    sensor_bundle->sensor_info->sensor_name,
    sensor_bundle->sensor_info->session_id);

  /* Initialize chroamtix subdevice id */
  sensor_bundle->sensor_info->subdev_id[SUB_MODULE_SENSOR] =
    (int32_t)sensor_bundle->sensor_info->session_id;
  sensor_bundle->sensor_info->subdev_id[SUB_MODULE_CHROMATIX] = 0;
  for (i = 0; i < SUB_MODULE_MAX; i++) {
    SLOW("subdev_id[%d] %d", i,
      sensor_bundle->sensor_info->subdev_id[i]);
  }

  /* Copy sensor subdev name to open and use during camera session */
  memcpy(sensor_bundle->subdev_info[SUB_MODULE_SENSOR].
    intf_info[SUBDEV_INTF_PRIMARY].sensor_sd_name, entity_name,
    MAX_SUBDEV_SIZE);

  lens_move_done_sig =
    &sensor_bundle->af_bracket_params.lens_move_done_sig;
  if (pthread_mutex_init(lens_move_done_sig, NULL)) {
    SERR("failed to init mutex errno %d", errno);
    free(sensor_bundle->sensor_info);
    free(sensor_bundle);
    return;
  }

  sensor_bundle->sensor_lib_params = sensor_lib_params;
  sensor_bundle->sensor_common_info.sensor_lib_params = sensor_lib_params;
  SLOW("sensor sd name %s",
    sensor_bundle->subdev_info[SUB_MODULE_SENSOR].
    intf_info[SUBDEV_INTF_PRIMARY].sensor_sd_name);

  /* Add sensor_bundle to module_ctrl list */
  module_ctrl->sensor_bundle =
    mct_list_append(module_ctrl->sensor_bundle, sensor_bundle, NULL,
    NULL);

  /* Increment sensor bundle size */
  module_ctrl->size++;

  return;
}

/** sensor_probe: probe available sensors
 *
 *  @fd: sensor_init fd
 *  @sensor_name: sensor name
 *
 *  Return: TRUE for success and FALSE for failure
 *
 *  1) Open sensor library
 *
 *  2) Pass slave information, probe sensor
 *
 *  3) If probe succeeds, create video node and sensor subdev is
 *  created in kernel
 *
 **/
static boolean sensor_probe(module_sensor_ctrl_t *module_ctrl, int32_t fd,
  const char *sensor_name, char *path, struct xmlCameraConfigInfo *xmlConfig)
{
  boolean                              ret = TRUE;
  int32_t                              rc = 0, i;
  sensor_lib_params_t                  *sensor_lib_params;
  struct sensor_init_cfg_data          cfg;
  struct msm_camera_sensor_slave_info *slave_info = NULL;
  struct msm_sensor_power_setting *power_up_setting = NULL;
  struct msm_sensor_power_setting *power_down_setting = NULL;
  struct camera_power_setting_array *power_setting_array;

  /* Validate input parameters */
  if ((fd < 0) || !sensor_name) {
    SERR("failed: invalid params fd %d sensor_name %s", fd, sensor_name);
    return FALSE;
  }

  sensor_lib_params = calloc(1, sizeof(sensor_lib_params_t));
  RETURN_ON_NULL(sensor_lib_params);

  /* Load sensor library */
  rc = sensor_load_library(sensor_name, sensor_lib_params, path);
  if (rc < 0) {
    SERR("sensor_load_library failed (non-fatal). Trying to load binary");
    ret = sensor_load_binary(sensor_name, sensor_lib_params, path);
    if (ret == FALSE) {
      SERR("sensor_load_binary failed");
      free(sensor_lib_params);
      return FALSE;
    }
  }

  power_setting_array = &sensor_lib_params->sensor_lib_ptr->sensor_slave_info.
    power_setting_array;

  slave_info = (struct msm_camera_sensor_slave_info *)malloc(
    sizeof(*slave_info));
  RETURN_ON_NULL(slave_info);

  power_up_setting = (struct msm_sensor_power_setting *)malloc(
    sizeof(*power_up_setting) * power_setting_array->size);

  if(power_up_setting == NULL) {
    SERR("failed: power_up_setting is NULL for %s",sensor_name);
    ret = FALSE;
    goto ERROR;
  }
  power_down_setting = (struct msm_sensor_power_setting *)malloc(
    sizeof(*power_down_setting) * power_setting_array->size_down);
  if(power_down_setting == NULL) {
    SERR("failed: power_down_setting is NULL for %s",sensor_name );
    ret = FALSE;
    goto ERROR;
  }
  translate_sensor_slave_info(slave_info,
    &sensor_lib_params->sensor_lib_ptr->sensor_slave_info,
    xmlConfig->configPtr, power_up_setting, power_down_setting);

  /* Update the output format in slave info */
  if (SENSOR_BAYER == sensor_lib_params->sensor_lib_ptr->
    sensor_output.output_format)
    slave_info->output_format = MSM_SENSOR_BAYER;
  else
    slave_info->output_format = MSM_SENSOR_YCBCR;

  /* Pass slave information to kernel and probe */
  memset(&cfg, 0, sizeof(cfg));
  cfg.cfgtype = CFG_SINIT_PROBE;
  cfg.cfg.setting = slave_info;
  if (ioctl(fd, VIDIOC_MSM_SENSOR_INIT_CFG, &cfg) < 0) {
    SERR("[%s]CFG_SINIT_PROBE failed",sensor_name);
    ret = FALSE;
    goto ERROR;
  }

  if (cfg.probed_info.session_id == 0) {
    SERR("[%s] probe failed.", sensor_name);
    ret = FALSE;
    goto ERROR;
  }

  SHIGH("[%s] probe succeeded: session_id(%d) entity_name(%s)",
    sensor_name, cfg.probed_info.session_id, cfg.entity_name);

  ret = sensor_xml_util_get_camera_full_config(xmlConfig);
  if (ret == FALSE) {
    SERR("sensor_xml_util_get_camera_full_config failed");
    goto ERROR;
  }

  /* CSID core is taken from xml config file */
  cfg.probed_info.subdev_id[SUB_MODULE_CSID] =
    xmlConfig->configPtr->camera_csi_params.csid_core;

  if ((cfg.probed_info.subdev_id[SUB_MODULE_CSID] < 0) ||
    (cfg.probed_info.subdev_id[SUB_MODULE_CSID] > 3)) {
    SERR("Invalid CSID core %d", cfg.probed_info.subdev_id[SUB_MODULE_CSID]);
    ret = FALSE;
    goto ERROR;
  }

  sensor_create_sbundle(module_ctrl, &cfg.probed_info, cfg.entity_name,
    xmlConfig->configPtr, sensor_lib_params);

ERROR:
  /* Unload sensor library */
  if (!ret) {
    sensor_unload_library(sensor_lib_params);
    free(sensor_lib_params);
  }

  free(slave_info);
  free(power_up_setting);
  free(power_down_setting);

  return ret;
}

static boolean sensor_init_xml_probe(module_sensor_ctrl_t *module_ctrl,
  int32_t sd_fd)
{
  int32_t         rc = 0;
  boolean         ret = FALSE;
  uint32_t        i = 0;
  char            config_xml_name[BUFF_SIZE_255];
  xmlDocPtr       docPtr = NULL;
  xmlNodePtr      rootPtr = NULL;
  xmlNodePtr      nodePtr = NULL;
  uint32_t        num_cam_config = 0;
  uint8_t         slot_probed[MAX_SENSOR_SLOT_NUMBER] = {0};
  camera_module_config_t camera_cfg;
  struct xmlCameraConfigInfo xmlConfig;
  char prop[PROPERTY_VALUE_MAX];
  uint32_t csidtg_enable = 0;

  property_get("persist.camera.csidtg.enable", prop, "0");
  csidtg_enable = atoi(prop);

  /* Create the xml path from data partition */
  snprintf(config_xml_name, BUFF_SIZE_255, "%s%s",
    CONFIG_XML_PATH, CONFIG_XML);

  if (access(config_xml_name, R_OK)) {
    SHIGH(" read fail (non-fatal) %s. Trying from system partition",
      config_xml_name);

    if (csidtg_enable) {
      /* Create the CSIDTG xml path from system partition */
      snprintf(config_xml_name, BUFF_SIZE_255, "%s%s",
        CONFIG_XML_SYSTEM_PATH, CSIDTG_CONFIG_XML);
    } else {
      /* Create the xml path from system partition */
      snprintf(config_xml_name, BUFF_SIZE_255, "%s%s",
        CONFIG_XML_SYSTEM_PATH, CONFIG_XML);
    }

    if (access(config_xml_name, R_OK)) {
      SERR("Cannot read file from %s. read failed", config_xml_name);
      return FALSE;
    }
  }

  SHIGH("reading from file %s", config_xml_name);

  /* Get the Root pointer and Document pointer of XMl file */
  ret = sensor_xml_util_load_file(config_xml_name, &docPtr, &rootPtr,
    "CameraConfigurationRoot");
  if (ret == FALSE) {
    SERR(" sensor_xml_util_load_file failed");
    return FALSE;
  }

  /* Get number of camera module configurations */
  num_cam_config = sensor_xml_util_get_num_nodes(rootPtr, "CameraModuleConfig");
  SLOW("num_cam_config = %d", num_cam_config);
  if (!num_cam_config || num_cam_config > MAX_CAMERA_CONFIG) {
    SERR(" invalid num_cam_config = %d", num_cam_config);
    ret = FALSE;
    goto XML_PROBE_EXIT;
  }

  xmlConfig.docPtr = docPtr;
  xmlConfig.configPtr = &camera_cfg;

  for (i = 0; i < num_cam_config; i++) {
    nodePtr = sensor_xml_util_get_node(rootPtr, "CameraModuleConfig", i);
    RETURN_ON_NULL(nodePtr);

    xmlConfig.nodePtr = nodePtr;
    ret = sensor_xml_util_get_camera_probe_config(&xmlConfig);
    if (ret == FALSE) {
      ret = FALSE;
      goto XML_PROBE_EXIT;
    }

    if (slot_probed[camera_cfg.camera_id]) {
      SHIGH("slot %d already probed", camera_cfg.camera_id);
      continue;
    }

    rc = sensor_probe(module_ctrl, sd_fd, camera_cfg.sensor_name,
      NULL, &xmlConfig);
    if (rc == FALSE) {
      SERR("failed: to probe %s", camera_cfg.sensor_name);
    } else {
      slot_probed[camera_cfg.camera_id] = TRUE;
    }
  }

XML_PROBE_EXIT:
  sensor_xml_util_unload_file(docPtr);
  return ret;
}

static boolean sensor_init_eebin_probe(module_sensor_ctrl_t *module_ctrl,
                                       int32_t sd_fd)
{
  int32_t rc = 0;
  boolean ret = TRUE;
  eebin_ctl_t bin_ctl;
  uint32_t num_devs, i;

  SLOW("Enter");

  bin_ctl.cmd = EEPROM_BIN_GET_NUM_DEV;
  bin_ctl.ctl.q_num.type = EEPROM_BIN_LIB_SENSOR;
  bin_ctl.ctl.q_num.num_devs = 0;
  eebin_interface_control(module_ctrl->eebin_hdl, &bin_ctl);

  num_devs = bin_ctl.ctl.q_num.num_devs;

  SLOW("num_devs:%d", num_devs);

  if (!num_devs)
   return FALSE;

  for (i = 0; i < num_devs; i++ ) {
    bin_ctl.cmd = EEPROM_BIN_GET_DEV_DATA;
    bin_ctl.ctl.dev_data.type = EEPROM_BIN_LIB_SENSOR;
    bin_ctl.ctl.dev_data.num = i;
    rc = eebin_interface_control(module_ctrl->eebin_hdl, &bin_ctl);
    if (rc < 0)
      continue;

    ret = sensor_probe(module_ctrl, sd_fd, bin_ctl.ctl.dev_data.name,
      bin_ctl.ctl.dev_data.path, NULL);
    if (ret == FALSE) {
      SERR("failed: to load %s", bin_ctl.ctl.dev_data.name);
    }
  }

  SLOW("Exit");
  return TRUE;
}

/** sensor_init_probe: probe available sensors
 *
 *  @module_ctrl: sensor ctrl pointer
 *
 *  Return: 0 for success and negative error on failure
 *
 *  1) Find sensor_init subdev and it
 *  2) Open EEPROM subdev and check whether any sensor library
 *  is present in EEPROM
 *  3) Open sensor libraries present in dumped firware location
 *  4) Check library version of EEPROM and dumped firmware
 *  5) Load latest of both
 *  6) Pass slave information, power up and probe sensors
 *  7) If probe succeeds, create video node and sensor subdev
 *  8) Repeat step 2-8 for all sensor libraries present in
 *  EEPROM
 *  9) Repeat step 6-8 for all sensor libraries present in
 *  absolute path
 **/

boolean sensor_init_probe(module_sensor_ctrl_t *module_ctrl)
{
  int32_t                     rc = 0, dev_fd = 0, sd_fd = 0;
  uint32_t                    i = 0;
  struct media_device_info    mdev_info;
  int32_t                     num_media_devices = 0;
  char                        dev_name[32];
  char                        subdev_name[32];
  struct sensor_init_cfg_data cfg;
  boolean                     ret = TRUE;

  while (1) {
    uint32_t num_entities = 1;
    snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
    dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
    if (dev_fd >= MAX_FD_PER_PROCESS) {
      dump_list_of_daemon_fd();
      dev_fd = -1;
      break;
    }
    if (dev_fd < 0) {
      SLOW("Done enumerating media devices");
      break;
    }
    num_media_devices++;
    rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
    if (rc < 0) {
      SLOW("Done enumerating media devices");
      close(dev_fd);
      break;
    }

    if (strncmp(mdev_info.model, "msm_config", sizeof(mdev_info.model)) != 0) {
      close(dev_fd);
      continue;
    }

    while (1) {
      struct media_entity_desc entity;
      memset(&entity, 0, sizeof(entity));
      entity.id = num_entities++;
      SLOW("entity id %d", entity.id);
      rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
      if (rc < 0) {
        SLOW("Done enumerating media entities");
        rc = 0;
        break;
      }
      SLOW("entity name %s type %d group id %d",
        entity.name, entity.type, entity.group_id);
      if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV &&
          entity.group_id == MSM_CAMERA_SUBDEV_SENSOR_INIT) {
        snprintf(subdev_name, sizeof(dev_name), "/dev/%s", entity.name);
        break;
      }
    }
    close(dev_fd);
  }

  /* Open sensor_init subdev */
  sd_fd = open(subdev_name, O_RDWR);
  if (sd_fd >= MAX_FD_PER_PROCESS) {
    dump_list_of_daemon_fd();
    sd_fd = -1;
    return FALSE;
  }
  if (sd_fd < 0) {
    SHIGH("Open sensor_init subdev failed");
    return FALSE;
  }

  ret = sensor_init_eebin_probe(module_ctrl, sd_fd);
  if (ret == FALSE) {
    SERR("failed: to probe eeprom bin sensors (non-fatal)");
  }

  RETURN_ON_FALSE(sensor_init_xml_probe(module_ctrl, sd_fd));

  return TRUE;
}
