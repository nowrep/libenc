#include "bitstream_hevc.h"
#include "util.h"

bitstream_hevc::bitstream_hevc()
   : bitstream()
{
}

void bitstream_hevc::write_vps(const vps &vps)
{
   write_nal_header(32, 0);

   ui(vps.vps_video_parameter_set_id, 4);
   ui(vps.vps_base_layer_internal_flag, 1);
   ui(vps.vps_base_layer_available_flag, 1);
   ui(0x0, 6); // vps_max_layers_minus1
   ui(vps.vps_max_sub_layers_minus1, 3);
   ui(vps.vps_temporal_id_nesting_flag, 1);
   ui(0xffff, 16); // vps_reserved_0xffff_16bits
   write_profile_tier_level(vps.vps_max_sub_layers_minus1, vps.profile_tier_level);
   ui(vps.vps_sub_layer_ordering_info_present_flag, 1);
   uint32_t i = vps.vps_sub_layer_ordering_info_present_flag ? 0 : vps.vps_max_sub_layers_minus1;
   for (; i <= vps.vps_max_sub_layers_minus1; i++) {
      ue(vps.vps_max_dec_pic_buffering_minus1[i]);
      ue(vps.vps_max_num_reorder_pics[i]);
      ue(vps.vps_max_latency_increase_plus1[i]);
   }
   ui(0x0, 6); // vps_max_layer_id
   ue(0x0); // vps_num_layer_sets_minus1
   ui(vps.vps_timing_info_present_flag, 1);
   if (vps.vps_timing_info_present_flag) {
      ui(vps.vps_num_units_in_tick, 32);
      ui(vps.vps_time_scale, 32);
      ui(vps.vps_poc_proportional_to_timing_flag, 1);
      if (vps.vps_poc_proportional_to_timing_flag)
         ue(vps.vps_num_ticks_poc_diff_one_minus1);
      ue(0x0); // vps_num_hrd_parameters
   }
   ui(0x0, 1); // vps_extension_flag

   trailing_bits();
}

void bitstream_hevc::write_sps(const sps &sps)
{
   write_nal_header(33, 0);

   ui(sps.sps_video_parameter_set_id, 4);
   ui(sps.sps_max_sub_layers_minus1, 3);
   ui(sps.sps_temporal_id_nesting_flag, 1);
   write_profile_tier_level(sps.sps_max_sub_layers_minus1, sps.profile_tier_level);
   ue(sps.sps_seq_parameter_set_id);
   ue(sps.chroma_format_idc);
   if (sps.chroma_format_idc == 3)
      ui(0x0, 1); // separate_colour_plane_flag
   ue(sps.pic_width_in_luma_samples);
   ue(sps.pic_height_in_luma_samples);
   ui(sps.conformance_window_flag, 1);
   if (sps.conformance_window_flag) {
      ue(sps.conf_win_left_offset);
      ue(sps.conf_win_right_offset);
      ue(sps.conf_win_top_offset);
      ue(sps.conf_win_bottom_offset);
   }
   ue(sps.bit_depth_luma_minus8);
   ue(sps.bit_depth_chroma_minus8);
   ue(sps.log2_max_pic_order_cnt_lsb_minus4);
   ui(sps.sps_sub_layer_ordering_info_present_flag, 1);
   uint32_t i = sps.sps_sub_layer_ordering_info_present_flag ? 0 : sps.sps_max_sub_layers_minus1;
   for (; i <= sps.sps_max_sub_layers_minus1; i++) {
      ue(sps.sps_max_dec_pic_buffering_minus1[i]);
      ue(sps.sps_max_num_reorder_pics[i]);
      ue(sps.sps_max_latency_increase_plus1[i]);
   }
   ue(sps.log2_min_luma_coding_block_size_minus3);
   ue(sps.log2_diff_max_min_luma_coding_block_size);
   ue(sps.log2_min_luma_transform_block_size_minus2);
   ue(sps.log2_diff_max_min_luma_transform_block_size);
   ue(sps.max_transform_hierarchy_depth_inter);
   ue(sps.max_transform_hierarchy_depth_intra);
   ui(0x0, 1); // scaling_list_enabled_flag
   ui(sps.amp_enabled_flag, 1);
   ui(sps.sample_adaptive_offset_enabled_flag, 1);
   ui(sps.pcm_enabled_flag, 1);
   if (sps.pcm_enabled_flag) {
      ui(sps.pcm_sample_bit_depth_luma_minus1, 4);
      ui(sps.pcm_sample_bit_depth_chroma_minus1, 4);
      ue(sps.log2_min_pcm_luma_coding_block_size_minus3);
      ue(sps.log2_diff_max_min_pcm_luma_coding_block_size);
      ui(sps.pcm_loop_filter_disabled_flag, 1);
   }
   ue(sps.num_short_term_ref_pic_sets);
   for (uint32_t i = 0; i < sps.num_short_term_ref_pic_sets; i++)
      write_st_ref_pic_set(i, sps.st_ref_pic_set[i]);
   ui(sps.long_term_ref_pics_present_flag, 1);
   if (sps.long_term_ref_pics_present_flag)
      ue(0x0); // num_long_term_ref_pics_sps
   ui(sps.sps_temporal_mvp_enabled_flag, 1);
   ui(sps.strong_intra_smoothing_enabled_flag, 1);
   ui(sps.vui_parameters_present_flag, 1);
   if (sps.vui_parameters_present_flag) {
      ui(sps.aspect_ratio_info_present_flag, 1);
      if (sps.aspect_ratio_info_present_flag) {
         ui(sps.aspect_ratio_idc, 8);
         if (sps.aspect_ratio_idc == 255) {
            ui(sps.sar_width, 16);
            ui(sps.sar_height, 16);
         }
      }
      ui(sps.overscan_info_present_flag, 1);
      if (sps.overscan_info_present_flag)
         ui(sps.overscan_appropriate_flag, 1);
      ui(sps.video_signal_type_present_flag, 1);
      if (sps.video_signal_type_present_flag) {
         ui(sps.video_format, 3);
         ui(sps.video_full_range_flag, 1);
         ui(sps.colour_description_present_flag, 1);
         if (sps.colour_description_present_flag) {
            ui(sps.colour_primaries, 8);
            ui(sps.transfer_characteristics, 8);
            ui(sps.matrix_coefficients, 8);
         }
      }
      ui(sps.chroma_loc_info_present_flag, 1);
      if (sps.chroma_loc_info_present_flag) {
         ue(sps.chroma_sample_loc_type_top_field);
         ue(sps.chroma_sample_loc_type_bottom_field);
      }
      ui(0x0, 1); // neutral_chroma_indication_flag
      ui(0x0, 1); // field_seq_flag
      ui(0x0, 1); // frame_field_info_present_flag
      ui(0x0, 1); // default_display_window_flag
      ui(sps.vui_timing_info_present_flag, 1);
      if (sps.vui_timing_info_present_flag) {
         ui(sps.vui_num_units_in_tick, 32);
         ui(sps.vui_time_scale, 32);
         ui(sps.vui_poc_proportional_to_timing_flag, 1);
         if (sps.vui_poc_proportional_to_timing_flag)
            ue(sps.vui_num_ticks_poc_diff_one_minus1);
         ui(0x0, 1); // vui_hrd_parameters_present_flag
      }
      ui(0x0, 1); // bitstream_restriction_flag
   }
   ui(0x0, 1); // sps_extension_present_flag

   trailing_bits();
}

void bitstream_hevc::write_pps(const pps &pps)
{
   write_nal_header(34, 0);

   ue(pps.pps_pic_parameter_set_id);
   ue(pps.pps_seq_parameter_set_id);
   ui(0x0, 1); // dependent_slice_segments_enabled_flag
   ui(pps.output_flag_present_flag, 1);
   ui(0x0, 3); // num_extra_slice_header_bits
   ui(pps.sign_data_hiding_enabled_flag, 1);
   ui(pps.cabac_init_present_flag, 1);
   ue(pps.num_ref_idx_l0_default_active_minus1);
   ue(pps.num_ref_idx_l1_default_active_minus1);
   se(pps.init_qp_minus26);
   ui(pps.constrained_intra_pred_flag, 1);
   ui(pps.transform_skip_enabled_flag, 1);
   ui(pps.cu_qp_delta_enabled_flag, 1);
   if (pps.cu_qp_delta_enabled_flag)
      ue(pps.diff_cu_qp_delta_depth);
   se(pps.pps_cb_qp_offset);
   se(pps.pps_cr_qp_offset);
   ui(pps.pps_slice_chroma_qp_offsets_present_flag, 1);
   ui(pps.weighted_pred_flag, 1);
   ui(pps.weighted_bipred_flag, 1);
   ui(pps.transquant_bypass_enabled_flag, 1);
   ui(0x0, 1); // tiles_enabled_flag
   ui(pps.entropy_coding_sync_enabled_flag, 1);
   ui(pps.pps_loop_filter_across_slices_enabled_flag, 1);
   ui(pps.deblocking_filter_control_present_flag, 1);
   if (pps.deblocking_filter_control_present_flag) {
      ui(pps.deblocking_filter_override_enabled_flag, 1);
      ui(pps.pps_deblocking_filter_disabled_flag, 1);
      if (!pps.pps_deblocking_filter_disabled_flag) {
         se(pps.pps_beta_offset_div2);
         se(pps.pps_tc_offset_div2);
      }
   }
   ui(0x0, 1); // pps_scaling_list_data_present_flag
   ui(0x0, 1); // lists_modification_present_flag
   ue(pps.log2_parallel_merge_level_minus2);
   ui(pps.slice_segment_header_extension_present_flag, 1);
   ui(0x0, 1); // pps_extension_present_flag

   trailing_bits();
}

void bitstream_hevc::write_slice(const slice &slice, const sps &sps, const pps &pps)
{
   write_nal_header(slice.nal_unit_type, slice.temporal_id);

   ui(slice.first_slice_segment_in_pic_flag, 1);
   if (slice.nal_unit_type >= 16 && slice.nal_unit_type <= 23)
      ui(slice.no_output_of_prior_pics_flag, 1);
   ue(slice.slice_pic_parameter_set_id);
   // if (!slice.first_slice_segment_in_pic_flag)
      // ui(slice.slice_segment_address, v);
   ue(slice.slice_type);
   if (pps.output_flag_present_flag)
      ui(slice.pic_output_flag, 1);
   if (slice.nal_unit_type != 19 && slice.nal_unit_type != 20) {
      ui(slice.slice_pic_order_cnt_lsb, sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
      ui(slice.short_term_ref_pic_set_sps_flag, 1);
      if (!slice.short_term_ref_pic_set_sps_flag)
         write_st_ref_pic_set(sps.num_short_term_ref_pic_sets, sps.st_ref_pic_set[sps.num_short_term_ref_pic_sets]);
      else if (sps.num_short_term_ref_pic_sets > 1)
         ui(slice.short_term_ref_pic_set_idx, logbase2_ceil(sps.num_short_term_ref_pic_sets));
      if (sps.long_term_ref_pics_present_flag) {
         ue(slice.num_long_term_pics);
         for (uint32_t i = 0; i < slice.num_long_term_pics; i++) {
            ui(slice.poc_lsb_lt[i], sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
            ui(slice.used_by_curr_pic_lt_flag[i], 1);
            ui(slice.delta_poc_msb_present_flag[i], 1);
            if (slice.delta_poc_msb_present_flag[i])
               ue(slice.delta_poc_msb_cycle_lt[i]);
         }
      }
      if (sps.sps_temporal_mvp_enabled_flag)
         ui(slice.slice_temporal_mvp_enabled_flag, 1);
   }
   if (sps.sample_adaptive_offset_enabled_flag) {
      ui(slice.slice_sao_luma_flag, 1);
      ui(slice.slice_sao_chroma_flag, 1);
   }
   if (slice.slice_type == 1 || slice.slice_type == 0) {
      ui(slice.num_ref_idx_active_override_flag, 1);
      if (slice.num_ref_idx_active_override_flag) {
         ue(slice.num_ref_idx_l0_active_minus1);
         if (slice.slice_type == 0)
            ue(slice.num_ref_idx_l1_active_minus1);
      }
      if (slice.slice_type == 0)
         ui(slice.mvd_l1_zero_flag, 1);
      if (pps.cabac_init_present_flag)
         ui(slice.cabac_init_flag, 1);
      ue(slice.five_minus_max_num_merge_cand);
   }
   se(slice.slice_qp_delta);
   if (pps.pps_slice_chroma_qp_offsets_present_flag) {
      se(slice.slice_cb_qp_offset);
      se(slice.slice_cr_qp_offset);
   }
   if (pps.deblocking_filter_override_enabled_flag)
      ui(slice.deblocking_filter_override_flag, 1);
   if (slice.deblocking_filter_override_flag) {
      ui(slice.slice_deblocking_filter_disabled_flag, 1);
      if (!slice.slice_deblocking_filter_disabled_flag) {
         se(slice.slice_beta_offset_div2);
         se(slice.slice_tc_offset_div2);
      }
   }
   if (pps.pps_loop_filter_across_slices_enabled_flag && (slice.slice_sao_luma_flag || slice.slice_sao_chroma_flag || !slice.slice_deblocking_filter_disabled_flag))
      ui(slice.slice_loop_filter_across_slices_enabled_flag, 1);

   trailing_bits();
}

void bitstream_hevc::write_nal_header(uint8_t nal_unit_type, uint8_t temporal_id)
{
   ui(0x1, 32); // startcode
   ui(0x0, 1); // forbidden_zero_bit
   ui(nal_unit_type, 6);
   ui(0x0, 6); // nuh_layer_id
   ui(temporal_id + 1, 3);
}

void bitstream_hevc::write_profile_tier_level(uint8_t max_sublayers_minus1, const profile_tier_level &ptl)
{
   auto profile_tier = [this](const struct profile_tier &pt) {
      ui(pt.general_profile_space, 2);
      ui(pt.general_tier_flag, 1);
      ui(pt.general_profile_idc, 5);
      for (uint32_t i = 0; i < 32; i++)
         ui(pt.general_profile_compatibility_flag[i], 1);
      ui(pt.general_progressive_source_flag, 1);
      ui(pt.general_interlaced_source_flag, 1);
      ui(pt.general_non_packed_constraint_flag, 1);
      ui(pt.general_frame_only_constraint_flag, 1);
      // general_reserved_zero_44bits
      ui(0x0, 16);
      ui(0x0, 16);
      ui(0x0, 12);
   };

   profile_tier(ptl.profile_tier);
   ui(ptl.general_level_idc, 8);

   for (uint32_t i = 0; i < max_sublayers_minus1; i++) {
      ui(ptl.sub_layer_profile_present_flag[i], 1);
      ui(ptl.sub_layer_level_present_flag[i], 1);
   }

   if (max_sublayers_minus1 > 0) {
      for (uint32_t i = max_sublayers_minus1; i < 8; i++)
         ui(0x0, 2); // reserved_zero_2bits
   }

   for (uint32_t i = 0; i < max_sublayers_minus1; i++) {
      if (ptl.sub_layer_profile_present_flag[i])
         profile_tier(ptl.sub_layer_profile_tier[i]);

      if (ptl.sub_layer_level_present_flag[i])
         ui(ptl.sub_layer_level_idc[i], 8);
   }
}

void bitstream_hevc::write_st_ref_pic_set(uint8_t index, const st_ref_pic_set &rps)
{
   if (index != 0)
      ui(0x0, 1); // inter_ref_pic_set_prediction_flag
   ue(rps.num_negative_pics);
   ue(rps.num_positive_pics);
   for (uint32_t i = 0; i < rps.num_negative_pics; i++) {
      ue(rps.delta_poc_s0_minus1[i]);
      ui(rps.used_by_curr_pic_s0_flag[i], 1);
   }
   for (uint32_t i = 0; i < rps.num_positive_pics; i++) {
      ue(rps.delta_poc_s1_minus1[i]);
      ui(rps.used_by_curr_pic_s1_flag[i], 1);
   }
}
