#!/usr/bin/python3
"""Unit tests for auto_analyse_raw_data.py"""
from io import StringIO
import unittest
import auto_analyse_raw_data as analyse


class TestRawIRMessage(unittest.TestCase):
  """Unit tests for the RawIRMessage class."""

  # pylint: disable=too-many-public-methods

  def test_display_binary(self):
    """Test the display_binary() method."""
    output = StringIO()
    message = analyse.RawIRMessage(100, [8000, 4000, 500, 500, 500], output,
                                   False)
    self.assertEqual(output.getvalue(), '')
    message.display_binary("10101010")
    message.display_binary("0000000000000000")
    message.display_binary("00010010001101000101011001111000")
    self.assertEqual(output.getvalue(), '\n'
                     '  Bits: 8\n'
                     '  Hex:  0xAA (MSB first)\n'
                     '        0x55 (LSB first)\n'
                     '  Dec:  170 (MSB first)\n'
                     '        85 (LSB first)\n'
                     '  Bin:  0b10101010 (MSB first)\n'
                     '        0b01010101 (LSB first)\n'
                     '\n'
                     '  Bits: 16\n'
                     '  Hex:  0x0000 (MSB first)\n'
                     '        0x0000 (LSB first)\n'
                     '  Dec:  0 (MSB first)\n'
                     '        0 (LSB first)\n'
                     '  Bin:  0b0000000000000000 (MSB first)\n'
                     '        0b0000000000000000 (LSB first)\n'
                     '\n'
                     '  Bits: 32\n'
                     '  Hex:  0x12345678 (MSB first)\n'
                     '        0x1E6A2C48 (LSB first)\n'
                     '  Dec:  305419896 (MSB first)\n'
                     '        510274632 (LSB first)\n'
                     '  Bin:  0b00010010001101000101011001111000 (MSB first)\n'
                     '        0b00011110011010100010110001001000 (LSB first)\n')


class TestAutoAnalyseRawData(unittest.TestCase):
  """Unit tests for the functions in AutoAnalyseRawData."""

  # pylint: disable=too-many-public-methods

  def test_dump_constants_simple(self):
    """Simple tests for the dump_constants() function."""
    ignore = StringIO()
    output = StringIO()
    defs = []
    message = analyse.RawIRMessage(200, [
        7930, 3952, 494, 1482, 520, 1482, 494, 1508, 494, 520, 494, 1482, 494,
        520, 494, 1482, 494, 1482, 494, 3978, 494, 520, 494, 520, 494, 520, 494,
        520, 520, 520, 494, 520, 494, 520, 494, 1482, 494
    ], ignore)
    analyse.dump_constants(message, defs, "BAR", output)
    self.assertEqual(defs, [
        'const uint16_t kBARHdrMark = 7930;',
        'const uint16_t kBARBitMark = 496;',
        'const uint16_t kBARHdrSpace = 3965;',
        'const uint16_t kBAROneSpace = 1485;',
        'const uint16_t kBARZeroSpace = 520;'
    ])
    self.assertEqual(output.getvalue(), 'Guessing key value:\n'
                     'kBARHdrMark   = 7930\n'
                     'kBARHdrSpace  = 3965\n'
                     'kBARBitMark   = 496\n'
                     'kBAROneSpace  = 1485\n'
                     'kBARZeroSpace = 520\n')

  def test_dump_constants_aircon(self):
    """More complex tests for the dump_constants() function."""
    ignore = StringIO()
    output = StringIO()
    defs = []
    message = analyse.RawIRMessage(200, [
        9008, 4496, 644, 1660, 676, 530, 648, 558, 672, 1636, 646, 1660, 644,
        556, 650, 584, 626, 560, 644, 580, 628, 1680, 624, 560, 648, 1662, 644,
        582, 648, 536, 674, 530, 646, 580, 628, 560, 670, 532, 646, 562, 644,
        556, 672, 536, 648, 1662, 646, 1660, 652, 554, 644, 558, 672, 538, 644,
        560, 668, 560, 648, 1638, 668, 536, 644, 1660, 668, 532, 648, 560, 648,
        1660, 674, 554, 622, 19990, 646, 580, 624, 1660, 648, 556, 648, 558,
        674, 556, 622, 560, 644, 564, 668, 536, 646, 1662, 646, 1658, 672, 534,
        648, 558, 644, 562, 648, 1662, 644, 584, 622, 558, 648, 562, 668, 534,
        670, 536, 670, 532, 672, 536, 646, 560, 646, 558, 648, 558, 670, 534,
        650, 558, 646, 560, 646, 560, 668, 1638, 646, 1662, 646, 1660, 646,
        1660, 648
    ], ignore)
    analyse.dump_constants(message, defs, "TEST", output)
    self.assertEqual(defs, [
        'const uint16_t kTESTHdrMark = 9008;',
        'const uint16_t kTESTBitMark = 650;',
        'const uint16_t kTESTHdrSpace = 4496;',
        'const uint16_t kTESTOneSpace = 1657;',
        'const uint16_t kTESTZeroSpace = 554;',
        'const uint16_t kTESTSpaceGap = 19990;'
    ])
    self.assertEqual(output.getvalue(), 'Guessing key value:\n'
                     'kTESTHdrMark   = 9008\n'
                     'kTESTHdrSpace  = 4496\n'
                     'kTESTBitMark   = 650\n'
                     'kTESTOneSpace  = 1657\n'
                     'kTESTZeroSpace = 554\n'
                     'kTESTSpaceGap = 19990\n')

  def test_convert_rawdata(self):
    """Tests for the convert_rawdata() function."""
    # trivial cases
    self.assertEqual(analyse.convert_rawdata("0"), [0])
    with self.assertRaises(ValueError) as context:
      analyse.convert_rawdata("")
    self.assertEqual(str(context.exception),
                     "Raw Data contains a non-numeric value of ''.")

    # Single parenthesis
    self.assertEqual(analyse.convert_rawdata("foo {10"), [10])
    self.assertEqual(analyse.convert_rawdata("20} bar"), [20])

    # No parentheses
    self.assertEqual(analyse.convert_rawdata("10,20 ,  30"), [10, 20, 30])

    # Dual parentheses
    self.assertEqual(analyse.convert_rawdata("{10,20 ,  30}"), [10, 20, 30])
    self.assertEqual(analyse.convert_rawdata("foo{10,20}bar"), [10, 20])

    # Many parentheses
    self.assertEqual(analyse.convert_rawdata("foo{10,20}{bar}"), [10, 20])
    self.assertEqual(analyse.convert_rawdata("foo{10,20}{bar}}{"), [10, 20])

    # Bad parentheses
    with self.assertRaises(ValueError) as context:
      analyse.convert_rawdata("}10{")
    self.assertEqual(str(context.exception),
                     "Raw Data not parsible due to parentheses placement.")

    # Non base-10 values
    with self.assertRaises(ValueError) as context:
      analyse.convert_rawdata("10, 20, foo, bar, 30")
    self.assertEqual(str(context.exception),
                     "Raw Data contains a non-numeric value of 'foo'.")

    # A messy usual "good" case.
    input_str = """uint16_t rawbuf[6] = {
    9008, 4496, 644,
    1660, 676,

    530}
    ;"""
    self.assertEqual(
        analyse.convert_rawdata(input_str), [9008, 4496, 644, 1660, 676, 530])

  def test_parse_and_report(self):
    """Tests for the parse_and_report() function."""

    # Without code generation.
    output = StringIO()
    input_str = """
        uint16_t rawbuf[139] = {9008, 4496, 644, 1660, 676, 530, 648, 558, 672,
            1636, 646, 1660, 644, 556, 650, 584, 626, 560, 644, 580, 628, 1680,
            624, 560, 648, 1662, 644, 582, 648, 536, 674, 530, 646, 580, 628,
            560, 670, 532, 646, 562, 644, 556, 672, 536, 648, 1662, 646, 1660,
            652, 554, 644, 558, 672, 538, 644, 560, 668, 560, 648, 1638, 668,
            536, 644, 1660, 668, 532, 648, 560, 648, 1660, 674, 554, 622, 19990,
            646, 580, 624, 1660, 648, 556, 648, 558, 674, 556, 622, 560, 644,
            564, 668, 536, 646, 1662, 646, 1658, 672, 534, 648, 558, 644, 562,
            648, 1662, 644, 584, 622, 558, 648, 562, 668, 534, 670, 536, 670,
            532, 672, 536, 646, 560, 646, 558, 648, 558, 670, 534, 650, 558,
            646, 560, 646, 560, 668, 1638, 646, 1662, 646, 1660, 646, 1660,
            648};"""
    analyse.parse_and_report(input_str, 200, False, "FOO", output)
    self.assertEqual(
        output.getvalue(), 'Found 139 timing entries.\n'
        'Potential Mark Candidates:\n'
        '[9008, 676]\n'
        'Potential Space Candidates:\n'
        '[19990, 4496, 1680, 584]\n'
        '\n'
        'Guessing encoding type:\n'
        'Looks like it uses space encoding. Yay!\n'
        '\n'
        'Guessing key value:\n'
        'kFOOHdrMark   = 9008\n'
        'kFOOHdrSpace  = 4496\n'
        'kFOOBitMark   = 650\n'
        'kFOOOneSpace  = 1657\n'
        'kFOOZeroSpace = 554\n'
        'kFOOSpaceGap = 19990\n'
        '\n'
        'Decoding protocol based on analysis so far:\n'
        '\n'
        'kFOOHdrMark+kFOOHdrSpace+10011000010100000000011000001010010GAP(19990)'
        '\n'
        '  Bits: 35\n'
        '  Hex:  0x4C2803052 (MSB first)\n'
        '        0x250600A19 (LSB first)\n'
        '  Dec:  20443050066 (MSB first)\n'
        '        9938405913 (LSB first)\n'
        '  Bin:  0b10011000010100000000011000001010010 (MSB first)\n'
        '        0b01001010000011000000000101000011001 (LSB first)\n'
        'kFOOBitMark(UNEXPECTED)01000000110001000000000000001111\n'
        '  Bits: 32\n'
        '  Hex:  0x40C4000F (MSB first)\n'
        '        0xF0002302 (LSB first)\n'
        '  Dec:  1086586895 (MSB first)\n'
        '        4026540802 (LSB first)\n'
        '  Bin:  0b01000000110001000000000000001111 (MSB first)\n'
        '        0b11110000000000000010001100000010 (LSB first)\n'
        '\n'
        'Total Nr. of suspected bits: 67\n')

    # With code generation.
    output = StringIO()
    input_str = """
        uint16_t rawbuf[37] = {7930, 3952, 494, 1482, 520, 1482, 494,
            1508, 494, 520, 494, 1482, 494, 520, 494, 1482, 494, 1482, 494,
            3978, 494, 520, 494, 520, 494, 520, 494, 520, 520, 520, 494, 520,
            494, 520, 494, 1482, 494};"""
    analyse.parse_and_report(input_str, 200, True, "FOO", output)
    self.assertEqual(
        output.getvalue(), 'Found 37 timing entries.\n'
        'Potential Mark Candidates:\n'
        '[7930, 520]\n'
        'Potential Space Candidates:\n'
        '[3978, 1508, 520]\n'
        '\n'
        'Guessing encoding type:\n'
        'Looks like it uses space encoding. Yay!\n'
        '\n'
        'Guessing key value:\n'
        'kFOOHdrMark   = 7930\n'
        'kFOOHdrSpace  = 3965\n'
        'kFOOBitMark   = 496\n'
        'kFOOOneSpace  = 1485\n'
        'kFOOZeroSpace = 520\n'
        '\n'
        'Decoding protocol based on analysis so far:\n'
        '\n'
        'kFOOHdrMark+kFOOHdrSpace+11101011\n'
        '  Bits: 8\n'
        '  Hex:  0xEB (MSB first)\n'
        '        0xD7 (LSB first)\n'
        '  Dec:  235 (MSB first)\n'
        '        215 (LSB first)\n'
        '  Bin:  0b11101011 (MSB first)\n'
        '        0b11010111 (LSB first)\n'
        'UNEXPECTED->kFOOHdrSpace+00000001\n'
        '  Bits: 8\n'
        '  Hex:  0x01 (MSB first)\n'
        '        0x80 (LSB first)\n'
        '  Dec:  1 (MSB first)\n'
        '        128 (LSB first)\n'
        '  Bin:  0b00000001 (MSB first)\n'
        '        0b10000000 (LSB first)\n'
        '\n'
        'Total Nr. of suspected bits: 16\n'
        '\n'
        'Generating a VERY rough code outline:\n'
        '\n'
        "// WARNING: This probably isn't directly usable. It's a guide only.\n"
        'const uint16_t kFOOHdrMark = 7930;\n'
        'const uint16_t kFOOBitMark = 496;\n'
        'const uint16_t kFOOHdrSpace = 3965;\n'
        'const uint16_t kFOOOneSpace = 1485;\n'
        'const uint16_t kFOOZeroSpace = 520;\n'
        'const uint16_t kFOOBits = 16;\n'
        '// Function should be safe up to 64 bits.\n'
        'void IRsend::sendFOO(const uint64_t data, const uint16_t nbits,'
        ' const uint16_t repeat) {\n'
        '  enableIROut(38);  // A guess. Most common frequency.\n'
        '  for (uint16_t r = 0; r <= repeat; r++) {\n'
        '    // Header\n'
        '    mark(kFOOHdrMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Data\n'
        '    // e.g. data = 0xEB, nbits = 8\n'
        '    sendData(kFOOBitMark, kFOOOneSpace, kFOOBitMark, kFOOZeroSpace,'
        ' data, nbits, true);\n'
        '    // Footer\n'
        '    mark(kFOOBitMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Data\n'
        '    // e.g. data = 0x1, nbits = 8\n'
        '    sendData(kFOOBitMark, kFOOOneSpace, kFOOBitMark, kFOOZeroSpace,'
        ' data, nbits, true);\n'
        '    // Footer\n'
        '    mark(kFOOBitMark);\n'
        '    space(kDefaultMessageGap);  // A 100% made up guess of the gap'
        ' between messages.\n'
        '  }\n'
        '}\n')

  def test_unusual_gaps(self):
    """Tests for unusual Space Gaps in parse_and_report() function."""

    # Tests for unusual Gaps. (Issue #482)
    output = StringIO()
    input_str = """
        uint16_t rawbuf[272] = {3485, 3512, 864, 864, 864, 2620, 864, 864,
            864, 2620, 864, 2620, 864, 2620, 864, 2620, 864, 2620, 864, 864,
            864, 2620, 864, 864, 864, 2620, 864, 2620, 864, 2620, 864, 2620,
            864, 2620, 864, 864, 864, 2620, 864, 864, 864, 864, 864, 864,
            864, 864, 864, 864, 864, 864, 864, 864, 864, 2620, 864, 864,
            864, 864, 864, 864, 864, 864, 864, 864, 864, 864,
            3485, 3512, 864, 864, 864, 2620, 864, 864, 864, 2620, 864, 2620,
            864, 2620, 864, 2620, 864, 2620, 864, 864, 864, 2620, 864, 864,
            864, 2620, 864, 2620, 864, 2620, 864, 2620, 864, 2620, 864, 864,
            864, 2620, 864, 864, 864, 864, 864, 864, 864, 864, 864, 864, 864,
            864, 864, 864, 864, 2620, 864, 864, 864, 864, 864, 864, 864, 864,
            864, 864, 864, 864,
            3485, 3512, 864, 13996,
            3485, 3512, 864, 864, 864, 864, 864, 2620, 864, 864, 864, 2620,
            864, 2620, 864, 2620, 864, 2620, 864, 864, 864, 864, 864, 2620,
            864, 864, 864, 2620, 864, 2620, 864, 2620, 864, 2620, 864, 864,
            864, 2620, 864, 2620, 864, 864, 864, 2620, 864, 2620, 864, 864,
            864, 864, 864, 864, 864, 2620, 864, 2620, 864, 864, 864, 2620,
            864, 2620, 864, 864, 864, 864,
            3485, 3512, 864, 864, 864, 864, 864, 2620, 864, 864, 864, 2620,
            864, 2620, 864, 2620, 864, 2620, 864, 864, 864, 864, 864, 2620,
            864, 864, 864, 2620, 864, 2620, 864, 2620, 864, 2620, 864, 864,
            864, 2620, 864, 2620, 864, 864, 864, 2620, 864, 2620, 864, 864,
            864, 864, 864, 864, 864, 2620, 864, 2620, 864, 864, 864, 2620,
            864, 2620, 864, 864, 864, 864, 3485, 3512, 864, 13996};"""
    analyse.parse_and_report(input_str, 200, True, "FOO", output)
    self.assertEqual(
        output.getvalue(), 'Found 272 timing entries.\n'
        'Potential Mark Candidates:\n'
        '[3485, 864]\n'
        'Potential Space Candidates:\n'
        '[13996, 3512, 2620, 864]\n'
        '\n'
        'Guessing encoding type:\n'
        'Looks like it uses space encoding. Yay!\n'
        '\n'
        'Guessing key value:\n'
        'kFOOHdrMark   = 3485\n'
        'kFOOHdrSpace  = 3512\n'
        'kFOOBitMark   = 864\n'
        'kFOOOneSpace  = 2620\n'
        'kFOOZeroSpace = 864\n'
        'kFOOSpaceGap = 13996\n'
        '\n'
        'Decoding protocol based on analysis so far:\n'
        '\n'
        'kFOOHdrMark+kFOOHdrSpace+01011111010111110100000001000000\n'
        '  Bits: 32\n'
        '  Hex:  0x5F5F4040 (MSB first)\n'
        '        0x0202FAFA (LSB first)\n'
        '  Dec:  1600077888 (MSB first)\n'
        '        33749754 (LSB first)\n'
        '  Bin:  0b01011111010111110100000001000000 (MSB first)\n'
        '        0b00000010000000101111101011111010 (LSB first)\n'
        'kFOOHdrMark+kFOOHdrSpace+01011111010111110100000001000000\n'
        '  Bits: 32\n'
        '  Hex:  0x5F5F4040 (MSB first)\n'
        '        0x0202FAFA (LSB first)\n'
        '  Dec:  1600077888 (MSB first)\n'
        '        33749754 (LSB first)\n'
        '  Bin:  0b01011111010111110100000001000000 (MSB first)\n'
        '        0b00000010000000101111101011111010 (LSB first)\n'
        'kFOOHdrMark+kFOOHdrSpace+GAP(13996)'
        'kFOOHdrMark+kFOOHdrSpace+00101111001011110110110001101100\n'
        '  Bits: 32\n'
        '  Hex:  0x2F2F6C6C (MSB first)\n'
        '        0x3636F4F4 (LSB first)\n'
        '  Dec:  791637100 (MSB first)\n'
        '        909571316 (LSB first)\n'
        '  Bin:  0b00101111001011110110110001101100 (MSB first)\n'
        '        0b00110110001101101111010011110100 (LSB first)\n'
        'kFOOHdrMark+kFOOHdrSpace+00101111001011110110110001101100\n'
        '  Bits: 32\n'
        '  Hex:  0x2F2F6C6C (MSB first)\n'
        '        0x3636F4F4 (LSB first)\n'
        '  Dec:  791637100 (MSB first)\n'
        '        909571316 (LSB first)\n'
        '  Bin:  0b00101111001011110110110001101100 (MSB first)\n'
        '        0b00110110001101101111010011110100 (LSB first)\n'
        'kFOOHdrMark+kFOOHdrSpace+GAP(13996)\n'
        'Total Nr. of suspected bits: 128\n'
        '\n'
        'Generating a VERY rough code outline:\n'
        '\n'
        "// WARNING: This probably isn't directly usable. It's a guide only.\n"
        'const uint16_t kFOOHdrMark = 3485;\n'
        'const uint16_t kFOOBitMark = 864;\n'
        'const uint16_t kFOOHdrSpace = 3512;\n'
        'const uint16_t kFOOOneSpace = 2620;\n'
        'const uint16_t kFOOZeroSpace = 864;\n'
        'const uint16_t kFOOSpaceGap = 13996;\n'
        'const uint16_t kFOOBits = 128;\n'
        'const uint16_t kFOOStateLength = 16;\n'
        "// DANGER: More than 64 bits detected. A uint64_t for 'data' won't"
        ' work!\n'
        '// Function should be safe up to 64 bits.\n'
        'void IRsend::sendFOO(const uint64_t data, const uint16_t nbits,'
        ' const uint16_t repeat) {\n'
        '  enableIROut(38);  // A guess. Most common frequency.\n'
        '  for (uint16_t r = 0; r <= repeat; r++) {\n'
        '    // Header\n'
        '    mark(kFOOHdrMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Data\n'
        '    // e.g. data = 0x5F5F4040, nbits = 32\n'
        '    sendData(kFOOBitMark, kFOOOneSpace, kFOOBitMark, kFOOZeroSpace,'
        ' data, nbits, true);\n'
        '    // Header\n'
        '    mark(kFOOHdrMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Data\n'
        '    // e.g. data = 0x5F5F4040, nbits = 32\n'
        '    sendData(kFOOBitMark, kFOOOneSpace, kFOOBitMark, kFOOZeroSpace,'
        ' data, nbits, true);\n'
        '    // Header\n'
        '    mark(kFOOHdrMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Gap\n'
        '    mark(kFOOBitMark);\n'
        '    space(kFOOSpaceGap);\n'
        '    // Header\n'
        '    mark(kFOOHdrMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Data\n'
        '    // e.g. data = 0x2F2F6C6C, nbits = 32\n'
        '    sendData(kFOOBitMark, kFOOOneSpace, kFOOBitMark, kFOOZeroSpace,'
        ' data, nbits, true);\n'
        '    // Header\n'
        '    mark(kFOOHdrMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Data\n'
        '    // e.g. data = 0x2F2F6C6C, nbits = 32\n'
        '    sendData(kFOOBitMark, kFOOOneSpace, kFOOBitMark, kFOOZeroSpace,'
        ' data, nbits, true);\n'
        '    // Header\n'
        '    mark(kFOOHdrMark);\n'
        '    space(kFOOHdrSpace);\n'
        '    // Gap\n'
        '    mark(kFOOBitMark);\n'
        '    space(kFOOSpaceGap);\n'
        '    space(kDefaultMessageGap);  // A 100% made up guess of the gap'
        ' between messages.\n'
        '  }\n'
        '}\n'
        '\n'
        '\n'
        '// Alternative >64 bit Function\n'
        'void IRsend::sendFOO(uint8_t data[], uint16_t nbytes, uint16_t repeat)'
        ' {\n'
        '  // nbytes should typically be kFOOStateLength\n'
        '  // data should typically be:\n'
        '  //   uint8_t data[kFOOStateLength] = {0x5F, 0x5F, 0x40, 0x40, 0x5F,'
        ' 0x5F, 0x40, 0x40, 0x2F, 0x2F, 0x6C, 0x6C, 0x2F, 0x2F, 0x6C, 0x6C};\n'
        '  // data[] is assumed to be in MSB order for this code.\n'
        '  for (uint16_t r = 0; r <= repeat; r++) {\n'
        '    sendGeneric(kFOOHdrMark, kFOOHdrSpace,\n'
        '                kFOOBitMark, kFOOOneSpace,\n'
        '                kFOOBitMark, kFOOZeroSpace,\n'
        '                kFOOBitMark,\n'
        '                kDefaultMessageGap, // 100% made-up guess at the'
        ' message gap.\n'
        '                data, nbytes,\n'
        '                38000, // Complete guess of the modulation'
        ' frequency.\n'
        '                true, 0, 50);\n'
        '  }\n'
        '}\n')

  def test_reduce_list(self):
    """Tests for the reduce_list method."""

    ignore = StringIO()
    message = analyse.RawIRMessage(200, [
        7930, 3952, 494, 1482, 520, 1482, 494, 1508, 494, 520, 494, 1482, 494,
        520, 494, 1482, 494, 1482, 494, 3978, 494, 520, 494, 520, 494, 520, 494,
        520, 520, 520, 494, 520, 494, 520, 494, 1482, 494
    ], ignore)
    test_space_data = [4496, 1660, 530, 558, 1636, 1660, 556]
    result_list, result_dict = message.reduce_list(test_space_data)
    self.assertEqual([4496, 1660, 558], result_list)
    self.assertEqual({
        558: [558, 556, 530],
        1660: [1660, 1660, 1636],
        4496: [4496]
    }, result_dict)

  def test_avg_list(self):
    """Tests for the avg_list method."""

    self.assertEqual(0, analyse.avg_list([]))
    self.assertEqual(23, analyse.avg_list([10, 20, 40]))


if __name__ == '__main__':
  unittest.main(verbosity=2)
