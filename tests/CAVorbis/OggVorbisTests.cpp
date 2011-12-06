/*
 *  OggVorbisTests.cpp
 *
 *    CAOggVorbisDecoder class test cases.
 *
 *
 *  Copyright (c) 2007  Arek Korbik
 *
 *  This file is part of XiphQT, the Xiph QuickTime Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: OggVorbisTests.cpp 12814 2007-03-27 22:09:32Z arek $
 *
 */


#include "OggVorbisTests.h"
#include <iostream>
#include <fstream>


OggVorbisTests::OggVorbisTests(TestInvocation *invocation)
    : TestCase(invocation)
{
}


OggVorbisTests::~OggVorbisTests()
{
}

void OggVorbisTests::setUp()
{
    mOggDecoder = new CAOggVorbisDecoder();
}

void OggVorbisTests::tearDown()
{
    delete mOggDecoder;
    mOggDecoder = NULL;
}

void OggVorbisTests::noop()
{
}

void OggVorbisTests::append_uninitialized()
{
    UInt32 bytes = 0;
    UInt32 packets = 0;

    Boolean appended = false;

    try {
        mOggDecoder->AppendInputData(NULL, bytes, packets, NULL);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == false);
}

void OggVorbisTests::init_cookie()
{
    std::ifstream f_in;
    char cookie[8192];

    f_in.open("../tests/data/vorbis.ogg.cookie", std::ios::in);

    CPTAssert(f_in.good());

    f_in.read(cookie, 8192);
    f_in.close();

    AudioStreamBasicDescription in_dsc = {44100.0, 'XoVs', 0, 0, 0, 0, 2, 0, 0};
    AudioStreamBasicDescription out_dsc = {44100.0, kAudioFormatLinearPCM,
                                           kAudioFormatFlagsNativeFloatPacked,
                                           8, 1, 8, 2, 32, 0};

    mOggDecoder->Initialize(&in_dsc, &out_dsc, NULL, 0);

    CPTAssert(!mOggDecoder->IsInitialized());

    mOggDecoder->Uninitialize();

    CPTAssert(!mOggDecoder->IsInitialized());

    mOggDecoder->Initialize(NULL, NULL, cookie, 4216);

    CPTAssert(mOggDecoder->IsInitialized());
}

void OggVorbisTests::append_single()
{
    std::ifstream f_in;
    char buffer[16384];

    f_in.open("../tests/data/vorbis.ogg.cookie", std::ios::in);

    CPTAssert(f_in.good());

    f_in.read(buffer, 16384);
    f_in.close();

    AudioStreamBasicDescription in_dsc = {44100.0, 'XoVs', 0, 0, 0, 0, 2, 0, 0};
    AudioStreamBasicDescription out_dsc = {44100.0, kAudioFormatLinearPCM,
                                           kAudioFormatFlagsNativeFloatPacked,
                                           8, 1, 8, 2, 32, 0};

    mOggDecoder->Initialize(&in_dsc, &out_dsc, buffer, 4216);
    CPTAssert(mOggDecoder->IsInitialized());

    f_in.open("../tests/data/vorbis.ogg.data", std::ios::in);

    CPTAssert(f_in.good());

    f_in.read(buffer, 16384);
    f_in.close();

    UInt32 bytes = 4152;
    UInt32 packets = 1;

    Boolean appended = false;

    AudioStreamPacketDescription pd = {0, 6720, 4152};

    try {
        mOggDecoder->AppendInputData(buffer, bytes, packets, &pd);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == true);

    packets = 6720;
    bytes = 8 * packets;
    char audio[bytes];
    UInt32 ac_ret = kAudioCodecProduceOutputPacketFailure;

    try {
        ac_ret = mOggDecoder->ProduceOutputPackets(audio, bytes, packets, NULL);
    } catch (...) {
        bytes = 0;
        packets = 0;
    };

    CPTAssert(ac_ret == kAudioCodecProduceOutputPacketSuccess &&
              bytes == 53760 && packets == 6720);
}

void OggVorbisTests::append_multiple()
{
    std::ifstream f_in;
    char buffer[16384];

    f_in.open("../tests/data/vorbis.ogg.cookie", std::ios::in);

    CPTAssert(f_in.good());

    f_in.read(buffer, 16384);
    f_in.close();

    AudioStreamBasicDescription in_dsc = {44100.0, 'XoVs', 0, 0, 0, 0, 2, 0, 0};
    AudioStreamBasicDescription out_dsc = {44100.0, kAudioFormatLinearPCM,
                                           kAudioFormatFlagsNativeFloatPacked,
                                           8, 1, 8, 2, 32, 0};

    mOggDecoder->Initialize(&in_dsc, &out_dsc, buffer, 4216);
    CPTAssert(mOggDecoder->IsInitialized());

    f_in.open("../tests/data/vorbis.ogg.data", std::ios::in);

    CPTAssert(f_in.good());

    f_in.read(buffer, 16384);
    f_in.close();

    UInt32 bytes = 8370;
    UInt32 packets = 2;

    Boolean appended = false;

    AudioStreamPacketDescription pd[2] = {{0, 6720, 4152}, {4152, 7168, 4218}};

    try {
        mOggDecoder->AppendInputData(buffer, bytes, packets, pd);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == true);

    packets = 6720;
    bytes = 8 * packets;
    char audio[bytes];
    UInt32 ac_ret = kAudioCodecProduceOutputPacketFailure;

    try {
        ac_ret = mOggDecoder->ProduceOutputPackets(audio, bytes, packets, NULL);
    } catch (...) {
        bytes = 0;
        packets = 0;
    };

    CPTAssert(ac_ret == kAudioCodecProduceOutputPacketSuccess &&
              bytes == 53760 && packets == 6720);
}

/*
 * The first ogg page contains more/less audio data than indicated by the grpos.
 * (Vorbis I specification, Section A.2)
 */
void OggVorbisTests::audio_offset()
{
    std::ifstream f_in;
    char cookie[8192];
    char buffer[16384];
    UInt32 cookie_size = 4216;

    f_in.open("../tests/data/vorbis.ogg.cookie", std::ios::in);

    CPTAssert(f_in.good());

    f_in.read(cookie, 8192);
    f_in.close();

    AudioStreamBasicDescription in_dsc = {44100.0, 'XoVs', 0, 0, 0, 0, 2, 0, 0};
    AudioStreamBasicDescription out_dsc = {44100.0, kAudioFormatLinearPCM,
                                           kAudioFormatFlagsNativeFloatPacked,
                                           8, 1, 8, 2, 32, 0};

    mOggDecoder->Initialize(&in_dsc, &out_dsc, cookie, cookie_size);
    CPTAssert(mOggDecoder->IsInitialized());

    f_in.open("../tests/data/vorbis.ogg.data", std::ios::in);

    CPTAssert(f_in.good());

    f_in.read(buffer, 16384);
    f_in.close();

    UInt32 bytes = 4152;
    UInt32 packets = 1;
    UInt32 orig_duration = 6720;

    Boolean appended = false;

    AudioStreamPacketDescription pd = {0, orig_duration, 4152};

    try {
        mOggDecoder->AppendInputData(buffer, bytes, packets, &pd);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == true);

    packets = orig_duration;
    bytes = 8 * packets;
    char audio_1[bytes];
    UInt32 ac_ret = kAudioCodecProduceOutputPacketFailure;

    try {
        ac_ret = mOggDecoder->ProduceOutputPackets(audio_1, bytes, packets, NULL);
    } catch (...) {
        bytes = 0;
        packets = 0;
    };

    CPTAssert(ac_ret == kAudioCodecProduceOutputPacketSuccess &&
              bytes == (orig_duration * 8) && packets == orig_duration);

    /* decode again, increased duration (decoder should prepend zeros) */
    mOggDecoder->Reset();
    mOggDecoder->Uninitialize();
    CPTAssert(!mOggDecoder->IsInitialized());

    mOggDecoder->Initialize(&in_dsc, &out_dsc, cookie, cookie_size);
    CPTAssert(mOggDecoder->IsInitialized());

    bytes = 4152;
    packets = 1;
    appended = false;
    UInt32 inc_duration = 6721;

    pd.mStartOffset = 0;
    pd.mVariableFramesInPacket = inc_duration;
    pd.mDataByteSize = 4152;

    try {
        mOggDecoder->AppendInputData(buffer, bytes, packets, &pd);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == true);

    packets = inc_duration;
    bytes = 8 * packets;
    char audio_2[bytes];
    char audio_z[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ac_ret = kAudioCodecProduceOutputPacketFailure;

    try {
        ac_ret = mOggDecoder->ProduceOutputPackets(audio_2, bytes, packets, NULL);
    } catch (...) {
        bytes = 0;
        packets = 0;
    };

    CPTAssert(ac_ret == kAudioCodecProduceOutputPacketSuccess &&
              bytes == (inc_duration * 8) && packets == inc_duration);
    CPTAssert(memcmp(audio_2, audio_z, 8) == 0);
    CPTAssert(memcmp(audio_2 + ((inc_duration - orig_duration) * 8),
                     audio_1, (orig_duration * 8)) == 0);

    /* decode again, decreased duration (decoder should truncate head) */
    mOggDecoder->Reset();
    mOggDecoder->Uninitialize();
    CPTAssert(!mOggDecoder->IsInitialized());

    mOggDecoder->Initialize(&in_dsc, &out_dsc, cookie, cookie_size);
    CPTAssert(mOggDecoder->IsInitialized());

    bytes = 4152;
    packets = 1;
    appended = false;
    UInt32 dec_duration_1 = 6716;

    pd.mStartOffset = 0;
    pd.mVariableFramesInPacket = dec_duration_1;
    pd.mDataByteSize = 4152;

    try {
        mOggDecoder->AppendInputData(buffer, bytes, packets, &pd);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == true);

    packets = dec_duration_1;
    bytes = 8 * packets;
    ac_ret = kAudioCodecProduceOutputPacketFailure;

    try {
        ac_ret = mOggDecoder->ProduceOutputPackets(audio_2, bytes, packets, NULL);
    } catch (...) {
        bytes = 0;
        packets = 0;
    };

    CPTAssert(ac_ret == kAudioCodecProduceOutputPacketSuccess &&
              bytes == (dec_duration_1 * 8) && packets == dec_duration_1);
    CPTAssert(memcmp(audio_2, audio_1 + ((orig_duration - dec_duration_1) * 8),
                     (dec_duration_1 * 8)) == 0);

    /* decode again, duration decreased a lot */
    mOggDecoder->Reset();
    mOggDecoder->Uninitialize();
    CPTAssert(!mOggDecoder->IsInitialized());

    mOggDecoder->Initialize(&in_dsc, &out_dsc, cookie, cookie_size);
    CPTAssert(mOggDecoder->IsInitialized());

    bytes = 4152;
    packets = 1;
    appended = false;
    UInt32 dec_duration_2 = 2560;

    pd.mStartOffset = 0;
    pd.mVariableFramesInPacket = dec_duration_2;
    pd.mDataByteSize = 4152;

    try {
        mOggDecoder->AppendInputData(buffer, bytes, packets, &pd);
        appended = true;
    } catch (...) {
    };

    CPTAssert(appended == true);

    packets = dec_duration_2;
    bytes = 8 * packets;
    ac_ret = kAudioCodecProduceOutputPacketFailure;

    try {
        ac_ret = mOggDecoder->ProduceOutputPackets(audio_2, bytes, packets, NULL);
    } catch (...) {
        bytes = 0;
        packets = 0;
    };

    CPTAssert(ac_ret == kAudioCodecProduceOutputPacketSuccess &&
              bytes == (dec_duration_2 * 8) && packets == dec_duration_2);
    CPTAssert(memcmp(audio_2, audio_1 + ((orig_duration - dec_duration_2) * 8),
                     (dec_duration_2 * 8)) == 0);
}


OggVorbisTests t_OV_noop(TEST_INVOCATION(OggVorbisTests, noop));
OggVorbisTests t_OV_append_uninitialized(TEST_INVOCATION(OggVorbisTests,
                                                         append_uninitialized));
OggVorbisTests t_OV_init_cookie(TEST_INVOCATION(OggVorbisTests, init_cookie));
OggVorbisTests t_OV_append_single(TEST_INVOCATION(OggVorbisTests, append_single));
OggVorbisTests t_OV_append_multiple(TEST_INVOCATION(OggVorbisTests,
                                                    append_multiple));
OggVorbisTests t_OV_audio_offset(TEST_INVOCATION(OggVorbisTests, audio_offset));
