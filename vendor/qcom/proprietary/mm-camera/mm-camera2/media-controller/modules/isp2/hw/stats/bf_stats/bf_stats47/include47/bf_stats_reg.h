/* bf_stats47_reg.h
 *
 * Copyright (c) 2015 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __BF_STATS47_REG_H__
#define __BF_STATS47_REG_H__

#define BF_STATS_OFF_0 0x00000AF0
#define BF_STATS_LEN_0 6

#define BF_STATS_OFF_1 0x00000A00
#define BF_STATS_LEN_1 5

#define H_FIR_MARGIN 12
#define V_FIR_MARGIN 4
#define H_IIR_MARGIN 64
#define V_IIR_MARGIN 32

#define MIN_X_OFFSET (H_FIR_MARGIN + H_IIR_MARGIN)
#define MIN_Y_OFFSET (V_FIR_MARGIN + V_IIR_MARGIN)
#define MIN_WIDTH    6
#define MIN_HEIGHT   8
#define MAX_NUM_ROI  281
#define MAX_GRID_WIDTH_DIVIDER    2
#define MAX_GRID_HEIGHT_DIVIDER    2
#define MIN_H_NUM         1
#define MAX_H_NUM         21
#define MIN_V_NUM         1
#define MAX_V_NUM         281

#define DMI_X_BITS 0x1FFF
#define DMI_Y_BITS 0x1FFF
#define DMI_W_BITS 0xFFF
#define DMI_H_BITS 0x1FFF

#define DMI_I_SHIFT 63
#define DMI_X_SHIFT 46
#define DMI_Y_SHIFT 31
#define DMI_W_SHIFT 15
#define DMI_H_SHIFT 0

#define MIN_START_PIX_OVERLAP 6
#define MIN_END_PIX_OVERLAP   6


#define MAX_HORIZONTAL_REGIONS  21
#define MAX_VERTICAL_SPACING    8
#define MAX_OVERLAPPING_REGIONS 4

#define STATS_BAF_Y_CONV_CFG_SHIFT 11

#define NUM_Y_COEFFCIENTS 3

#define STATS_BAF_CFG_G_SEL_GR 0x0
#define STATS_BAF_CFG_G_SEL_GB 0x1

#define STATS_BAF_CFG_CH_SEL_G 0x0
#define STATS_BAF_CFG_CH_SEL_Y 0x1

#define FIR_BITS 6
#define IIR_BITS 16
#define SHIFTER_BITS 4
#define THRESHOLD_MASK 0x3FFFF
#define CORING_BITS  0x3FF

#define STATS_BAF_CFG_ADDR    0xAE4
#define STATS_BAF_Y_CONV_ADDR 0xAE8

#define STATS_BAF_IIR_SHIFT_BITS 14

#define BF_CH_SEL_SUPPORTED TRUE
#define BF_GAMMA_SUPPORTED       TRUE
//#define BF_GAMMA_DOWNSCALER_WA 1
#define BF_DOWN_SCALER_SUPPORTED TRUE

#define NUM_H1_FIR_A_KERNEL 13
#define NUM_H1_IIR_A_KERNEL 4
#define NUM_H1_IIR_B_KERNEL 6
#define H1_FIR_DISABLE_SUPPORTED TRUE
#define H1_IIR_DISABLE_SUPPORTED TRUE
#define H1_DOWNSCALE_SUPPORTED TRUE
#define H1_IS_CORING_VARIABLE TRUE

#define NUM_H2_FIR_A_KERNEL 13
#define NUM_H2_IIR_A_KERNEL 4
#define NUM_H2_IIR_B_KERNEL 6
#define H2_FIR_DISABLE_SUPPORTED TRUE
#define H2_IIR_DISABLE_SUPPORTED TRUE
#define H2_DOWNSCALE_SUPPORTED TRUE
#define H2_IS_CORING_VARIABLE TRUE

#define NUM_V_FIR_A_KERNEL 5
#define NUM_V_IIR_A_KERNEL 2
#define NUM_V_IIR_B_KERNEL 3
#define V_FIR_DISABLE_SUPPORTED TRUE
#define V_IIR_DISABLE_SUPPORTED TRUE
#define V_DOWNSCALE_SUPPORTED FALSE
#define V_IS_CORING_VARIABLE TRUE

#define BF_CGC_OVERRIDE TRUE
#define BF_CGC_OVERRIDE_REGISTER 0x30
#define BF_CGC_OVERRIDE_BIT 2

typedef struct bf_stats_enable_t {
  union {
    struct  __attribute__((packed, aligned(4))) {
      /* STATS_BAF_CFG */
      uint32_t g_sel                : 1;
      uint32_t gamma_lut_en         : 1;
      uint32_t gamma_lut_bank_sel   : 1;
      uint32_t ch_sel               : 1;
      uint32_t scale_en             : 1;
      uint32_t h_1_scale_sel        : 1;
      uint32_t h_2_scale_sel        : 1;
      uint32_t /* reserved */       : 1;
      uint32_t h_1_fir_en           : 1;
      uint32_t h_1_iir_en           : 1;
      uint32_t h_2_fir_en           : 1;
      uint32_t h_2_iir_en           : 1;
      uint32_t v_fir_en             : 1;
      uint32_t v_iir_en             : 1;
      uint32_t /* reserved */       : 2;
      uint32_t roi_ind_lut_bank_sel : 1;
      uint32_t /* reserved */       : 15;
    } fields;
    uint32_t value;
  };
} __attribute__((packed, aligned(4))) bf_stats_enable_t;

typedef struct bf_stats_y_conv_t {

  /* STATS_BAF_Y_CONF_CFG_0 */
  uint32_t a00                  : 12;
  uint32_t /* reserved */       : 4;
  uint32_t a01                  : 12;
  uint32_t /* reserved */       : 4;

  /* STATS_BAF_Y_CONF_CFG_1 */
  uint32_t a02                  : 12;
  uint32_t /* reserved */       : 20;

} __attribute__((packed, aligned(4))) bf_stats_y_conv_t;

typedef struct bf_stats_h_fir_cfg_t {

  /* STATS_BAF_H_FIR_CFG_0 */
  uint32_t a00                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a01                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a02                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a03                  : 6;
  uint32_t /* reserved */       : 2;

  /* STATS_BAF_H_FIR_CFG_1 */
  uint32_t a04                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a05                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a06                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a07                  : 6;
  uint32_t /* reserved */       : 2;

  /* STATS_BAF_H_FIR_CFG_2 */
  uint32_t a08                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a09                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a10                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a11                  : 6;
  uint32_t /* reserved */       : 2;

  /* STATS_BAF_H_FIR_CFG_3 */
  uint32_t a12                  : 6;
  uint32_t /* reserved */       : 26;

} __attribute__((packed, aligned(4))) bf_stats_h_fir_cfg_t;

typedef struct bf_stats_h_iir_cfg_t {

  /* STATS_BAF_H_IIR_CFG_0 */
  uint32_t b10                  : 16;
  uint32_t b11                  : 16;

  /* STATS_BAF_H_IIR_CFG_1 */
  uint32_t b12                  : 16;
  uint32_t b22                  : 16;

  /* STATS_BAF_H_IIR_CFG_2 */
  uint32_t a11                  : 16;
  uint32_t a12                  : 16;

  /* STATS_BAF_H_IIR_CFG_3 */
  uint32_t b20                  : 16;
  uint32_t b21                  : 16;

  /* STATS_BAF_H_IIR_CFG_4 */
  uint32_t a21                  : 16;
  uint32_t a22                  : 16;

} __attribute__((packed, aligned(4))) bf_stats_h_iir_cfg_t;

typedef struct bf_stats_v_fir_cfg_t {

  /* STATS_BAF_V_FIR_CFG_0 */
  uint32_t a00                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a01                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a02                  : 6;
  uint32_t /* reserved */       : 2;
  uint32_t a03                  : 6;
  uint32_t /* reserved */       : 2;

  /* STATS_BAF_V_FIR_CFG_1 */
  uint32_t a04                  : 6;
  uint32_t /* reserved */       : 26;

} __attribute__((packed, aligned(4))) bf_stats_v_fir_cfg_t;

typedef struct bf_stats_v_iir_cfg_t {

  /* STATS_BAF_V_IIR_CFG_0 */
  uint32_t b10                  : 16;
  uint32_t b11                  : 16;

  /* STATS_BAF_V_IIR_CFG_1 */
  uint32_t b12                  : 16;
  uint32_t /* reserved */       : 16;

  /* STATS_BAF_V_IIR_CFG_2 */
  uint32_t a11                  : 16;
  uint32_t a12                  : 16;

} __attribute__((packed, aligned(4))) bf_stats_v_iir_cfg_t;

typedef struct bf_stats_shift_bits_cfg_t {

  /* STATS_BAF_SHIFT_BITS_CFG */
  uint32_t h_1                  : 4;
  uint32_t /* reserved */       : 4;
  uint32_t h_2                  : 4;
  uint32_t /* reserved */       : 4;
  uint32_t v                    : 4;
  uint32_t /* reserved */       : 12;

} __attribute__((packed, aligned(4))) bf_stats_shift_bits_cfg_t;

typedef struct bf_stats_coring_cfg_t {

  /* STATS_BAF_TH_CFG */
  uint32_t threshold            : 17;
  uint32_t /* reserved */       : 15;

  /* STATS_BAF_CORING_CFG_0 */
  uint32_t ind_0                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_1                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_2                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_3                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_4                : 5;
  uint32_t /* reserved */       : 3;

  /* STATS_BAF_CORING_CFG_1 */
  uint32_t ind_5                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_6                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_7                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_8                : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_9                : 5;
  uint32_t /* reserved */       : 3;

  /* STATS_BAF_CORING_CFG_2 */
  uint32_t ind_10               : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_11               : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_12               : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_13               : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_14               : 5;
  uint32_t /* reserved */       : 3;

  /* STATS_BAF_CORING_CFG_3 */
  uint32_t ind_15               : 5;
  uint32_t /* reserved */       : 1;
  uint32_t ind_16               : 5;
  uint32_t /* reserved */       : 21;

} __attribute__((packed, aligned(4))) bf_stats_coring_cfg_t;

typedef struct bf_stats_coring_gain_cfg_t {

  /* STATS_BAF_CORING_GAIN_CFG_0 */
  uint32_t h_1_gain             : 10;
  uint32_t /* reserved */       : 6;
  uint32_t h_2_gain             : 10;
  uint32_t /* reserved */       : 6;

  /* STATS_BAF_CORING_GAIN_CFG_1 */
  uint32_t v_gain               : 10;
  uint32_t /* reserved */       : 22;

} __attribute__((packed, aligned(4))) bf_stats_coring_gain_cfg_t;

typedef struct _bf_stats_filter_cfg_t {

  bf_stats_h_fir_cfg_t       bf_stats_h_1_fir_cfg;
  bf_stats_h_iir_cfg_t       bf_stats_h_1_iir_cfg;
  bf_stats_h_fir_cfg_t       bf_stats_h_2_fir_cfg;
  bf_stats_h_iir_cfg_t       bf_stats_h_2_iir_cfg;
  bf_stats_v_fir_cfg_t       bf_stats_v_fir_cfg;
  bf_stats_v_iir_cfg_t       bf_stats_v_iir_cfg;
  bf_stats_shift_bits_cfg_t  bf_stats_shift_bits_cfg;
  bf_stats_coring_cfg_t      bf_stats_h_1_coring_cfg;
  bf_stats_coring_cfg_t      bf_stats_h_2_coring_cfg;
  bf_stats_coring_cfg_t      bf_stats_v_coring_cfg;
  bf_stats_coring_gain_cfg_t bf_stats_coring_gain_cfg;

} __attribute__((packed, aligned(4))) bf_stats_filter_cfg_t;

typedef struct bf_stats_reg_cfg_t {

  bf_stats_enable_t      bf_stats_enable_mask;
  bf_stats_enable_t      bf_stats_enable_val;
  bf_stats_y_conv_t      bf_stats_y_conv;
  bf_stats_filter_cfg_t  bf_stats_filter_cfg;

} __attribute__((packed, aligned(4))) bf_stats_reg_cfg_t;

#endif /* __BF_STATS47_REG_H__ */
