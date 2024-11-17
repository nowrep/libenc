# libenc

Video encode library using VAAPI.

## Features

Main focus of the library is low latency encoding, so features like B-frames are not supported.

* H264/HEVC/AV1
* Full control of references (eg. can use hierarchical P coding)
* Reference frame invalidation
* Intra refresh
* Dynamic rate control change
* Format and color conversion
* SVC Temporal with rate control layers
* Multiple slices

Tested with Mesa and Intel drivers. Mesa requires >= 24.3, older versions may not work.

## Building

```sh
meson setup build
cd build
ninja
```

## Sample

```c
struct enc_dev *dev = enc_dev_create((struct enc_dev_params){
   .device_path = "/dev/dri/renderD128"
});

struct enc_rate_control_params rc_params = {
   .frame_rate = 60,
};

struct enc_encoder *enc = enc_encoder_create((struct enc_encoder_params){
   .dev = dev,
   .codec = ENC_CODEC_H264,
   .width = 1280,
   .height = 720,
   .gop_size = 30,
   .rc_mode = ENC_RATE_CONTROL_MODE_CQP,
   .num_rc_layers = 1,
   .rc_params = &rc_params,
});

struct enc_surface *surf = enc_surface_create((struct enc_surface_params){
   .dev = dev,
   .format = ENC_FORMAT_NV12,
   .width = 1280,
   .height = 720,
});

struct enc_task *task = enc_encoder_encode_frame(enc, (struct enc_frame_params){
   .surface = surf,
   .qp = 20,
});

enc_task_wait(task, UINT64_MAX);

uint32_t size = 0;
uint8_t *data = NULL;
while (enc_task_get_bitstream(task, &size, &data))
   fwrite(data, size, 1, file);
```

More complex example can be found in `app` directory.

## TODO

* Quality setting
* Query capabilities
