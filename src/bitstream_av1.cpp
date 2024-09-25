#include "bitstream_av1.h"

bitstream_av1::bitstream_av1(uint8_t obu_size_bytes)
   : bitstream()
   , obu_size_bytes(obu_size_bytes)
{
}

void bitstream_av1::write_temporal_delimiter()
{
   write_obu(2, {});
}

void bitstream_av1::write_seq(const seq &seq)
{
   bitstream bs;
   bs.ui(seq.seq_profile, 3);
   bs.ui(seq.still_picture, 1);
   bs.ui(0x0, 1); // reduced_still_picture_header
   bs.ui(seq.timing_info_present_flag, 1);
   if (seq.timing_info_present_flag) {
      bs.ui(seq.num_units_in_display_tick, 32);
      bs.ui(seq.time_scale, 32);
      bs.ui(seq.equal_picture_interval, 1);
      if (seq.equal_picture_interval)
         bs.ue(seq.num_ticks_per_picture_minus_1);
      bs.ui(0x0, 1); // decoder_model_info_present_flag
   }
   bs.ui(0x0, 1); // initial_display_delay_present_flag
   bs.ui(seq.operating_points_cnt_minus_1, 5);
   for (uint32_t i = 0; i <= seq.operating_points_cnt_minus_1; i++) {
      bs.ui(seq.operating_point_idc[i], 12);
      bs.ui(seq.seq_level_idx[i], 5);
      if (seq.seq_level_idx[i] > 7)
         bs.ui(seq.seq_tier[i], 1);
   }
   bs.ui(seq.frame_width_bits_minus_1, 4);
   bs.ui(seq.frame_height_bits_minus_1, 4);
   bs.ui(seq.max_frame_width_minus_1, seq.frame_width_bits_minus_1 + 1);
   bs.ui(seq.max_frame_height_minus_1, seq.frame_height_bits_minus_1 + 1);
   bs.ui(0x0, 1); // frame_id_numbers_present_flag
   bs.ui(seq.use_128x128_superblock, 1);
   bs.ui(seq.enable_filter_intra, 1);
   bs.ui(seq.enable_intra_edge_filter, 1);
   bs.ui(seq.enable_interintra_compound, 1);
   bs.ui(seq.enable_masked_compound, 1);
   bs.ui(seq.enable_warped_motion, 1);
   bs.ui(seq.enable_dual_filter, 1);
   bs.ui(0x0, 1); // enable_order_hint
   bs.ui(0x0, 1); // seq_choose_screen_content_tools
   bs.ui(0x0, 1); // seq_force_screen_content_tools
   bs.ui(0x0, 1); // enable_superres
   bs.ui(seq.enable_cdef, 1);
   bs.ui(0x0, 1); // enable_restoration
   bs.ui(seq.high_bitdepth, 1);
   bs.ui(0x0, 1); // mono_chrome
   bs.ui(seq.color_description_present_flag, 1);
   if (seq.color_description_present_flag) {
      bs.ui(seq.color_primaries, 8);
      bs.ui(seq.transfer_characteristics, 8);
      bs.ui(seq.matrix_coefficients, 8);
   }
   if (seq.color_primaries != 1 || seq.transfer_characteristics != 13 || seq.matrix_coefficients != 0) {
      bs.ui(seq.color_range, 1);
      bs.ui(seq.chroma_sample_position, 2);
   }
   bs.ui(0x0, 1); // separate_uv_delta_q
   bs.ui(0x0, 1); // film_grain_params_present
   bs.trailing_bits();

   write_obu(1, bs);
}

bitstream_av1::frame_offsets bitstream_av1::write_frame(const frame &frame, const seq &seq)
{
   bool error_resilient_mode = frame.error_resilient_mode;
   bool frame_is_intra = frame.frame_type == 0 || frame.frame_type == 2;
   bool force_integer_mv = frame_is_intra;

   frame_offsets off = {};

   bitstream bs;
   bs.ui(0x0, 1); // show_existing_frame
   bs.ui(frame.frame_type, 2);
   bs.ui(frame.show_frame, 1);
   if (!frame.show_frame)
      bs.ui(frame.showable_frame, 1);
   if (frame.frame_type == 3 || (frame.frame_type == 0 && frame.show_frame))
      error_resilient_mode = true;
   else
      bs.ui(error_resilient_mode, 1);
   bs.ui(frame.disable_cdf_update, 1);
   if (frame.frame_type != 3)
      bs.ui(0x0, 1); // frame_size_override_flag
   if (!frame_is_intra && !error_resilient_mode)
      bs.ui(frame.primary_ref_frame, 3);
   if (frame.frame_type != 3 && (frame.frame_type != 0 || !frame.show_frame))
      bs.ui(frame.refresh_frame_flags, 8);
   if (frame_is_intra) {
      bs.ui(0x0, 1); // render_and_frame_size_different
   } else {
      for (uint32_t i = 0; i < 7; i++)
         bs.ui(frame.ref_frame_idx[i], 3);
      bs.ui(0x0, 1); // render_and_frame_size_different
      if (!force_integer_mv)
         bs.ui(frame.allow_high_precision_mv, 1);
      bs.ui(frame.is_filter_switchable, 1);
      if (frame.is_filter_switchable != 1)
         bs.ui(frame.interpolation_filter, 2);
      bs.ui(frame.is_motion_mode_switchable, 1);
   }
   if (!frame.disable_cdf_update)
      bs.ui(frame.disable_frame_end_update_cdf, 1);
   bs.ui(frame.uniform_tile_spacing_flag, 1);
   off.base_q_idx = bs.size_bits();
   bs.ui(frame.base_q_idx, 8);
   bs.ui(0x0, 1); // using_qmatrix
   off.segmentation_enabled = bs.size_bits();
   bs.ui(0x0, 1); // segmentation_enabled
   if (frame.base_q_idx > 0)
      bs.ui(0x0, 1); // delta_q_present
   off.loop_filter_params = bs.size_bits();
   bs.ui(0x0, 6); // loop_filter_level[0]
   bs.ui(0x0, 6); // loop_filter_level[1]
   bs.ui(0x0, 3); // loop_filter_sharpness
   bs.ui(0x0, 1); // loop_filter_delta_enabled
   if (seq.enable_cdef) {
      off.cdef_params = bs.size_bits();
      bs.ui(0x0, 2); // cdef_damping_minus_3
      bs.ui(0x0, 1); // cdef_bits
      bs.ui(0x0, 4); // cdef_y_pri_strength[0]
      bs.ui(0x0, 2); // cdef_y_sec_strength[0]
      bs.ui(0x0, 4); // cdef_uv_pri_strength[0]
      bs.ui(0x0, 2); // cdef_uv_sec_strength[0]
      off.cdef_params_size = bs.size_bits() - off.cdef_params;
   }
   bs.ui(0x0, 1); // tx_mode_select
   if (!frame_is_intra)
      bs.ui(0x0, 1); // reference_select
   if (!frame_is_intra && !error_resilient_mode && seq.enable_warped_motion)
      bs.ui(0x0, 1); // allow_warped_motion
   bs.ui(0x0, 1); // reduced_tx_set
   if (!frame_is_intra) {
      for (uint32_t i = 1; i <= 7; i++)
         bs.ui(0x0, 1); // is_global
   }
   off.frame_header_end = bs.size_bits();
   bs.byte_align();

   off.obu_size = write_obu(6, bs, obu_size_bytes);

   const uint32_t obu_offset = off.obu_size + (obu_size_bytes * 8);
   off.base_q_idx += obu_offset;
   off.segmentation_enabled += obu_offset;
   off.loop_filter_params += obu_offset;
   off.cdef_params += obu_offset;
   off.frame_header_end += obu_offset;

   return off;
}

uint32_t bitstream_av1::write_obu(uint8_t type, const bitstream &bs, uint8_t size_bytes)
{
   ui(0x0, 1); // obu_forbidden_bit
   ui(type, 4);
   ui(0x0, 1); // obu_extension_flag
   ui(0x1, 1); // obu_has_size_field
   ui(0x0, 1); // obu_reserved_1bit

   leb128(bs.size(), size_bytes);
   for (uint32_t i = 0; i < bs.size(); i++)
      ui(bs.data()[i], 8);

   return 8;
}
