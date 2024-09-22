#pragma once

#include "bitstream.h"

class bitstream_h264 : public bitstream
{
public:
   struct sps {
      uint8_t profile_idc;
      uint8_t constraint_set_flags;
      uint8_t level_idc;
      uint8_t seq_parameter_set_id;
      uint8_t chroma_format_idc;
      uint8_t bit_depth_luma_minus8;
      uint8_t bit_depth_chroma_minus8;
      uint8_t log2_max_frame_num_minus4;
      uint8_t pic_order_cnt_type;
      uint8_t log2_max_pic_order_cnt_lsb_minus4;
      uint8_t max_num_ref_frames;
      uint8_t gaps_in_frame_num_value_allowed_flag : 1;
      uint32_t pic_width_in_mbs_minus1;
      uint32_t pic_height_in_map_units_minus1;
      uint8_t frame_mbs_only_flag : 1;
      uint8_t mb_adaptive_frame_field_flag : 1;
      uint8_t direct_8x8_inference_flag : 1;
      uint8_t frame_cropping_flag : 1;
      uint32_t frame_crop_left_offset;
      uint32_t frame_crop_right_offset;
      uint32_t frame_crop_top_offset;
      uint32_t frame_crop_bottom_offset;
      uint8_t vui_parameters_present_flag : 1;
      uint8_t aspect_ratio_info_present_flag : 1;
      uint8_t aspect_ratio_idc;
      uint16_t sar_width;
      uint16_t sar_height;
      uint8_t overscan_info_present_flag : 1;
      uint8_t overscan_appropriate_flag : 1;
      uint8_t video_signal_type_present_flag : 1;
      uint8_t video_format;
      uint8_t video_full_range_flag : 1;
      uint8_t colour_description_present_flag : 1;
      uint8_t colour_primaries;
      uint8_t transfer_characteristics;
      uint8_t matrix_coefficients;
      uint8_t chroma_loc_info_present_flag : 1;
      uint8_t chroma_sample_loc_type_top_field;
      uint8_t chroma_sample_loc_type_bottom_field;
      uint8_t timing_info_present_flag : 1;
      uint32_t num_units_in_tick;
      uint32_t time_scale;
      uint8_t fixed_frame_rate_flag : 1;
      uint8_t pic_struct_present_flag : 1;
      uint8_t bitstream_restriction_flag : 1;
      uint8_t motion_vectors_over_pic_boundaries_flag : 1;
      uint32_t max_bytes_per_pic_denom;
      uint32_t max_bits_per_mb_denom;
      uint8_t log2_max_mv_length_horizontal;
      uint8_t log2_max_mv_length_vertical;
      uint8_t max_num_reorder_frames;
      uint8_t max_dec_frame_buffering;
   };

   struct pps {
      uint8_t pic_parameter_set_id;
      uint8_t seq_parameter_set_id;
      uint8_t entropy_coding_mode_flag : 1;
      uint8_t num_ref_idx_l0_default_active_minus1;
      uint8_t num_ref_idx_l1_default_active_minus1;
      uint8_t weighted_pred_flag : 1;
      uint8_t weighted_bipred_idc : 1;
      int16_t pic_init_qp_minus26;
      int16_t pic_init_qs_minus26;
      int16_t chroma_qp_index_offset;
      uint8_t deblocking_filter_control_present_flag : 1;
      uint8_t constrained_intra_pred_flag : 1;
      uint8_t redundant_pic_cnt_present_flag : 1;
      uint8_t transform_8x8_mode_flag : 1;
      int16_t second_chroma_qp_index_offset;
   };

   struct ref_list_mod {
      uint8_t modification_of_pic_nums_idc;
      uint32_t abs_diff_pic_num_minus1;
      uint32_t long_term_pic_num;
   };

   struct mmco_op {
      uint8_t memory_management_control_operation;
      uint32_t difference_of_pic_nums_minus1;
      uint32_t long_term_pic_num;
      uint32_t long_term_frame_idx;
      uint32_t max_long_term_frame_idx_plus1;
   };

   struct slice {
      uint8_t nal_ref_idc;
      uint8_t nal_unit_type;
      uint32_t first_mb_in_slice;
      uint8_t slice_type;
      uint8_t pic_parameter_set_id;
      uint32_t frame_num;
      uint32_t idr_pic_id;
      uint32_t pic_order_cnt_lsb;
      uint32_t redundant_pic_cnt;
      uint8_t direct_spatial_mv_pred_flag : 1;
      uint8_t num_ref_idx_active_override_flag : 1;
      uint8_t num_ref_idx_l0_active_minus1;
      uint8_t num_ref_idx_l1_active_minus1;
      uint8_t ref_pic_list_modification_flag_l0 : 1;
      uint8_t ref_pic_list_modification_flag_l1 : 1;
      uint8_t num_ref_list0_mod;
      struct ref_list_mod ref_list0_mod[16];
      uint8_t num_ref_list1_mod;
      struct ref_list_mod ref_list1_mod[16];
      uint8_t no_output_of_prior_pics_flag : 1;
      uint8_t long_term_reference_flag : 1;
      uint8_t adaptive_ref_pic_marking_mode_flag : 1;
      uint8_t num_mmco_op;
      struct mmco_op mmco_op[16];
      uint8_t cabac_init_idc;
      int16_t slice_qp_delta;
      uint8_t disable_deblocking_filter_idc;
      int8_t slice_alpha_c0_offset_div2;
      int8_t slice_beta_offset_div2;
   };

   struct prefix {
      uint8_t svc_extension_flag : 1;
      uint8_t idr_flag : 1;
      uint8_t priority_id;
      uint8_t no_inter_layer_pred_flag : 1;
      uint8_t dependency_id;
      uint8_t quality_id;
      uint8_t temporal_id;
      uint8_t use_ref_base_pic_flag : 1;
      uint8_t discardable_flag : 1;
      uint8_t output_flag : 1;
   };

   struct sei_recovery_point {
      uint16_t recovery_frame_cnt;
      uint8_t exact_match_flag;
      uint8_t broken_link_flag;
      uint8_t changing_slice_group_idc;
   };

   struct sei_scalability_info {
      uint32_t num_layers_minus1;
   };

   bitstream_h264();

   void write_sps(const sps &sps);
   void write_pps(const pps &pps);
   void write_slice(const slice &slice, const sps &sps, const pps &pps);
   void write_prefix(const prefix &pfx);
   void write_sei_recovery_point(const sei_recovery_point &srp);
   void write_sei_scalability_info(const sei_scalability_info &ssi);

private:
   void write_nal_header(uint8_t nal_ref_idc, uint8_t nal_unit_type);
   void write_sei(uint32_t type, const bitstream &bs);
};
