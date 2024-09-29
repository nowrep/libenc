#pragma once

#include "bitstream.h"

class bitstream_hevc : public bitstream
{
public:
   struct profile_tier {
      uint8_t general_profile_space;
      uint8_t general_tier_flag : 1;
      uint8_t general_profile_idc;
      uint8_t general_profile_compatibility_flag[32];
      uint8_t general_progressive_source_flag : 1;
      uint8_t general_interlaced_source_flag : 1;
      uint8_t general_non_packed_constraint_flag : 1;
      uint8_t general_frame_only_constraint_flag : 1;
   };

   struct profile_tier_level {
      uint8_t general_level_idc;
      uint8_t sub_layer_profile_present_flag[7];
      uint8_t sub_layer_level_present_flag[7];
      uint8_t sub_layer_level_idc[7];
      struct profile_tier profile_tier;
      struct profile_tier sub_layer_profile_tier[7];
   };

   struct vps {
      uint8_t vps_video_parameter_set_id;
      uint8_t vps_base_layer_internal_flag : 1;
      uint8_t vps_base_layer_available_flag : 1;
      uint8_t vps_max_sub_layers_minus1;
      uint8_t vps_temporal_id_nesting_flag : 1;
      struct profile_tier_level profile_tier_level;
      uint8_t vps_sub_layer_ordering_info_present_flag : 1;
      uint8_t vps_max_dec_pic_buffering_minus1[7];
      uint8_t vps_max_num_reorder_pics[7];
      uint32_t vps_max_latency_increase_plus1[7];
      uint8_t vps_max_layer_id;
      uint8_t vps_timing_info_present_flag : 1;
      uint32_t vps_num_units_in_tick;
      uint32_t vps_time_scale;
      uint8_t vps_poc_proportional_to_timing_flag : 1;
      uint32_t vps_num_ticks_poc_diff_one_minus1;
   };

   struct st_ref_pic_set {
      uint8_t inter_ref_pic_set_prediction_flag : 1;
      uint8_t num_negative_pics;
      uint8_t num_positive_pics;
      uint16_t delta_poc_s0_minus1[16];
      uint8_t used_by_curr_pic_s0_flag[16];
      uint16_t delta_poc_s1_minus1[16];
      uint8_t used_by_curr_pic_s1_flag[16];
   };

   struct sps {
      uint8_t sps_video_parameter_set_id;
      uint8_t sps_max_sub_layers_minus1;
      uint8_t sps_temporal_id_nesting_flag : 1;
      struct profile_tier_level profile_tier_level;
      uint8_t sps_seq_parameter_set_id;
      uint8_t chroma_format_idc;
      uint32_t pic_width_in_luma_samples;
      uint32_t pic_height_in_luma_samples;
      uint8_t conformance_window_flag;
      uint32_t conf_win_left_offset;
      uint32_t conf_win_right_offset;
      uint32_t conf_win_top_offset;
      uint32_t conf_win_bottom_offset;
      uint8_t bit_depth_luma_minus8;
      uint8_t bit_depth_chroma_minus8;
      uint8_t log2_max_pic_order_cnt_lsb_minus4;
      uint8_t sps_sub_layer_ordering_info_present_flag : 1;
      uint8_t sps_max_dec_pic_buffering_minus1[7];
      uint8_t sps_max_num_reorder_pics[7];
      uint32_t sps_max_latency_increase_plus1[7];
      uint8_t log2_min_luma_coding_block_size_minus3;
      uint8_t log2_diff_max_min_luma_coding_block_size;
      uint8_t log2_min_luma_transform_block_size_minus2;
      uint8_t log2_diff_max_min_luma_transform_block_size;
      uint8_t max_transform_hierarchy_depth_inter;
      uint8_t max_transform_hierarchy_depth_intra;
      uint8_t amp_enabled_flag : 1;
      uint8_t sample_adaptive_offset_enabled_flag : 1;
      uint8_t pcm_enabled_flag : 1;
      uint8_t pcm_sample_bit_depth_luma_minus1;
      uint8_t pcm_sample_bit_depth_chroma_minus1;
      uint8_t log2_min_pcm_luma_coding_block_size_minus3;
      uint8_t log2_diff_max_min_pcm_luma_coding_block_size;
      uint8_t pcm_loop_filter_disabled_flag : 1;
      uint8_t num_short_term_ref_pic_sets;
      struct st_ref_pic_set st_ref_pic_set[17];
      uint8_t long_term_ref_pics_present_flag : 1;
      uint8_t sps_temporal_mvp_enabled_flag: 1;
      uint8_t strong_intra_smoothing_enabled_flag : 1;
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
      uint8_t vui_timing_info_present_flag : 1;
      uint32_t vui_num_units_in_tick;
      uint32_t vui_time_scale;
      uint8_t vui_poc_proportional_to_timing_flag : 1;
      uint32_t vui_num_ticks_poc_diff_one_minus1;
   };

   struct pps {
      uint8_t pps_pic_parameter_set_id;
      uint8_t pps_seq_parameter_set_id;
      uint8_t output_flag_present_flag : 1;
      uint8_t sign_data_hiding_enabled_flag : 1;
      uint8_t cabac_init_present_flag : 1;
      uint8_t num_ref_idx_l0_default_active_minus1;
      uint8_t num_ref_idx_l1_default_active_minus1;
      int16_t init_qp_minus26;
      uint8_t constrained_intra_pred_flag : 1;
      uint8_t transform_skip_enabled_flag : 1;
      uint8_t cu_qp_delta_enabled_flag : 1;
      uint8_t diff_cu_qp_delta_depth;
      int16_t pps_cb_qp_offset;
      int16_t pps_cr_qp_offset;
      uint8_t pps_slice_chroma_qp_offsets_present_flag : 1;
      uint8_t weighted_pred_flag : 1;
      uint8_t weighted_bipred_flag : 1;
      uint8_t transquant_bypass_enabled_flag : 1;
      uint8_t entropy_coding_sync_enabled_flag : 1;
      uint8_t pps_loop_filter_across_slices_enabled_flag : 1;
      uint8_t deblocking_filter_control_present_flag : 1;
      uint8_t deblocking_filter_override_enabled_flag : 1;
      uint8_t pps_deblocking_filter_disabled_flag : 1;
      int16_t pps_beta_offset_div2;
      int16_t pps_tc_offset_div2;
      uint8_t log2_parallel_merge_level_minus2;
      uint8_t slice_segment_header_extension_present_flag : 1;
   };

   struct slice {
      uint8_t nal_unit_type;
      uint8_t temporal_id;
      uint8_t first_slice_segment_in_pic_flag : 1;
      uint8_t no_output_of_prior_pics_flag : 1;
      uint8_t slice_pic_parameter_set_id;
      uint8_t slice_type;
      uint32_t pic_output_flag : 1;
      uint32_t slice_pic_order_cnt_lsb;
      uint8_t short_term_ref_pic_set_sps_flag : 1;
      uint8_t short_term_ref_pic_set_idx;
      uint8_t num_long_term_pics;
      uint32_t poc_lsb_lt[16];
      uint8_t used_by_curr_pic_lt_flag[16];
      uint8_t delta_poc_msb_present_flag[16];
      uint32_t delta_poc_msb_cycle_lt[16];
      uint8_t slice_temporal_mvp_enabled_flag : 1;
      uint8_t slice_sao_luma_flag : 1;
      uint8_t slice_sao_chroma_flag : 1;
      uint8_t num_ref_idx_active_override_flag : 1;
      uint8_t num_ref_idx_l0_active_minus1;
      uint8_t num_ref_idx_l1_active_minus1;
      uint8_t mvd_l1_zero_flag : 1;
      uint8_t cabac_init_flag : 1;
      uint8_t five_minus_max_num_merge_cand;
      int16_t slice_qp_delta;
      uint8_t deblocking_filter_override_flag : 1;
      uint8_t slice_deblocking_filter_disabled_flag : 1;
      int16_t slice_cb_qp_offset;
      int16_t slice_cr_qp_offset;
      int16_t slice_beta_offset_div2;
      int16_t slice_tc_offset_div2;
      uint8_t slice_loop_filter_across_slices_enabled_flag : 1;
   };

   bitstream_hevc();

   void write_vps(const vps &vps);
   void write_sps(const sps &sps);
   void write_pps(const pps &pps);
   void write_slice(const slice &slice, const sps &sps, const pps &pps);

private:
   void write_nal_header(uint8_t nal_unit_type, uint8_t temporal_id);
   void write_profile_tier_level(uint8_t max_sublayers_minus1, const profile_tier_level &ptl);
   void write_st_ref_pic_set(uint8_t index, const st_ref_pic_set &rps);
};
