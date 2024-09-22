#include "encoder_h264.h"
#include "bitstream_h264.h"

encoder_h264::encoder_h264()
   : enc_encoder()
{
   unit_width = 16;
   unit_height = 16;
}

bool encoder_h264::create(const struct enc_encoder_params *params)
{
   std::vector<VAConfigAttrib> attribs;

   VAConfigAttrib attrib;
   attrib.type = VAConfigAttribEncPackedHeaders;
   attrib.value = VA_ENC_PACKED_HEADER_SEQUENCE | VA_ENC_PACKED_HEADER_PICTURE | VA_ENC_PACKED_HEADER_SLICE | VA_ENC_PACKED_HEADER_RAW_DATA;
   attribs.push_back(attrib);

   VAProfile profile = VAProfileNone;
   switch (params->h264.profile) {
   case ENC_H264_PROFILE_CONSTRAINED_BASELINE:
      profile = VAProfileH264ConstrainedBaseline;
      break;
   case ENC_H264_PROFILE_MAIN:
      profile = VAProfileH264Main;
      break;
   case ENC_H264_PROFILE_HIGH:
      profile = VAProfileH264High;
      break;
   default:
      std::cerr << "Invalid H264 profile " << params->h264.profile << std::endl;
      return false;
      break;
   }

   if (!create_context(params, profile, attribs))
      return false;

   dpb_poc.resize(dpb.size());

   sps.profile_idc = params->h264.profile;
   sps.constraint_set_flags = 0;
   sps.level_idc = params->h264.level_idc;
   sps.chroma_format_idc = 1,
   sps.bit_depth_luma_minus8 = params->bit_depth - 8;
   sps.bit_depth_chroma_minus8 = params->bit_depth - 8;
   sps.log2_max_frame_num_minus4 = 4;
   sps.pic_order_cnt_type = 2;
   sps.log2_max_pic_order_cnt_lsb_minus4 = 4;
   sps.max_num_ref_frames = num_refs;
   sps.pic_width_in_mbs_minus1 = (aligned_width / unit_width) - 1;
   sps.pic_height_in_map_units_minus1 = (aligned_height / unit_height) - 1;
   sps.frame_mbs_only_flag = 1;
   sps.direct_8x8_inference_flag = 1;
   if (aligned_width != params->width || aligned_height != params->height) {
      sps.frame_cropping_flag = 1;
      sps.frame_crop_left_offset = 0;
      sps.frame_crop_right_offset = (aligned_width - params->width) / 2;
      sps.frame_crop_top_offset = 0;
      sps.frame_crop_bottom_offset = (aligned_height - params->height) / 2;
   }
   sps.vui_parameters_present_flag = 1;
   sps.video_signal_type_present_flag = 1;
   sps.video_format = 5;
   sps.timing_info_present_flag = 1;
   auto framerate = get_framerate(params->rc_params[0].frame_rate);
   sps.time_scale = framerate.first * 2;
   sps.num_units_in_tick = framerate.second;

   pps.entropy_coding_mode_flag = 1;
   pps.num_ref_idx_l0_default_active_minus1 = 0;
   pps.weighted_pred_flag = 0;
   pps.weighted_bipred_idc = 0;
   pps.pic_init_qp_minus26 = 0;
   pps.pic_init_qs_minus26 = 0;
   pps.chroma_qp_index_offset = 0;
   pps.deblocking_filter_control_present_flag = 1;
   pps.constrained_intra_pred_flag = 0;
   pps.redundant_pic_cnt_present_flag = 0;
   pps.transform_8x8_mode_flag = 1;
   pps.second_chroma_qp_index_offset = 0;

   return true;
}

struct enc_task *encoder_h264::encode_frame(const struct enc_frame_params *params)
{
   auto task = begin_encode(params);
   if (!task)
      return {};

   const bool is_idr = enc_params.frame_type == ENC_FRAME_TYPE_IDR;

   if (is_idr)
      pic_order_cnt = 0;

   dpb_poc[enc_params.recon_slot] = pic_order_cnt;

   bitstream_h264 bs;

   if (num_layers > 1) {
      bitstream_h264::prefix pfx;
      pfx.svc_extension_flag = 1;
      pfx.temporal_id = is_idr ? 0 : params->temporal_id;
      bs.write_prefix(pfx);
      add_packed_header(VAEncPackedHeaderRawData, bs);
      bs.reset();
   }

   if (enc_params.need_sequence_headers) {
      VAEncSequenceParameterBufferH264 seq = {};
      seq.seq_parameter_set_id = sps.seq_parameter_set_id;
      seq.level_idc = sps.level_idc;
      seq.intra_period = gop_size;
      seq.intra_idr_period = gop_size;
      seq.ip_period = 1;
      seq.max_num_ref_frames = sps.max_num_ref_frames;
      seq.picture_width_in_mbs = sps.pic_width_in_mbs_minus1 + 1;
      seq.picture_height_in_mbs = sps.pic_height_in_map_units_minus1 + 1;
      seq.seq_fields.bits.chroma_format_idc = sps.chroma_format_idc;
      seq.seq_fields.bits.frame_mbs_only_flag = sps.frame_mbs_only_flag;
      seq.seq_fields.bits.mb_adaptive_frame_field_flag = sps.mb_adaptive_frame_field_flag;
      seq.seq_fields.bits.direct_8x8_inference_flag = sps.direct_8x8_inference_flag;
      seq.seq_fields.bits.log2_max_frame_num_minus4 = sps.log2_max_frame_num_minus4;
      seq.seq_fields.bits.pic_order_cnt_type = sps.pic_order_cnt_type;
      seq.seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = sps.log2_max_pic_order_cnt_lsb_minus4;
      seq.bit_depth_luma_minus8 = sps.bit_depth_luma_minus8;
      seq.bit_depth_chroma_minus8 = sps.bit_depth_chroma_minus8;
      seq.frame_cropping_flag = sps.frame_cropping_flag;
      seq.frame_crop_left_offset = sps.frame_crop_left_offset;
      seq.frame_crop_right_offset = sps.frame_crop_right_offset;
      seq.frame_crop_top_offset = sps.frame_crop_top_offset;
      seq.frame_crop_bottom_offset = sps.frame_crop_bottom_offset;
      seq.vui_parameters_present_flag = sps.vui_parameters_present_flag;
      seq.vui_fields.bits.aspect_ratio_info_present_flag = sps.aspect_ratio_info_present_flag;
      seq.vui_fields.bits.timing_info_present_flag = sps.timing_info_present_flag;
      seq.vui_fields.bits.bitstream_restriction_flag = sps.bitstream_restriction_flag;
      seq.vui_fields.bits.log2_max_mv_length_horizontal = sps.log2_max_mv_length_horizontal;
      seq.vui_fields.bits.log2_max_mv_length_vertical = sps.log2_max_mv_length_vertical;
      seq.vui_fields.bits.fixed_frame_rate_flag = sps.fixed_frame_rate_flag;
      seq.vui_fields.bits.motion_vectors_over_pic_boundaries_flag = sps.motion_vectors_over_pic_boundaries_flag;
      seq.aspect_ratio_idc = sps.aspect_ratio_idc;
      seq.sar_width = sps.sar_width;
      seq.sar_height = sps.sar_height;
      seq.num_units_in_tick = sps.num_units_in_tick;
      seq.time_scale = sps.time_scale;
      add_buffer(VAEncSequenceParameterBufferType, sizeof(seq), &seq);

      if (enc_params.is_recovery_point) {
         bitstream_h264::sei_recovery_point srp = {};
         srp.exact_match_flag = 1;
         bs.write_sei_recovery_point(srp);
         add_packed_header(VAEncPackedHeaderRawData, bs);
         bs.reset();
      }

      if (num_layers > 1) {
         bitstream_h264::sei_scalability_info ssi = {};
         ssi.num_layers_minus1 = num_layers - 1;
         bs.write_sei_scalability_info(ssi);
         add_packed_header(VAEncPackedHeaderRawData, bs);
         bs.reset();
      }

      bs.write_sps(sps);
      add_packed_header(VAEncPackedHeaderSequence, bs);
      bs.reset();

      bs.write_pps(pps);
      add_packed_header(VAEncPackedHeaderPicture, bs);
      bs.reset();
   }

   if (is_idr) {
      slice.nal_unit_type = 5;
      slice.nal_ref_idc = 3;
      slice.slice_type = 2;
      slice.idr_pic_id = idr_pic_id++;
   } else {
      slice.nal_unit_type = 1;
      slice.nal_ref_idc = !!enc_params.referenced;
      if (enc_params.frame_type == ENC_FRAME_TYPE_I)
         slice.slice_type = 2;
      else if (enc_params.frame_type == ENC_FRAME_TYPE_P)
         slice.slice_type = 0;
   }
   slice.first_mb_in_slice = 0;
   slice.frame_num = enc_params.frame_id % (1 << (sps.log2_max_frame_num_minus4 + 4));
   slice.pic_order_cnt_lsb = pic_order_cnt % (1 << (sps.log2_max_pic_order_cnt_lsb_minus4 + 4));
   slice.slice_qp_delta = params->qp - (pps.pic_init_qp_minus26 + 26);
   slice.ref_pic_list_modification_flag_l0 = 0;
   if (enc_params.ref_l0_slot != 0xff && enc_params.frame_id - dpb[enc_params.ref_l0_slot].frame_id > 1) {
      slice.ref_pic_list_modification_flag_l0 = 1;
      slice.num_ref_list0_mod = 1;
      slice.ref_list0_mod[0].modification_of_pic_nums_idc = 0;
      slice.ref_list0_mod[0].abs_diff_pic_num_minus1 = enc_params.frame_id - dpb[enc_params.ref_l0_slot].frame_id - 1;
   }
   slice.adaptive_ref_pic_marking_mode_flag = 0;
   /*
   if (params->num_invalidate_refs) {
      slice.adaptive_ref_pic_marking_mode_flag = 1;
      slice.num_mmco_op = params->num_invalidate_refs;
      for (uint32_t i = 0; i < params->num_invalidate_refs; i++) {
         if (enc_params.frame_id > params->invalidate_refs[i]) {
            slice.mmco_op[i].memory_management_control_operation = 1;
            slice.mmco_op[i].difference_of_pic_nums_minus1 = enc_params.frame_id - params->invalidate_refs[i] - 1;
         }
      }
   }
   */

   VAEncPictureParameterBufferH264 pic = {};
   pic.CurrPic.picture_id = dpb[enc_params.recon_slot].surface;
   pic.coded_buf = task->buffer_id;
   pic.pic_parameter_set_id = pps.pic_parameter_set_id;
   pic.seq_parameter_set_id = pps.seq_parameter_set_id;
   pic.frame_num = slice.frame_num;
   pic.pic_init_qp = pps.pic_init_qp_minus26 + 26;
   pic.num_ref_idx_l0_active_minus1 = pps.num_ref_idx_l0_default_active_minus1;
   pic.num_ref_idx_l1_active_minus1 = pps.num_ref_idx_l1_default_active_minus1;
   pic.chroma_qp_index_offset = pps.chroma_qp_index_offset;
   pic.second_chroma_qp_index_offset = pps.second_chroma_qp_index_offset;
   pic.pic_fields.bits.idr_pic_flag = is_idr;
   pic.pic_fields.bits.reference_pic_flag = !!slice.nal_ref_idc;
   pic.pic_fields.bits.entropy_coding_mode_flag = pps.entropy_coding_mode_flag;
   pic.pic_fields.bits.weighted_pred_flag = pps.weighted_pred_flag;
   pic.pic_fields.bits.weighted_bipred_idc = pps.weighted_bipred_idc;
   pic.pic_fields.bits.constrained_intra_pred_flag = pps.constrained_intra_pred_flag;
   pic.pic_fields.bits.transform_8x8_mode_flag = pps.transform_8x8_mode_flag;
   pic.pic_fields.bits.deblocking_filter_control_present_flag = pps.deblocking_filter_control_present_flag;
   pic.pic_fields.bits.redundant_pic_cnt_present_flag = pps.redundant_pic_cnt_present_flag;
   for (uint32_t i = 0; i < 16; i++) {
      pic.ReferenceFrames[i].picture_id = VA_INVALID_ID;
      pic.ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;
   }
   if (enc_params.ref_l0_slot != 0xff) {
      pic.ReferenceFrames[0].picture_id = dpb[enc_params.ref_l0_slot].surface;
      pic.ReferenceFrames[0].frame_idx = dpb[enc_params.ref_l0_slot].frame_id;
      pic.ReferenceFrames[0].TopFieldOrderCnt = dpb_poc[enc_params.ref_l0_slot];
      pic.ReferenceFrames[0].BottomFieldOrderCnt = dpb_poc[enc_params.ref_l0_slot];
   }
   add_buffer(VAEncPictureParameterBufferType, sizeof(pic), &pic);

   VAEncSliceParameterBufferH264 sl = {};
   sl.num_macroblocks = (aligned_width / unit_width) * (aligned_height / unit_height);
   sl.macroblock_info = VA_INVALID_ID;
   sl.slice_type = slice.slice_type;
   sl.pic_parameter_set_id = slice.pic_parameter_set_id;
   sl.idr_pic_id = slice.idr_pic_id;
   sl.pic_order_cnt_lsb = slice.pic_order_cnt_lsb;
   sl.direct_spatial_mv_pred_flag = slice.direct_spatial_mv_pred_flag;
   sl.num_ref_idx_active_override_flag = slice.num_ref_idx_active_override_flag;
   sl.num_ref_idx_l0_active_minus1 = slice.num_ref_idx_l0_active_minus1;
   sl.num_ref_idx_l1_active_minus1 = slice.num_ref_idx_l1_active_minus1;
   sl.cabac_init_idc = slice.cabac_init_idc;
   sl.slice_qp_delta = slice.slice_qp_delta;
   sl.disable_deblocking_filter_idc = slice.disable_deblocking_filter_idc;
   sl.slice_alpha_c0_offset_div2 = slice.slice_alpha_c0_offset_div2;
   sl.slice_beta_offset_div2 = slice.slice_beta_offset_div2;
   for (uint32_t i = 0; i < 32; i++) {
      sl.RefPicList0[i].picture_id = VA_INVALID_ID;
      sl.RefPicList0[i].flags = VA_PICTURE_H264_INVALID;
      sl.RefPicList1[i].picture_id = VA_INVALID_ID;
      sl.RefPicList1[i].flags = VA_PICTURE_H264_INVALID;
   }
   if (enc_params.ref_l0_slot != 0xff) {
      sl.RefPicList0[0].picture_id = dpb[enc_params.ref_l0_slot].surface;
      sl.RefPicList0[0].frame_idx = dpb[enc_params.ref_l0_slot].frame_id;
      sl.RefPicList0[0].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
      sl.RefPicList0[0].TopFieldOrderCnt = dpb_poc[enc_params.ref_l0_slot];
      sl.RefPicList0[0].BottomFieldOrderCnt = dpb_poc[enc_params.ref_l0_slot];
   }
   add_buffer(VAEncSliceParameterBufferType, sizeof(sl), &sl);

   bs.write_slice(slice, sps, pps);
   add_packed_header(VAEncPackedHeaderSlice, bs);
   bs.reset();

   if (!end_encode(params))
      return {};

   pic_order_cnt += 2;
   if (pic_order_cnt > INT32_MAX)
      pic_order_cnt = 0;

   return task.release();
}
