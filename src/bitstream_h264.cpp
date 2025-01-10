#include "bitstream_h264.h"

bitstream_h264::bitstream_h264(bool emulation_prevention)
   : bitstream()
   , emulation_prevention(emulation_prevention)
{
}

void bitstream_h264::write_sps(const sps &sps)
{
   write_nal_header(3, 7);

   ui(sps.profile_idc, 8);
   ui(sps.constraint_set_flags, 6);
   ui(0x0, 2); // reserved_zero_2bits
   ui(sps.level_idc, 8);
   ue(sps.seq_parameter_set_id);
   if (sps.profile_idc == 100 || sps.profile_idc == 110 ||
       sps.profile_idc == 122 || sps.profile_idc == 244 || sps.profile_idc == 44 ||
       sps.profile_idc == 83 || sps.profile_idc == 86 || sps.profile_idc == 118 ||
       sps.profile_idc == 128 || sps.profile_idc == 138 || sps.profile_idc == 139 ||
       sps.profile_idc == 134 || sps.profile_idc == 135) {
      ue(sps.chroma_format_idc);
      if (sps.chroma_format_idc == 3)
         ui(0x0, 1); // separate_colour_plane_flag
      ue(sps.bit_depth_luma_minus8);
      ue(sps.bit_depth_chroma_minus8);
      ui(0x0, 1); // qpprime_y_zero_transform_bypass_flag
      ui(0x0, 1); // seq_scaling_matrix_present_flag
   }
   ue(sps.log2_max_frame_num_minus4);
   ue(sps.pic_order_cnt_type);
   if (sps.pic_order_cnt_type == 0) {
      ue(sps.log2_max_pic_order_cnt_lsb_minus4);
   } else if (sps.pic_order_cnt_type == 1) {
      // TODO
   }
   ue(sps.max_num_ref_frames);
   ui(sps.gaps_in_frame_num_value_allowed_flag, 1);
   ue(sps.pic_width_in_mbs_minus1);
   ue(sps.pic_height_in_map_units_minus1);
   ui(sps.frame_mbs_only_flag, 1);
   if (!sps.frame_mbs_only_flag)
      ui(sps.mb_adaptive_frame_field_flag, 1);
   ui(sps.direct_8x8_inference_flag, 1);
   ui(sps.frame_cropping_flag, 1);
   if (sps.frame_cropping_flag) {
      ue(sps.frame_crop_left_offset);
      ue(sps.frame_crop_right_offset);
      ue(sps.frame_crop_top_offset);
      ue(sps.frame_crop_bottom_offset);
   }
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
      ui(sps.timing_info_present_flag, 1);
      if (sps.timing_info_present_flag) {
         ui(sps.num_units_in_tick, 32);
         ui(sps.time_scale, 32);
         ui(sps.fixed_frame_rate_flag, 1);
      }
      ui(0x0, 1); // nal_hrd_parameters_present_flag
      ui(0x0, 1); // vcl_hrd_parameters_present_flag
      ui(sps.pic_struct_present_flag, 1);
      ui(sps.bitstream_restriction_flag, 1);
      if (sps.bitstream_restriction_flag) {
         ui(sps.motion_vectors_over_pic_boundaries_flag, 1);
         ue(sps.max_bytes_per_pic_denom);
         ue(sps.max_bits_per_mb_denom);
         ue(sps.log2_max_mv_length_horizontal);
         ue(sps.log2_max_mv_length_vertical);
         ue(sps.max_num_reorder_frames);
         ue(sps.max_dec_frame_buffering);
      }
   }

   trailing_bits();
}

void bitstream_h264::write_pps(const pps &pps)
{
   write_nal_header(3, 8);

   ue(pps.pic_parameter_set_id);
   ue(pps.seq_parameter_set_id);
   ui(pps.entropy_coding_mode_flag, 1);
   ui(0x0, 1); // bottom_field_pic_order_in_frame_present_flag
   ue(0x0); // num_slice_groups_minus1
   ue(pps.num_ref_idx_l0_default_active_minus1);
   ue(pps.num_ref_idx_l1_default_active_minus1);
   ui(pps.weighted_pred_flag, 1);
   ui(pps.weighted_bipred_idc, 2);
   se(pps.pic_init_qp_minus26);
   se(pps.pic_init_qs_minus26);
   se(pps.chroma_qp_index_offset);
   ui(pps.deblocking_filter_control_present_flag, 1);
   ui(pps.constrained_intra_pred_flag, 1);
   ui(pps.redundant_pic_cnt_present_flag, 1);
   ui(pps.transform_8x8_mode_flag, 1);
   ui(0x0, 1); // pic_scaling_matrix_present_flag
   se(pps.second_chroma_qp_index_offset);

   trailing_bits();
}

void bitstream_h264::write_slice(const slice &slice, const sps &sps, const pps &pps)
{
   uint8_t slice_type = slice.slice_type % 5;

   write_nal_header(slice.nal_ref_idc, slice.nal_unit_type);

   ue(slice.first_mb_in_slice);
   ue(slice.slice_type);
   ue(slice.pic_parameter_set_id);
   ui(slice.frame_num, sps.log2_max_frame_num_minus4 + 4);
   if (slice.nal_unit_type == 5)
      ue(slice.idr_pic_id);
   if (sps.pic_order_cnt_type == 0)
      ui(slice.pic_order_cnt_lsb, sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
   if (pps.redundant_pic_cnt_present_flag)
      ue(slice.redundant_pic_cnt);
   if (slice_type == 1)
      ui(slice.direct_spatial_mv_pred_flag, 1);
   if (slice_type == 0 || slice_type == 3 || slice_type == 1) {
      ui(slice.num_ref_idx_active_override_flag, 1);
      if (slice.num_ref_idx_active_override_flag) {
         ue(slice.num_ref_idx_l0_active_minus1);
         if (slice_type == 1)
            ue(slice.num_ref_idx_l1_active_minus1);
      }
   }
   if (slice_type != 2 && slice_type != 4) {
      ui(slice.ref_pic_list_modification_flag_l0, 1);
      if (slice.ref_pic_list_modification_flag_l0) {
         for (uint32_t i = 0; i < slice.num_ref_list0_mod; i++) {
            uint8_t op = slice.ref_list0_mod[i].modification_of_pic_nums_idc;
            ue(op);
            if (op == 0 || op == 1)
               ue(slice.ref_list0_mod[i].abs_diff_pic_num_minus1);
            else if (op == 2)
               ue(slice.ref_list0_mod[i].long_term_pic_num);
         }
         ue(0x3); // modification_of_pic_nums_idc
      }
   }
   if (slice_type == 1) {
      ui(slice.ref_pic_list_modification_flag_l1, 1);
      if (slice.ref_pic_list_modification_flag_l1) {
         for (uint32_t i = 0; i < slice.num_ref_list1_mod; i++) {
            uint8_t op = slice.ref_list1_mod[i].modification_of_pic_nums_idc;
            ue(op);
            if (op == 0 || op == 1)
               ue(slice.ref_list1_mod[i].abs_diff_pic_num_minus1);
            else if (op == 2)
               ue(slice.ref_list1_mod[i].long_term_pic_num);
         }
         ue(0x3); // modification_of_pic_nums_idc
      }
   }
   if (slice.nal_ref_idc != 0) {
      if (slice.nal_unit_type == 5) {
         ui(slice.no_output_of_prior_pics_flag, 1);
         ui(slice.long_term_reference_flag, 1);
      } else {
         ui(slice.adaptive_ref_pic_marking_mode_flag, 1);
         if (slice.adaptive_ref_pic_marking_mode_flag) {
            for (uint32_t i = 0; i < slice.num_mmco_op; i++) {
               uint8_t op = slice.mmco_op[i].memory_management_control_operation;
               ue(op);
               if (op == 1 || op == 3)
                  ue(slice.mmco_op[i].difference_of_pic_nums_minus1);
               if (op == 2)
                  ue(slice.mmco_op[i].long_term_pic_num);
               if (op == 3 || op == 6)
                  ue(slice.mmco_op[i].long_term_frame_idx);
               if (op == 4)
                  ue(slice.mmco_op[i].max_long_term_frame_idx_plus1);
            }
            ue(0x0); // memory_management_control_operation
         }
      }
   }
   if (pps.entropy_coding_mode_flag && slice_type != 2 && slice_type != 4)
      ue(slice.cabac_init_idc);
   se(slice.slice_qp_delta);
   if (pps.deblocking_filter_control_present_flag) {
      ue(slice.disable_deblocking_filter_idc);
      if (slice.disable_deblocking_filter_idc != 1) {
         se(slice.slice_alpha_c0_offset_div2);
         se(slice.slice_beta_offset_div2);
      }
   }

   trailing_bits();
}

void bitstream_h264::write_prefix(const prefix &pfx)
{
   write_nal_header(pfx.nal_ref_idc, 14);

   ui(pfx.svc_extension_flag, 1);
   if (pfx.svc_extension_flag) {
      ui(pfx.idr_flag, 1);
      ui(pfx.priority_id, 6);
      ui(pfx.no_inter_layer_pred_flag, 1);
      ui(pfx.dependency_id, 3);
      ui(pfx.quality_id, 4);
      ui(pfx.temporal_id, 3);
      ui(pfx.use_ref_base_pic_flag, 1);
      ui(pfx.discardable_flag, 1);
      ui(pfx.output_flag, 1);
      ui(0x3, 2); // reserved_three_2bits
   }
}

void bitstream_h264::write_sei_recovery_point(const sei_recovery_point &srp)
{
   write_nal_header(0, 6);

   bitstream bs;
   bs.ue(srp.recovery_frame_cnt);
   bs.ui(srp.exact_match_flag, 1);
   bs.ui(srp.broken_link_flag, 1);
   bs.ui(srp.changing_slice_group_idc, 2);
   bs.byte_align();

   write_sei(6, bs);
}

void bitstream_h264::write_sei_scalability_info(const sei_scalability_info &ssi)
{
   write_nal_header(0, 6);

   bitstream bs;
   bs.ui(0x0, 1); // temporal_id_nesting_flag
   bs.ui(0x0, 1); // priority_layer_info_present_flag
   bs.ui(0x0, 1); // priority_id_setting_flag
   bs.ue(ssi.num_layers_minus1);
   for (uint32_t i = 0; i <= ssi.num_layers_minus1; i++) {
      bs.ue(i); // layer_id[i]
      bs.ui(0x0, 6); // priority_id[i]
      bs.ui(0x0, 1); // discardable_flag[i]
      bs.ui(0x0, 3); // dependency_id[i]
      bs.ui(0x0, 4); // quality_id[i]
      bs.ui(i, 3); // temporal_id[i]
      bs.ui(0x0, 1); // sub_pic_layer_flag[i]
      bs.ui(0x0, 1); // sub_region_layer_flag[i]
      bs.ui(0x0, 1); // iroi_division_info_present_flag[i]
      bs.ui(0x0, 1); // profile_level_info_present_flag[i]
      bs.ui(0x0, 1); // bitrate_info_present_flag[i]
      bs.ui(0x0, 1); // frm_rate_info_present_flag[i]
      bs.ui(0x0, 1); // frm_size_info_present_flag[i]
      bs.ui(0x0, 1); // layer_dependency_info_present_flag[i]
      bs.ui(0x0, 1); // parameter_sets_info_present_flag[i]
      bs.ui(0x0, 1); // bitstream_restriction_info_present_flag[i]
      bs.ui(0x0, 1); // exact_inter_layer_pred_flag[i]
      bs.ui(0x0, 1); // layer_conversion_flag[i]
      bs.ui(0x0, 1); // layer_output_flag[i]
      bs.ue(0x0); // layer_dependency_info_src_layer_id_delta[i]
      bs.ue(0x0); // parameter_sets_info_src_layer_id_delta[i]
   }
   bs.byte_align();

   write_sei(24, bs);
}

void bitstream_h264::write_nal_header(uint8_t nal_ref_idc, uint8_t nal_unit_type)
{
   ui(0x1, 32); // startcode
   ui(0x0, 1); // forbidden_zero_bit
   ui(nal_ref_idc, 2);
   ui(nal_unit_type, 5);

   if (emulation_prevention)
      enable_emulation_prevention();
}

void bitstream_h264::write_sei(uint32_t type, const bitstream &bs)
{
   ui(type, 8); // last_payload_type_byte
   ui(bs.size(), 8); // last_payload_size_byte

   for (uint32_t i = 0; i < bs.size(); i++)
      ui(bs.data()[i], 8);

   trailing_bits();
}
