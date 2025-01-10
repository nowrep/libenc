#include "encoder_av1.h"
#include "bitstream_av1.h"

#include <cstring>

encoder_av1::encoder_av1()
   : enc_encoder()
{
   unit_size = 64;
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

   coded_width = params->width;
   coded_height = params->height;

   std::vector<VAConfigAttrib> attribs;
   if (!create_context(params, attribs))
      return false;

   features1.value = get_config_attrib(VAConfigAttribEncAV1);
   features2.value = get_config_attrib(VAConfigAttribEncAV1Ext1);
   features3.value = get_config_attrib(VAConfigAttribEncAV1Ext2);

   dpb_ref_idx.resize(dpb.size());

   seq.seq_profile = params->av1.profile;
   seq.timing_info_present_flag = 1;
   auto framerate = get_framerate(params->rc_params[0].frame_rate);
   seq.time_scale = framerate.first;
   seq.num_units_in_display_tick = framerate.second;
   seq.operating_points_cnt_minus_1 = num_layers - 1;
   for (uint32_t i = 0; i < num_layers; ++i) {
      if (num_layers > 1)
         seq.operating_point_idc[i] = ((1 << (num_layers - i)) - 1) | (1 << 8);
      seq.seq_level_idx[i] = params->av1.level;
      seq.seq_tier[i] = params->av1.tier;
   }
   seq.frame_width_bits_minus_1 = logbase2(coded_width);
   seq.frame_height_bits_minus_1 = logbase2(coded_height);
   seq.max_frame_width_minus_1 = coded_width - 1;
   seq.max_frame_height_minus_1 = coded_height - 1;
   seq.high_bitdepth = params->bit_depth == 10;
   seq.chroma_sample_position = 1;

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

   bitstream_av1 bs(features3.bits.obu_size_bytes_minus1 + 1);
   uint32_t bs_offset = 0;

   bs.write_temporal_delimiter();
   add_packed_header(VAEncPackedHeaderRawData, bs);
   bs_offset += bs.size_bits();
   bs.reset();

   if (enc_params.need_sequence_headers) {
      VAEncSequenceParameterBufferAV1 sq = {};
      sq.seq_profile = seq.seq_profile;
      sq.seq_level_idx = seq.seq_level_idx[0];
      sq.seq_tier = seq.seq_tier[0];
      sq.intra_period = gop_size;
      sq.ip_period = 1;
      sq.bits_per_second = initial_bit_rate;
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
      bs_offset += bs.size_bits();
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
   frame.obu_extension_flag = num_layers > 1;
   frame.temporal_id = params->temporal_id;
   frame.show_frame = 1;
   frame.primary_ref_frame = enc_params.ref_l0_slot != 0xff ? 0 : 7;
   frame.uniform_tile_spacing_flag = 1;
   frame.base_q_idx = params->qp;
   frame.tx_mode_select = (features3.bits.tx_mode_support & 0x4) ? 1 : 0;
   frame.reduced_tx_set = 1;

   VAEncPictureParameterBufferAV1 pic = {};
   pic.frame_width_minus_1 = seq.max_frame_width_minus_1;
   pic.frame_height_minus_1 = seq.max_frame_height_minus_1;
   pic.reconstructed_frame = dpb[enc_params.recon_slot].surface;
   pic.primary_ref_frame = frame.primary_ref_frame;
   pic.coded_buf = task->buffer_id;
   pic.refresh_frame_flags = frame.refresh_frame_flags;
   pic.picture_flags.bits.frame_type = frame.frame_type;
   pic.picture_flags.bits.error_resilient_mode = frame.error_resilient_mode;
   pic.picture_flags.bits.disable_cdf_update = frame.disable_cdf_update;
   pic.picture_flags.bits.allow_high_precision_mv = frame.allow_high_precision_mv;
   pic.picture_flags.bits.disable_frame_end_update_cdf = frame.disable_frame_end_update_cdf;
   pic.picture_flags.bits.reduced_tx_set = frame.reduced_tx_set;
   pic.picture_flags.bits.enable_frame_obu = 1;
   pic.picture_flags.bits.long_term_reference = enc_params.long_term;
   pic.picture_flags.bits.disable_frame_recon = !enc_params.referenced;
   pic.temporal_id = frame.temporal_id;
   pic.base_qindex = frame.base_q_idx;
   pic.min_base_qindex = 1;
   pic.max_base_qindex = 255;
   pic.mode_control_flags.bits.tx_mode = frame.tx_mode_select ? 2 : 0;
   pic.tile_cols = 1;
   pic.tile_rows = 1;
   pic.width_in_sbs_minus_1[0] = aligned_width / 64 - 1;
   pic.height_in_sbs_minus_1[0] = aligned_height / 64 - 1;
   pic.tile_group_obu_hdr_info.bits.obu_has_size_field = 1;
   for (uint32_t i = 0; i < 8; i++)
      pic.reference_frames[i] = VA_INVALID_ID;
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
   pic.byte_offset_frame_hdr_obu_size = (bs_offset + offsets.obu_size) / 8;
   pic.size_in_bits_frame_hdr_obu = offsets.frame_header_end;
   add_buffer(VAEncPictureParameterBufferType, sizeof(pic), &pic);

   VAEncTileGroupBufferAV1 tile = {};
   add_buffer(VAEncSliceParameterBufferType, sizeof(tile), &tile);

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

uint32_t encoder_av1::write_sps(uint8_t *buf, uint32_t buf_size)
{
   bitstream_av1 bs(features3.bits.obu_size_bytes_minus1 + 1);
   bs.write_seq(seq);
   if (bs.size() > buf_size)
      return 0;
   std::memcpy(buf, bs.data(), bs.size());
   return bs.size();
}
