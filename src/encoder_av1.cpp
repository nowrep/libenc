#include "encoder_av1.h"
#include "bitstream_av1.h"

encoder_av1::encoder_av1()
   : enc_encoder()
{
   unit_width = 1;
   unit_height = 1;
}

bool encoder_av1::create(const struct enc_encoder_params *params)
{
   switch (params->av1.profile) {
   case ENC_AV1_PROFILE_0:
      profile = VAProfileAV1Profile0;
      break;
   default:
      std::cerr << "Invalid AV1 profile " << params->h264.profile << std::endl;
      return false;
      break;
   }

   std::vector<VAConfigAttrib> attribs;
   if (!create_context(params, attribs))
      return false;

   dpb_ref_idx.resize(dpb.size());

   seq.seq_profile = params->av1.profile;
   seq.seq_level_idx[0] = params->av1.level;
   seq.seq_tier[0] = params->av1.tier;
   seq.timing_info_present_flag = 1;
   auto framerate = get_framerate(params->rc_params[0].frame_rate);
   seq.time_scale = framerate.first;
   seq.num_units_in_display_tick = framerate.second;
   seq.frame_width_bits_minus_1 = logbase2_ceil(aligned_width) - 1;
   seq.frame_height_bits_minus_1 = logbase2_ceil(aligned_height) - 1;
   seq.max_frame_width_minus_1 = aligned_width - 1;
   seq.max_frame_height_minus_1 = aligned_height - 1;
   seq.high_bitdepth = params->bit_depth == 10;

   return true;
}

struct enc_task *encoder_av1::encode_frame(const struct enc_frame_params *params)
{
   auto task = begin_encode(params);
   if (!task)
      return {};

   const bool is_idr = enc_params.frame_type == ENC_FRAME_TYPE_IDR;

   if (is_idr)
      ref_idx_slot = 0;

   dpb_ref_idx[enc_params.recon_slot] = ref_idx_slot;

   bitstream_av1 bs(4);
   bs.write_temporal_delimiter();
   add_packed_header(VAEncPackedHeaderRawData, bs);
   bs.reset();

   if (enc_params.need_sequence_headers) {
      VAEncSequenceParameterBufferAV1 sq = {};
      sq.seq_profile = seq.seq_profile;
      sq.seq_level_idx = seq.seq_level_idx[0];
      sq.seq_tier = seq.seq_tier[0];
      sq.intra_period = gop_size;
      sq.ip_period = 1;
      sq.seq_fields.bits.still_picture = seq.still_picture;
      sq.seq_fields.bits.use_128x128_superblock = seq.use_128x128_superblock;
      sq.seq_fields.bits.enable_filter_intra = seq.enable_filter_intra;
      sq.seq_fields.bits.enable_intra_edge_filter = seq.enable_intra_edge_filter;
      sq.seq_fields.bits.enable_interintra_compound = seq.enable_interintra_compound;
      sq.seq_fields.bits.enable_masked_compound = seq.enable_masked_compound;
      sq.seq_fields.bits.enable_warped_motion = seq.enable_warped_motion;
      sq.seq_fields.bits.enable_dual_filter = seq.enable_dual_filter;
      sq.seq_fields.bits.enable_cdef = seq.enable_cdef;
      sq.seq_fields.bits.bit_depth_minus8 = seq.high_bitdepth ? 2 : 0;
      sq.seq_fields.bits.subsampling_x = 1;
      sq.seq_fields.bits.subsampling_y = 1;
      add_buffer(VAEncSequenceParameterBufferType, sizeof(sq), &sq);

      bs.write_seq(seq);
      add_packed_header(VAEncPackedHeaderSequence, bs);
      bs.reset();
   }

   if (is_idr) {
      frame.frame_type = 0;
      frame.refresh_frame_flags = 0xff;
   } else {
      frame.refresh_frame_flags = enc_params.referenced ? 1 << ref_idx_slot : 0;
      if (enc_params.frame_type == ENC_FRAME_TYPE_I)
         frame.frame_type = 2;
      else if (enc_params.frame_type == ENC_FRAME_TYPE_P)
         frame.frame_type = 1;
   }
   frame.temporal_id = params->temporal_id;
   frame.show_frame = 1;
   frame.uniform_tile_spacing_flag = 1;
   frame.base_q_idx = params->qp * 5;

   VAEncPictureParameterBufferAV1 pic = {};
   pic.frame_width_minus_1 = seq.max_frame_width_minus_1;
   pic.frame_height_minus_1 = seq.max_frame_height_minus_1;
   pic.reconstructed_frame = dpb[enc_params.recon_slot].surface;
   pic.coded_buf = task->buffer_id;
   pic.refresh_frame_flags = frame.refresh_frame_flags;
   pic.picture_flags.bits.frame_type = frame.frame_type;
   pic.picture_flags.bits.error_resilient_mode = frame.error_resilient_mode;
   pic.picture_flags.bits.disable_cdf_update = frame.disable_cdf_update;
   pic.picture_flags.bits.allow_high_precision_mv = frame.allow_high_precision_mv;
   pic.picture_flags.bits.disable_frame_end_update_cdf = frame.disable_frame_end_update_cdf;
   pic.picture_flags.bits.enable_frame_obu = 1;
   pic.picture_flags.bits.long_term_reference = enc_params.long_term;
   pic.picture_flags.bits.disable_frame_recon = !enc_params.referenced;
   pic.temporal_id = frame.temporal_id;
   pic.base_qindex = frame.base_q_idx;
   pic.min_base_qindex = 1;
   pic.max_base_qindex = 255;
   pic.tile_cols = 1;
   pic.tile_rows = 1;
   pic.width_in_sbs_minus_1[0] = align(aligned_width, 64) - 1;
   pic.height_in_sbs_minus_1[0] = align(aligned_height, 64) - 1;
   for (uint32_t i = 0; i < 8; i++)
      pic.reference_frames[i] = VA_INVALID_ID;
   for (uint32_t i = 0; i < 7; i++)
      pic.ref_frame_idx[i] = 0xff;
   if (enc_params.ref_l0_slot != 0xff) {
      for (uint32_t i = 0; i < dpb.size(); i++) {
         if (dpb[i].ok() && i != enc_params.recon_slot)
            pic.reference_frames[dpb_ref_idx[i]] = dpb[i].surface;
      }
      uint32_t ref_idx = dpb_ref_idx[enc_params.ref_l0_slot];
      for (uint32_t i = 0; i < 7; i++) {
         pic.ref_frame_idx[i] = ref_idx;
         frame.ref_frame_idx[i] = ref_idx;
      }
      pic.primary_ref_frame = ref_idx;
      pic.ref_frame_ctrl_l0.fields.search_idx0 = 1;
   }

   auto offsets = bs.write_frame(frame, seq);
   add_packed_header(VAEncPackedHeaderPicture, bs);
   bs.reset();

   pic.bit_offset_qindex = offsets.base_q_idx;
   pic.bit_offset_segmentation = offsets.segmentation_enabled;
   pic.bit_offset_loopfilter_params = offsets.loop_filter_params;
   pic.bit_offset_cdef_params = offsets.cdef_params;
   pic.size_in_bits_cdef_params = offsets.cdef_params_size;
   pic.byte_offset_frame_hdr_obu_size = offsets.obu_size / 8;
   pic.size_in_bits_frame_hdr_obu = offsets.frame_header_end;
   add_buffer(VAEncPictureParameterBufferType, sizeof(pic), &pic);

   if (!end_encode(params))
      return {};

   if (enc_params.referenced) {
      std::vector<bool> ltr_slots(num_refs, false);
      for (uint32_t i = 0; i < dpb.size(); i++) {
         if (dpb[i].ok() && dpb[i].long_term)
            ltr_slots[dpb_ref_idx[i]] = true;
      }
      do {
         ref_idx_slot = (ref_idx_slot + 1) % num_refs;
      } while (ltr_slots[ref_idx_slot]);
   }

   return task.release();
}
