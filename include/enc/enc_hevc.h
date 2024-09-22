#pragma once

enum enc_hevc_profile {
   ENC_HEVC_PROFILE_MAIN = 1,
   ENC_HEVC_PROFILE_MAIN_10 = 2,
};

struct enc_hevc_encoder_params {
   // Profile.
   enum enc_hevc_profile profile;
};
