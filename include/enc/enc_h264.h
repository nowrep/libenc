#pragma once

enum enc_h264_profile {
   ENC_H264_PROFILE_CONSTRAINED_BASELINE = 75,
   ENC_H264_PROFILE_MAIN = 77,
   ENC_H264_PROFILE_HIGH = 100,
};

struct enc_h264_encoder_params {
   // Profile.
   enum enc_h264_profile profile;

   // Matches H264 spec.
   uint8_t level_idc;
};
