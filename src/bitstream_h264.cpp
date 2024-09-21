#include "bitstream_h264.h"

bitstream_h264::bitstream_h264()
   : bitstream()
{
}

void bitstream_h264::write_sps(const sps &sps)
{
   write_nal_header(0, 7);

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
   ui(0x0, 1); // vui_parameters_present_flag

   trailing_bits();
}

void bitstream_h264::write_pps(const pps &pps)
{
   write_nal_header(0, 8);

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
   if (slice.nal_ref_idc == 3)
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
      if (slice.nal_ref_idc == 3) {
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

void bitstream_h264::write_nal_header(uint8_t nal_ref_idc, uint8_t nal_unit_type)
{
   ui(0x1, 32); // startcode
   ui(0x0, 1); // forbidden_zero_bit
   ui(nal_ref_idc, 2);
   ui(nal_unit_type, 5);
}
