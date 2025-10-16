#include "md5_helper.hpp"

#include <array>
#include <cstring>
#include <string>

namespace fb_ops_test {
namespace {

struct Md5Context {
  std::array<std::uint32_t, 4> state{{0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u}};
  std::array<std::uint8_t, 64> buffer{};
  std::uint64_t bitCount = 0;
};

constexpr std::uint32_t F(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  return (x & y) | (~x & z);
}

constexpr std::uint32_t G(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  return (x & z) | (y & ~z);
}

constexpr std::uint32_t H(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  return x ^ y ^ z;
}

constexpr std::uint32_t I(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  return y ^ (x | ~z);
}

constexpr std::uint32_t rotateLeft(std::uint32_t value, std::uint32_t shift) {
  return (value << shift) | (value >> (32u - shift));
}

void transform(std::array<std::uint32_t, 4>& state, const std::uint8_t block[64]) {
  static constexpr std::uint32_t S11 = 7;
  static constexpr std::uint32_t S12 = 12;
  static constexpr std::uint32_t S13 = 17;
  static constexpr std::uint32_t S14 = 22;
  static constexpr std::uint32_t S21 = 5;
  static constexpr std::uint32_t S22 = 9;
  static constexpr std::uint32_t S23 = 14;
  static constexpr std::uint32_t S24 = 20;
  static constexpr std::uint32_t S31 = 4;
  static constexpr std::uint32_t S32 = 11;
  static constexpr std::uint32_t S33 = 16;
  static constexpr std::uint32_t S34 = 23;
  static constexpr std::uint32_t S41 = 6;
  static constexpr std::uint32_t S42 = 10;
  static constexpr std::uint32_t S43 = 15;
  static constexpr std::uint32_t S44 = 21;

  std::uint32_t a = state[0];
  std::uint32_t b = state[1];
  std::uint32_t c = state[2];
  std::uint32_t d = state[3];

  std::uint32_t x[16];
  for (int i = 0; i < 16; ++i) {
    const int offset = i * 4;
    x[i] = static_cast<std::uint32_t>(block[offset]) |
           (static_cast<std::uint32_t>(block[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(block[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(block[offset + 3]) << 24);
  }

#define FF(a, b, c, d, xk, s, ac) \
  { \
    (a) += F((b), (c), (d)) + (xk) + static_cast<std::uint32_t>(ac); \
    (a) = rotateLeft((a), (s)); \
    (a) += (b); \
  }

#define GG(a, b, c, d, xk, s, ac) \
  { \
    (a) += G((b), (c), (d)) + (xk) + static_cast<std::uint32_t>(ac); \
    (a) = rotateLeft((a), (s)); \
    (a) += (b); \
  }

#define HH(a, b, c, d, xk, s, ac) \
  { \
    (a) += H((b), (c), (d)) + (xk) + static_cast<std::uint32_t>(ac); \
    (a) = rotateLeft((a), (s)); \
    (a) += (b); \
  }

#define II(a, b, c, d, xk, s, ac) \
  { \
    (a) += I((b), (c), (d)) + (xk) + static_cast<std::uint32_t>(ac); \
    (a) = rotateLeft((a), (s)); \
    (a) += (b); \
  }

  // Round 1
  FF(a, b, c, d, x[0], S11, 0xd76aa478);  // 1
  FF(d, a, b, c, x[1], S12, 0xe8c7b756);  // 2
  FF(c, d, a, b, x[2], S13, 0x242070db);  // 3
  FF(b, c, d, a, x[3], S14, 0xc1bdceee);  // 4
  FF(a, b, c, d, x[4], S11, 0xf57c0faf);  // 5
  FF(d, a, b, c, x[5], S12, 0x4787c62a);  // 6
  FF(c, d, a, b, x[6], S13, 0xa8304613);  // 7
  FF(b, c, d, a, x[7], S14, 0xfd469501);  // 8
  FF(a, b, c, d, x[8], S11, 0x698098d8);  // 9
  FF(d, a, b, c, x[9], S12, 0x8b44f7af);  // 10
  FF(c, d, a, b, x[10], S13, 0xffff5bb1); // 11
  FF(b, c, d, a, x[11], S14, 0x895cd7be); // 12
  FF(a, b, c, d, x[12], S11, 0x6b901122); // 13
  FF(d, a, b, c, x[13], S12, 0xfd987193); // 14
  FF(c, d, a, b, x[14], S13, 0xa679438e); // 15
  FF(b, c, d, a, x[15], S14, 0x49b40821); // 16

  // Round 2
  GG(a, b, c, d, x[1], S21, 0xf61e2562);  // 17
  GG(d, a, b, c, x[6], S22, 0xc040b340);  // 18
  GG(c, d, a, b, x[11], S23, 0x265e5a51); // 19
  GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);  // 20
  GG(a, b, c, d, x[5], S21, 0xd62f105d);  // 21
  GG(d, a, b, c, x[10], S22, 0x02441453); // 22
  GG(c, d, a, b, x[15], S23, 0xd8a1e681); // 23
  GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);  // 24
  GG(a, b, c, d, x[9], S21, 0x21e1cde6);  // 25
  GG(d, a, b, c, x[14], S22, 0xc33707d6); // 26
  GG(c, d, a, b, x[3], S23, 0xf4d50d87);  // 27
  GG(b, c, d, a, x[8], S24, 0x455a14ed);  // 28
  GG(a, b, c, d, x[13], S21, 0xa9e3e905); // 29
  GG(d, a, b, c, x[2], S22, 0xfcefa3f8);  // 30
  GG(c, d, a, b, x[7], S23, 0x676f02d9);  // 31
  GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); // 32

  // Round 3
  HH(a, b, c, d, x[5], S31, 0xfffa3942);  // 33
  HH(d, a, b, c, x[8], S32, 0x8771f681);  // 34
  HH(c, d, a, b, x[11], S33, 0x6d9d6122); // 35
  HH(b, c, d, a, x[14], S34, 0xfde5380c); // 36
  HH(a, b, c, d, x[1], S31, 0xa4beea44);  // 37
  HH(d, a, b, c, x[4], S32, 0x4bdecfa9);  // 38
  HH(c, d, a, b, x[7], S33, 0xf6bb4b60);  // 39
  HH(b, c, d, a, x[10], S34, 0xbebfbc70); // 40
  HH(a, b, c, d, x[13], S31, 0x289b7ec6); // 41
  HH(d, a, b, c, x[0], S32, 0xeaa127fa);  // 42
  HH(c, d, a, b, x[3], S33, 0xd4ef3085);  // 43
  HH(b, c, d, a, x[6], S34, 0x04881d05);  // 44
  HH(a, b, c, d, x[9], S31, 0xd9d4d039);  // 45
  HH(d, a, b, c, x[12], S32, 0xe6db99e5); // 46
  HH(c, d, a, b, x[15], S33, 0x1fa27cf8); // 47
  HH(b, c, d, a, x[2], S34, 0xc4ac5665);  // 48

  // Round 4
  II(a, b, c, d, x[0], S41, 0xf4292244);  // 49
  II(d, a, b, c, x[7], S42, 0x432aff97);  // 50
  II(c, d, a, b, x[14], S43, 0xab9423a7); // 51
  II(b, c, d, a, x[5], S44, 0xfc93a039);  // 52
  II(a, b, c, d, x[12], S41, 0x655b59c3); // 53
  II(d, a, b, c, x[3], S42, 0x8f0ccc92);  // 54
  II(c, d, a, b, x[10], S43, 0xffeff47d); // 55
  II(b, c, d, a, x[1], S44, 0x85845dd1);  // 56
  II(a, b, c, d, x[8], S41, 0x6fa87e4f);  // 57
  II(d, a, b, c, x[15], S42, 0xfe2ce6e0); // 58
  II(c, d, a, b, x[6], S43, 0xa3014314);  // 59
  II(b, c, d, a, x[13], S44, 0x4e0811a1); // 60
  II(a, b, c, d, x[4], S41, 0xf7537e82);  // 61
  II(d, a, b, c, x[11], S42, 0xbd3af235); // 62
  II(c, d, a, b, x[2], S43, 0x2ad7d2bb);  // 63
  II(b, c, d, a, x[9], S44, 0xeb86d391);  // 64

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

#undef FF
#undef GG
#undef HH
#undef II
}

void update(Md5Context& ctx, const std::uint8_t* input, std::size_t length) {
  std::size_t index = static_cast<std::size_t>((ctx.bitCount >> 3) & 0x3F);
  ctx.bitCount += static_cast<std::uint64_t>(length) << 3;

  const std::size_t partLen = 64 - index;
  std::size_t i = 0;

  if (length >= partLen) {
    std::memcpy(ctx.buffer.data() + index, input, partLen);
    transform(ctx.state, ctx.buffer.data());
    for (i = partLen; i + 63 < length; i += 64) {
      transform(ctx.state, input + i);
    }
    index = 0;
  }

  std::memcpy(ctx.buffer.data() + index, input + i, length - i);
}

std::array<std::uint8_t, 16> finalize(Md5Context& ctx) {
  static constexpr std::uint8_t PADDING[64] = {0x80};

  std::uint8_t bits[8];
  for (int i = 0; i < 8; ++i) {
    bits[i] = static_cast<std::uint8_t>((ctx.bitCount >> (8 * i)) & 0xFFu);
  }

  const std::size_t index = static_cast<std::size_t>((ctx.bitCount >> 3) & 0x3F);
  const std::size_t padLen = (index < 56) ? (56 - index) : (120 - index);
  update(ctx, PADDING, padLen);
  update(ctx, bits, 8);

  std::array<std::uint8_t, 16> digest{};
  for (int i = 0; i < 4; ++i) {
    digest[i * 4 + 0] = static_cast<std::uint8_t>(ctx.state[i] & 0xFFu);
    digest[i * 4 + 1] = static_cast<std::uint8_t>((ctx.state[i] >> 8) & 0xFFu);
    digest[i * 4 + 2] = static_cast<std::uint8_t>((ctx.state[i] >> 16) & 0xFFu);
    digest[i * 4 + 3] = static_cast<std::uint8_t>((ctx.state[i] >> 24) & 0xFFu);
  }
  return digest;
}

}  // namespace

std::string computeMd5Hex(const std::uint8_t* data, std::size_t size) {
  Md5Context ctx;
  update(ctx, data, size);
  const auto digest = finalize(ctx);

  static constexpr char hex[] = "0123456789abcdef";
  std::string out;
  out.resize(digest.size() * 2);
  for (std::size_t i = 0; i < digest.size(); ++i) {
    out[i * 2] = hex[(digest[i] >> 4) & 0x0Fu];
    out[i * 2 + 1] = hex[digest[i] & 0x0Fu];
  }
  return out;
}

}  // namespace fb_ops_test
