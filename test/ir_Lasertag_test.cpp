// Copyright 2017 David Conran

#include "IRsend.h"
#include "IRsend_test.h"
#include "gtest/gtest.h"

//   LL        AAA    SSSSS  EEEEEEE RRRRRR  TTTTTTT   AAA     GGGG
//   LL       AAAAA  SS      EE      RR   RR   TTT    AAAAA   GG  GG
//   LL      AA   AA  SSSSS  EEEEE   RRRRRR    TTT   AA   AA GG
//   LL      AAAAAAA      SS EE      RR  RR    TTT   AAAAAAA GG   GG
//   LLLLLLL AA   AA  SSSSS  EEEEEEE RR   RR   TTT   AA   AA  GGGGGG


// Tests for sendLasertag().

// Test sending simplest case data only.
TEST(TestSendLasertag, SendDataOnly) {
  IRsendTest irsend(4);
  irsend.begin();

  irsend.reset();
  irsend.sendLasertag(0x1);  // Red 1
  EXPECT_EQ(
      "m333s333m333s333m333s333m333s333m333s333m333s333m333s333m333s333m333"
      "s333m333s333m333s333m333s666m333s100000", irsend.outputStr());

  irsend.reset();
  irsend.sendLasertag(0x2);  // Red 2
  EXPECT_EQ(
      "m333s333m333s333m333s333m333s333m333s333m333s333m333s333"
      "m333s333m333s333m333s333m333s666m666s100333", irsend.outputStr());

  irsend.reset();
  irsend.sendLasertag(0x51);  // Green 1
  // Raw: (21)
  // m364s364m332s336m384s276m332s364m332s304m416s584m692s724m640s360m304s332m392s612m380,
  EXPECT_EQ(
    // m364s364m332s336m384s276m332s364m332s304m416s584
    // m692s724m640s360m304s332m392s612m380
      "m333s333m333s333m333s333m333s333m333s333m333s666"
      "m666s666m666s333m333s333m333s666m333s100000", irsend.outputStr());

  irsend.reset();
  // Raw: (19)
  // m332s308m412s280m360s336m332s304m444s248m332s644m744s612m696s692m668s636m360
  irsend.sendLasertag(0x55);  // Green 5
  EXPECT_EQ(
    // m332s308m412s280m360s336m332s304m444s248m332s644
    // m744s612m696s692m668s636m360
      "m333s333m333s333m333s333m333s333m333s333m333s666"
      "m666s666m666s666m666s666m333s100000",
      irsend.outputStr());
}

TEST(TestSendLasertag, SendDataWithRepeat) {
  IRsendTest irsend(4);
  irsend.begin();

  irsend.reset();
  irsend.sendLasertag(0x1, LASERTAG_BITS, 1);  // Red 1, one repeat.
  EXPECT_EQ(
      "m333s333m333s333m333s333m333s333m333s333m333s333m333s333m333s333"
      "m333s333m333s333m333s333m333s666m333s100000"
      "m333s333m333s333m333s333m333s333m333s333m333s333m333s333m333s333"
      "m333s333m333s333m333s333m333s666m333s100000", irsend.outputStr());

  irsend.reset();
  irsend.sendLasertag(0x52, LASERTAG_BITS, 2);  // Green 2, two repeats.
  EXPECT_EQ(
      "m333s333m333s333m333s333m333s333m333s333m333s666m666s666m666s333"
      "m333s666m666s100333"
      "m333s333m333s333m333s333m333s333m333s333m333s666m666s666m666s333"
      "m333s666m666s100333"
      "m333s333m333s333m333s333m333s333m333s333m333s666m666s666m666s333"
      "m333s666m666s100333", irsend.outputStr());
}

TEST(TestSendLasertag, SmallestMessageSize) {
  IRsendTest irsend(4);
  irsend.begin();

  irsend.reset();
  irsend.sendLasertag(0x1555);  // Alternating bit pattern will be the smallest.
  // i.e. 7 actual 'mark' pulses, which is a rawlen of 13.
  EXPECT_EQ("m0s333m666s666m666s666m666s666m666s666m666s666m666s666m333s100000",
            irsend.outputStr());
}

// Tests for decodeLasertag().

// Decode normal Lasertag messages.
TEST(TestDecodeLasertag, NormalSyntheticDecodeWithStrict) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  // Normal Lasertag 13-bit message.
  irsend.reset();
  irsend.sendLasertag(0x01);  // Red 1
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x01, irsend.capture.value);
  EXPECT_EQ(0x1, irsend.capture.address);  // Unit 1
  EXPECT_EQ(0x0, irsend.capture.command);  // Team Red
  EXPECT_FALSE(irsend.capture.repeat);

  irsend.reset();
  irsend.sendLasertag(0x02);  // Red 2
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x02, irsend.capture.value);
  EXPECT_EQ(0x2, irsend.capture.address);  // Unit 2
  EXPECT_EQ(0x0, irsend.capture.command);  // Team Red
  EXPECT_FALSE(irsend.capture.repeat);

  irsend.reset();
  irsend.sendLasertag(0x06);  // Red 6
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x06, irsend.capture.value);
  EXPECT_EQ(0x6, irsend.capture.address);  // Unit 6
  EXPECT_EQ(0x0, irsend.capture.command);  // Team Red
  EXPECT_FALSE(irsend.capture.repeat);

  irsend.reset();
  irsend.sendLasertag(0x51);  // Green 1
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x51, irsend.capture.value);
  EXPECT_EQ(0x1, irsend.capture.address);  // Unit 1
  EXPECT_EQ(0x5, irsend.capture.command);  // Team Green
  EXPECT_FALSE(irsend.capture.repeat);

  irsend.reset();
  irsend.sendLasertag(0x56);  // Green 6
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x56, irsend.capture.value);
  EXPECT_EQ(0x6, irsend.capture.address);  // Unit
  EXPECT_EQ(0x5, irsend.capture.command);  // Team
  EXPECT_FALSE(irsend.capture.repeat);
}

// Example data taken from: https://github.com/z3t0/Arduino-IRremote/issues/532
TEST(TestDecodeLasertag, RealExamples) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  irsend.reset();
  uint16_t green3[21] = {
      360, 364, 272, 360, 420, 248, 360, 360, 332, 308, 388, 612, 692, 696,
      636, 360, 332, 700, 300, 308, 416};
  irsend.sendRaw(green3, 21, 36000);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x53, irsend.capture.value);
  EXPECT_EQ(0x3, irsend.capture.address);  // Unit
  EXPECT_EQ(0x5, irsend.capture.command);  // Team

  irsend.reset();
  uint16_t green1[21] = {
      364, 364, 332, 336, 384, 276, 332, 364, 332, 304, 416, 584, 692, 724,
      640, 360, 304, 332, 392, 612, 380};
  irsend.sendRaw(green1, 21, 36000);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x51, irsend.capture.value);
  EXPECT_EQ(0x1, irsend.capture.address);  // Unit
  EXPECT_EQ(0x5, irsend.capture.command);  // Team

  irsend.reset();
  uint16_t green4[19] = {
      336, 304, 412, 280, 360, 360, 304, 308, 420, 276, 332, 636, 744, 620,
      688, 724, 640, 360, 304};
  irsend.sendRaw(green4, 19, 36000);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x54, irsend.capture.value);
  EXPECT_EQ(0x4, irsend.capture.address);  // Unit
  EXPECT_EQ(0x5, irsend.capture.command);  // Team

  irsend.reset();
  uint16_t unit15[25] = {
      280, 360, 360, 308, 332, 388, 308, 332, 360, 308, 360, 360, 304, 304,
      412, 284, 304, 692, 364, 360, 276, 336, 416, 276, 328};
  irsend.sendRaw(unit15, 25, 36000);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(LASERTAG, irsend.capture.decode_type);
  EXPECT_EQ(LASERTAG_BITS, irsend.capture.bits);
  EXPECT_EQ(0x0F, irsend.capture.value);
  EXPECT_EQ(0xF, irsend.capture.address);  // Unit
  EXPECT_EQ(0x0, irsend.capture.command);  // Team
}
