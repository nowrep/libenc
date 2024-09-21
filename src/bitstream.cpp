#include "bitstream.h"

bitstream::bitstream()
{
}

bitstream::~bitstream()
{
}

uint32_t bitstream::size() const
{
   return buf.size();
}

const uint8_t *bitstream::data() const
{
   return buf.data();
}

void bitstream::reset()
{
   buf.clear();
   shifter = 0;
   bits_in_shifter = 0;
}

void bitstream::ue(uint32_t value)
{
    uint32_t bits = 0;
    uint32_t tmp = ++value;
    while (tmp) {
        tmp >>= 1;
        bits++;
    }
    ui(0, bits - 1);
    ui(value, bits);
}

void bitstream::se(int32_t value)
{
    if (value <= 0)
        ue(-2 * value);
    else
        ue(2 * value - 1);
}

void bitstream::ui(uint32_t value, uint8_t bits)
{
   unsigned int bits_to_pack = 0;

   while (bits > 0) {
      uint32_t value_to_pack = value & (0xffffffff >> (32 - bits));
      bits_to_pack = bits > (32 - bits_in_shifter) ? (32 - bits_in_shifter) : bits;

      if (bits_to_pack < bits)
         value_to_pack = value_to_pack >> (bits - bits_to_pack);

      shifter |= value_to_pack << (32 - bits_in_shifter - bits_to_pack);
      bits -= bits_to_pack;
      bits_in_shifter += bits_to_pack;

      while (bits_in_shifter >= 8) {
         uint8_t output_byte = shifter >> 24;
         shifter <<= 8;
         buf.push_back(output_byte);
         bits_in_shifter -= 8;
      }
   }
}

void bitstream::byte_align()
{
   uint32_t num_padding_zeros = (32 - bits_in_shifter) % 8;
   if (num_padding_zeros > 0)
      ui(0x0, num_padding_zeros);
}

void bitstream::trailing_bits()
{
   ui(0x1, 1);
   byte_align();
}
