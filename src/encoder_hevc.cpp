#include "encoder_hevc.h"
#include "bitstream_hevc.h"

#include <algorithm>

encoder_hevc::encoder_hevc()
   : enc_encoder()
{
   unit_width = 64;
   unit_height = 64;
}

bool encoder_hevc::create(const struct enc_encoder_params *params)
{
   std::vector<VAConfigAttrib> attribs;

   VAConfigAttrib attrib;
   attrib.type = VAConfigAttribEncPackedHeaders;
   attrib.value = VA_ENC_PACKED_HEADER_SEQUENCE | VA_ENC_PACKED_HEADER_PICTURE | VA_ENC_PACKED_HEADER_SLICE | VA_ENC_PACKED_HEADER_RAW_DATA;
   attribs.push_back(attrib);

   VAProfile profile = VAProfileNone;
   switch (params->hevc.profile) {
   case ENC_HEVC_PROFILE_MAIN:
      profile = VAProfileHEVCMain;
      break;
   case ENC_HEVC_PROFILE_MAIN_10:
      profile = VAProfileHEVCMain10;
      break;
   default:
      std::cerr << "Invalid HEVC profile " << params->hevc.profile << std::endl;
      return false;
      break;
   }

   if (!create_context(params, profile, attribs))
      return false;

   dpb_poc.resize(dpb.size());

   bitstream_hevc::profile_tier_level ptl = {};
   ptl.profile_tier.general_profile_idc = params->hevc.profile;
   ptl.profile_tier.general_tier_flag = params->hevc.tier;
   ptl.general_level_idc = params->hevc.level;
   ptl.profile_tier.general_profile_compatibility_flag = 1 << ptl.profile_tier.general_profile_idc;
   if ((ptl.profile_tier.general_profile_compatibility_flag >> 1) & 1)
      ptl.profile_tier.general_profile_compatibility_flag |= 4;
   if ((ptl.profile_tier.general_profile_compatibility_flag >> 3) & 1)
      ptl.profile_tier.general_profile_compatibility_flag |= 6;
   ptl.profile_tier.general_progressive_source_flag = 1;
   ptl.profile_tier.general_non_packed_constraint_flag = 1;
   ptl.profile_tier.general_frame_only_constraint_flag = 1;

   vps.vps_base_layer_internal_flag = 1;
   vps.vps_base_layer_available_flag = 1;
   vps.vps_max_sub_layers_minus1 = num_layers - 1;
   vps.vps_temporal_id_nesting_flag = 1;
   vps.profile_tier_level = ptl;
   vps.vps_max_dec_pic_buffering_minus1[0] = num_refs;
   vps.vps_timing_info_present_flag = 1;
   auto framerate = get_framerate(params->rc_params[0].frame_rate);
   vps.vps_time_scale = framerate.first;
   vps.vps_num_units_in_tick = framerate.second;
   vps.vps_poc_proportional_to_timing_flag = 1;

   sps.sps_max_sub_layers_minus1 = num_layers - 1;
   sps.sps_temporal_id_nesting_flag = 1;
   sps.profile_tier_level = ptl;
   sps.chroma_format_idc = 1;
   sps.pic_width_in_luma_samples = aligned_width;
   sps.pic_height_in_luma_samples = aligned_height;
   if (aligned_width != params->width || aligned_height != params->height) {
      sps.conformance_window_flag = 1;
      sps.conf_win_left_offset = 0;
      sps.conf_win_right_offset = (aligned_width - params->width) / 2;
      sps.conf_win_top_offset = 0;
      sps.conf_win_bottom_offset = (aligned_height - params->height) / 2;
   }
   sps.bit_depth_luma_minus8 = params->bit_depth - 8;
   sps.bit_depth_chroma_minus8 = params->bit_depth - 8;
   sps.log2_max_pic_order_cnt_lsb_minus4 = 4;
   sps.sps_max_dec_pic_buffering_minus1[0] = num_refs;
   sps.long_term_ref_pics_present_flag = 1;
   sps.vui_parameters_present_flag = 1;
   sps.video_signal_type_present_flag = 1;
   sps.video_format = 5;
   sps.vui_timing_info_present_flag = 1;
   sps.vui_time_scale = vps.vps_time_scale;
   sps.vui_num_units_in_tick = vps.vps_num_units_in_tick;

   pps.num_ref_idx_l0_default_active_minus1 = 0;
   pps.init_qp_minus26 = 0;
   pps.pps_loop_filter_across_slices_enabled_flag = 1;

   return true;
}

struct enc_task *encoder_hevc::encode_frame(const struct enc_frame_params *params)
{
   auto task = begin_encode(params);
   if (!task)
      return {};

   const bool is_idr = enc_params.frame_type == ENC_FRAME_TYPE_IDR;

   if (is_idr)
      pic_order_cnt = 0;

   dpb_poc[enc_params.recon_slot] = pic_order_cnt;

   bitstream_hevc bs;

   if (enc_params.need_sequence_headers) {
      VAEncSequenceParameterBufferHEVC seq = {};
      seq.general_profile_idc = sps.profile_tier_level.profile_tier.general_profile_idc;
      seq.general_level_idc = sps.profile_tier_level.general_level_idc;
      seq.general_tier_flag = sps.profile_tier_level.profile_tier.general_tier_flag;
      seq.intra_period = gop_size;
      seq.intra_idr_period = gop_size;
      seq.ip_period = 1;
      seq.pic_width_in_luma_samples = sps.pic_width_in_luma_samples;
      seq.pic_height_in_luma_samples = sps.pic_height_in_luma_samples;
      seq.seq_fields.bits.chroma_format_idc = sps.chroma_format_idc;
      seq.seq_fields.bits.bit_depth_luma_minus8 = sps.bit_depth_luma_minus8;
      seq.seq_fields.bits.bit_depth_chroma_minus8 = sps.bit_depth_chroma_minus8;
      seq.seq_fields.bits.strong_intra_smoothing_enabled_flag = sps.strong_intra_smoothing_enabled_flag;
      seq.seq_fields.bits.amp_enabled_flag = sps.amp_enabled_flag;
      seq.seq_fields.bits.sample_adaptive_offset_enabled_flag = sps.sample_adaptive_offset_enabled_flag;
      seq.log2_min_luma_coding_block_size_minus3 = sps.log2_min_luma_coding_block_size_minus3;
      seq.log2_diff_max_min_luma_coding_block_size = sps.log2_diff_max_min_luma_coding_block_size;
      seq.log2_min_transform_block_size_minus2 = sps.log2_min_luma_transform_block_size_minus2;
      seq.log2_diff_max_min_transform_block_size = sps.log2_diff_max_min_luma_transform_block_size;
      seq.max_transform_hierarchy_depth_inter = sps.max_transform_hierarchy_depth_inter;
      seq.max_transform_hierarchy_depth_intra = sps.max_transform_hierarchy_depth_intra;
      seq.vui_parameters_present_flag = sps.vui_parameters_present_flag;
      add_buffer(VAEncSequenceParameterBufferType, sizeof(seq), &seq);

      bs.write_vps(vps);
      add_packed_header(VAEncPackedHeaderSequence, bs);
      bs.reset();

      bs.write_sps(sps);
      add_packed_header(VAEncPackedHeaderSequence, bs);
      bs.reset();

      bs.write_pps(pps);
      add_packed_header(VAEncPackedHeaderPicture, bs);
      bs.reset();
   }

   if (is_idr) {
      slice.nal_unit_type = 19; // IDR_W_RADL
      slice.slice_type = 2;
   } else {
      slice.nal_unit_type = enc_params.referenced ? 1 : 0; // TRAIL_R/N
      if (params->temporal_id > 0)
         slice.nal_unit_type = enc_params.referenced ? 3 : 2; // TSA_R/N
      if (enc_params.frame_type == ENC_FRAME_TYPE_I)
         slice.slice_type = 2;
      else if (enc_params.frame_type == ENC_FRAME_TYPE_P)
         slice.slice_type = 1;
   }
   slice.temporal_id = params->temporal_id;
   slice.first_slice_segment_in_pic_flag = 1;
   slice.slice_pic_order_cnt_lsb = pic_order_cnt % (1 << (sps.log2_max_pic_order_cnt_lsb_minus4 + 4));
   slice.slice_qp_delta = params->qp - (pps.init_qp_minus26 + 26);
   if (enc_params.ref_l0_slot != 0xff) {
      std::vector<uint64_t> ref_list0;
      std::vector<uint64_t> ltr_list;
      for (uint32_t i = 0; i < dpb.size(); i++) {
         if (dpb[i].valid && dpb_poc[i] < pic_order_cnt) {
            if (dpb[i].long_term)
               ltr_list.push_back(dpb_poc[i]);
            else
               ref_list0.push_back(dpb_poc[i]);
         }
      }
      std::sort(ltr_list.rbegin(), ltr_list.rend());
      std::sort(ref_list0.rbegin(), ref_list0.rend());
      uint64_t poc = pic_order_cnt;
      for (uint32_t i = 0; i < ref_list0.size(); i++) {
         uint64_t ref_poc = ref_list0[i];
         sps.st_ref_pic_set[0].delta_poc_s0_minus1[i] = poc - ref_poc - 1;
         sps.st_ref_pic_set[0].used_by_curr_pic_s0_flag[i] = ref_poc == dpb_poc[enc_params.ref_l0_slot];
         poc = ref_poc;
      }
      sps.st_ref_pic_set[0].num_negative_pics = ref_list0.size();

      uint32_t max_poc = 1 << (sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
      poc = pic_order_cnt - (pic_order_cnt % max_poc);
      for (uint32_t i = 0; i < ltr_list.size(); i++) {
         uint64_t ltr_poc = ltr_list[i];
         uint32_t ltr_lsb = ltr_poc % max_poc;
         uint32_t ltr_msb = ltr_poc - ltr_lsb;
         slice.poc_lsb_lt[i] = ltr_lsb;
         slice.used_by_curr_pic_lt_flag[i] = ltr_poc == dpb_poc[enc_params.ref_l0_slot];
         slice.delta_poc_msb_present_flag[i] = 1;
         slice.delta_poc_msb_cycle_lt[i] = (poc - ltr_msb) / max_poc;
         poc = ltr_poc;
      }
      slice.num_long_term_pics = ltr_list.size();
   }

   VAEncPictureParameterBufferHEVC pic = {};
   pic.decoded_curr_pic.picture_id = dpb[enc_params.recon_slot].surface;
   pic.decoded_curr_pic.flags = 0;
   pic.coded_buf = task->buffer_id;
   pic.collocated_ref_pic_index = 0xff;
   pic.pic_init_qp = pps.init_qp_minus26 + 26;
   pic.diff_cu_qp_delta_depth = pps.diff_cu_qp_delta_depth;
   pic.pps_cb_qp_offset = pps.pps_cb_qp_offset;
   pic.pps_cr_qp_offset = pps.pps_cr_qp_offset;
   pic.log2_parallel_merge_level_minus2 = pps.log2_parallel_merge_level_minus2;
   pic.num_ref_idx_l0_default_active_minus1 = pps.num_ref_idx_l0_default_active_minus1;
   pic.num_ref_idx_l1_default_active_minus1 = pps.num_ref_idx_l1_default_active_minus1;
   pic.slice_pic_parameter_set_id = slice.slice_pic_parameter_set_id;
   pic.nal_unit_type = slice.nal_unit_type;
   pic.pic_fields.bits.idr_pic_flag = is_idr;
   pic.pic_fields.bits.coding_type = enc_params.frame_type == ENC_FRAME_TYPE_P ? 2 : 1;
   pic.pic_fields.bits.reference_pic_flag = enc_params.referenced;
   pic.pic_fields.bits.sign_data_hiding_enabled_flag = pps.sign_data_hiding_enabled_flag;
   pic.pic_fields.bits.constrained_intra_pred_flag = pps.constrained_intra_pred_flag;
   pic.pic_fields.bits.transform_skip_enabled_flag = pps.transform_skip_enabled_flag;
   pic.pic_fields.bits.cu_qp_delta_enabled_flag = pps.cu_qp_delta_enabled_flag;
   pic.pic_fields.bits.weighted_pred_flag = pps.weighted_pred_flag;
   pic.pic_fields.bits.weighted_bipred_flag = pps.weighted_bipred_flag;
   pic.pic_fields.bits.transquant_bypass_enabled_flag = pps.transquant_bypass_enabled_flag;
   pic.pic_fields.bits.entropy_coding_sync_enabled_flag = pps.entropy_coding_sync_enabled_flag;
   pic.pic_fields.bits.pps_loop_filter_across_slices_enabled_flag = pps.pps_loop_filter_across_slices_enabled_flag;
   pic.pic_fields.bits.no_output_of_prior_pics_flag = slice.no_output_of_prior_pics_flag;
   for (uint32_t i = 0; i < 15; i++) {
      pic.reference_frames[i].picture_id = VA_INVALID_ID;
      pic.reference_frames[i].flags = VA_PICTURE_HEVC_INVALID;
   }
   if (enc_params.ref_l0_slot != 0xff) {
      pic.reference_frames[0].picture_id = dpb[enc_params.ref_l0_slot].surface;
      pic.reference_frames[0].pic_order_cnt = dpb_poc[enc_params.ref_l0_slot];
      if (dpb[enc_params.ref_l0_slot].long_term)
         pic.reference_frames[0].flags = VA_PICTURE_HEVC_LONG_TERM_REFERENCE;
   }
   add_buffer(VAEncPictureParameterBufferType, sizeof(pic), &pic);

   VAEncSliceParameterBufferHEVC sl = {};
   sl.num_ctu_in_slice = (aligned_width / unit_width) * (aligned_height / unit_height);
   sl.slice_type = slice.slice_type;
   sl.slice_pic_parameter_set_id = slice.slice_pic_parameter_set_id;
   sl.num_ref_idx_l0_active_minus1 = slice.num_ref_idx_active_override_flag ? slice.num_ref_idx_l0_active_minus1 : pps.num_ref_idx_l0_default_active_minus1;
   sl.num_ref_idx_l1_active_minus1 = slice.num_ref_idx_active_override_flag ? slice.num_ref_idx_l1_active_minus1 : pps.num_ref_idx_l1_default_active_minus1;
   sl.max_num_merge_cand = 5 - slice.five_minus_max_num_merge_cand;
   sl.slice_qp_delta = slice.slice_qp_delta;
   sl.slice_cb_qp_offset = slice.slice_cb_qp_offset;
   sl.slice_cr_qp_offset = slice.slice_cr_qp_offset;
   sl.slice_beta_offset_div2 = slice.slice_beta_offset_div2;
   sl.slice_tc_offset_div2 = slice.slice_tc_offset_div2;
   sl.slice_fields.bits.last_slice_of_pic_flag = 1;
   sl.slice_fields.bits.slice_sao_luma_flag = slice.slice_sao_luma_flag;
   sl.slice_fields.bits.slice_sao_chroma_flag = slice.slice_sao_chroma_flag;
   sl.slice_fields.bits.num_ref_idx_active_override_flag = slice.num_ref_idx_active_override_flag;
   sl.slice_fields.bits.mvd_l1_zero_flag = slice.mvd_l1_zero_flag;
   sl.slice_fields.bits.cabac_init_flag = slice.cabac_init_flag;
   sl.slice_fields.bits.slice_deblocking_filter_disabled_flag = slice.slice_deblocking_filter_disabled_flag;
   sl.slice_fields.bits.slice_loop_filter_across_slices_enabled_flag = slice.slice_loop_filter_across_slices_enabled_flag;
   for (uint32_t i = 0; i < 15; i++) {
      sl.ref_pic_list0[i].picture_id = VA_INVALID_ID;
      sl.ref_pic_list0[i].flags = VA_PICTURE_HEVC_INVALID;
      sl.ref_pic_list1[i].picture_id = VA_INVALID_ID;
      sl.ref_pic_list1[i].flags = VA_PICTURE_HEVC_INVALID;
   }
   if (enc_params.ref_l0_slot != 0xff) {
      sl.ref_pic_list0[0].picture_id = dpb[enc_params.ref_l0_slot].surface;
      sl.ref_pic_list0[0].pic_order_cnt = dpb_poc[enc_params.ref_l0_slot];
   }
   add_buffer(VAEncSliceParameterBufferType, sizeof(sl), &sl);

   bs.write_slice(slice, sps, pps);
   add_packed_header(VAEncPackedHeaderSlice, bs);
   bs.reset();

   if (!end_encode(params))
      return {};

   pic_order_cnt++;

   return task.release();
}
