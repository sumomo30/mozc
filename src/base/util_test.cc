// Copyright 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <limits.h>
#include <fstream>
#include <sstream>
#include <string>
#include "base/file_stream.h"
#include "base/util.h"
#include "base/mutex.h"
#include "base/thread.h"
#include "testing/base/public/googletest.h"
#include "testing/base/public/gunit.h"

DECLARE_string(test_tmpdir);

namespace mozc {

class ThreadTest: public Thread {
 public:
  virtual void Run() {
    for (int i = 0; i < 3; ++i) {
#ifdef COMPILER_MSVC
      Sleep(1000);
#else
      sleep(1);
#endif
    }
  }
};

TEST(UtilTest, JoinStrings) {
  vector<string> input;
  input.push_back("ab");
  input.push_back("cdef");
  input.push_back("ghr");
  string output;
  Util::JoinStrings(input, ":", &output);
  EXPECT_EQ("ab:cdef:ghr", output);
}

TEST(UtilTest, SplitStringAllowEmpty) {
  {
    const string input = "a b  c def";
    vector<string> output;
    Util::SplitStringAllowEmpty(input, " ", &output);
    EXPECT_EQ(output.size(), 5);
    EXPECT_EQ("a", output[0]);
    EXPECT_EQ("b", output[1]);
    EXPECT_EQ("", output[2]);
    EXPECT_EQ("c", output[3]);
    EXPECT_EQ("def", output[4]);
  }

  {
    const string input = " a b  c";
    vector<string> output;
    Util::SplitStringAllowEmpty(input, " ", &output);
    EXPECT_EQ(output.size(), 5);
    EXPECT_EQ("", output[0]);
    EXPECT_EQ("a", output[1]);
    EXPECT_EQ("b", output[2]);
    EXPECT_EQ("", output[3]);
    EXPECT_EQ("c", output[4]);
  }

  {
    const string input = "a b  c ";
    vector<string> output;
    Util::SplitStringAllowEmpty(input, " ", &output);
    EXPECT_EQ(output.size(), 5);
    EXPECT_EQ("a", output[0]);
    EXPECT_EQ("b", output[1]);
    EXPECT_EQ("", output[2]);
    EXPECT_EQ("c", output[3]);
    EXPECT_EQ("", output[4]);
  }

  {
    const string input = "a:b  c ";
    vector<string> output;
    Util::SplitStringAllowEmpty(input, ": ", &output);
    EXPECT_EQ(output.size(), 5);
    EXPECT_EQ("a", output[0]);
    EXPECT_EQ("b", output[1]);
    EXPECT_EQ("", output[2]);
    EXPECT_EQ("c", output[3]);
    EXPECT_EQ("", output[4]);
  }
}

TEST(UtilTest, SplitCSV) {
  vector<string> answer_vector;

  Util::SplitCSV(
      "Google,x,\"Buchheit, Paul\",\"string with \"\" quote in it\"",
      &answer_vector);
  CHECK_EQ(answer_vector.size(), 4);
  CHECK_EQ(answer_vector[0], "Google");
  CHECK_EQ(answer_vector[1], "x");
  CHECK_EQ(answer_vector[2], "Buchheit, Paul");
  CHECK_EQ(answer_vector[3], "string with \" quote in it");

  Util::SplitCSV("Google,hello,",  &answer_vector);
  CHECK_EQ(answer_vector.size(), 3);
  CHECK_EQ(answer_vector[0], "Google");
  CHECK_EQ(answer_vector[1], "hello");
  CHECK_EQ(answer_vector[2], "");

  Util::SplitCSV("Google rocks,hello", &answer_vector);
  CHECK_EQ(answer_vector.size(), 2);
  CHECK_EQ(answer_vector[0], "Google rocks");
  CHECK_EQ(answer_vector[1], "hello");

  Util::SplitCSV(",,\"\",,", &answer_vector);
  CHECK_EQ(answer_vector.size(), 5);
  CHECK_EQ(answer_vector[0], "");
  CHECK_EQ(answer_vector[1], "");
  CHECK_EQ(answer_vector[2], "");
  CHECK_EQ(answer_vector[3], "");
  CHECK_EQ(answer_vector[4], "");

  // Test a string containing a comma.
  Util::SplitCSV("\",\",hello", &answer_vector);
  CHECK_EQ(answer_vector.size(), 2);
  CHECK_EQ(answer_vector[0], ",");
  CHECK_EQ(answer_vector[1], "hello");

  // Invalid CSV
  Util::SplitCSV("\"no,last,quote", &answer_vector);
  CHECK_EQ(answer_vector.size(), 1);
  CHECK_EQ(answer_vector[0], "no,last,quote");

  Util::SplitCSV("backslash\\,is,no,an,\"escape\"", &answer_vector);
  CHECK_EQ(answer_vector.size(), 5);
  CHECK_EQ(answer_vector[0], "backslash\\");
  CHECK_EQ(answer_vector[1], "is");
  CHECK_EQ(answer_vector[2], "no");
  CHECK_EQ(answer_vector[3], "an");
  CHECK_EQ(answer_vector[4], "escape");

  Util::SplitCSV("", &answer_vector);
  CHECK_EQ(answer_vector.size(), 0);
}

TEST(UtilTest, ReplaceString) {
  const string input = "foobarfoobar";
  string output;
  Util::StringReplace(input, "bar", "buz", true, &output);
  EXPECT_EQ("foobuzfoobuz", output);

  output.clear();
  Util::StringReplace(input, "bar", "buz", false, &output);
  EXPECT_EQ("foobuzfoobar", output);
}

TEST(UtilTest, LowerString) {
  string s = "TeSTtest";
  Util::LowerString(&s);
  EXPECT_EQ("testtest", s);

  // "ＴｅＳＴ＠ＡＢＣＸＹＺ［｀ａｂｃｘｙｚ｛"
  string s2 = "\xef\xbc\xb4\xef\xbd\x85\xef\xbc\xb3\xef\xbc\xb4\xef\xbc\xa0\xef"
              "\xbc\xa1\xef\xbc\xa2\xef\xbc\xa3\xef\xbc\xb8\xef\xbc\xb9\xef\xbc"
              "\xba\xef\xbc\xbb\xef\xbd\x80\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83"
              "\xef\xbd\x98\xef\xbd\x99\xef\xbd\x9a\xef\xbd\x9b";
  Util::LowerString(&s2);
  // "ｔｅｓｔ＠ａｂｃｘｙｚ［｀ａｂｃｘｙｚ｛"
  EXPECT_EQ("\xef\xbd\x94\xef\xbd\x85\xef\xbd\x93\xef\xbd\x94\xef\xbc\xa0\xef"
            "\xbd\x81\xef\xbd\x82\xef\xbd\x83\xef\xbd\x98\xef\xbd\x99\xef\xbd"
            "\x9a\xef\xbc\xbb\xef\xbd\x80\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83"
            "\xef\xbd\x98\xef\xbd\x99\xef\xbd\x9a\xef\xbd\x9b", s2);
}

TEST(UtilTest, UpperString) {
  string s = "TeSTtest";
  Util::UpperString(&s);
  EXPECT_EQ("TESTTEST", s);

  // "ＴｅＳＴ＠ＡＢＣＸＹＺ［｀ａｂｃｘｙｚ｛"
  string s2 = "\xef\xbc\xb4\xef\xbd\x85\xef\xbc\xb3\xef\xbc\xb4\xef\xbc\xa0\xef"
              "\xbc\xa1\xef\xbc\xa2\xef\xbc\xa3\xef\xbc\xb8\xef\xbc\xb9\xef\xbc"
              "\xba\xef\xbc\xbb\xef\xbd\x80\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83"
              "\xef\xbd\x98\xef\xbd\x99\xef\xbd\x9a\xef\xbd\x9b";
  Util::UpperString(&s2);
  // "ＴＥＳＴ＠ＡＢＣＸＹＺ［｀ＡＢＣＸＹＺ｛"
  EXPECT_EQ("\xef\xbc\xb4\xef\xbc\xa5\xef\xbc\xb3\xef\xbc\xb4\xef\xbc\xa0\xef"
            "\xbc\xa1\xef\xbc\xa2\xef\xbc\xa3\xef\xbc\xb8\xef\xbc\xb9\xef\xbc"
            "\xba\xef\xbc\xbb\xef\xbd\x80\xef\xbc\xa1\xef\xbc\xa2\xef\xbc\xa3"
            "\xef\xbc\xb8\xef\xbc\xb9\xef\xbc\xba\xef\xbd\x9b", s2);
}

TEST(UtilTest, CapitalizeString) {
  string s = "TeSTtest";
  Util::CapitalizeString(&s);
  EXPECT_EQ("Testtest", s);

  // "ＴｅＳＴ＠ＡＢＣＸＹＺ［｀ａｂｃｘｙｚ｛"
  string s2 = "\xef\xbc\xb4\xef\xbd\x85\xef\xbc\xb3\xef\xbc\xb4\xef\xbc\xa0\xef"
              "\xbc\xa1\xef\xbc\xa2\xef\xbc\xa3\xef\xbc\xb8\xef\xbc\xb9\xef\xbc"
              "\xba\xef\xbc\xbb\xef\xbd\x80\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83"
              "\xef\xbd\x98\xef\xbd\x99\xef\xbd\x9a\xef\xbd\x9b";
  Util::CapitalizeString(&s2);
  // "Ｔｅｓｔ＠ａｂｃｘｙｚ［｀ａｂｃｘｙｚ｛"
  EXPECT_EQ("\xef\xbc\xb4\xef\xbd\x85\xef\xbd\x93\xef\xbd\x94\xef\xbc\xa0\xef"
            "\xbd\x81\xef\xbd\x82\xef\xbd\x83\xef\xbd\x98\xef\xbd\x99\xef\xbd"
            "\x9a\xef\xbc\xbb\xef\xbd\x80\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83"
            "\xef\xbd\x98\xef\xbd\x99\xef\xbd\x9a\xef\xbd\x9b", s2);
}

TEST(UtilTest, CharsLen) {
  // "私の名前は中野です"
  const string src = "\xe7\xa7\x81\xe3\x81\xae\xe5\x90\x8d\xe5\x89\x8d\xe3\x81"
                     "\xaf\xe4\xb8\xad\xe9\x87\x8e\xe3\x81\xa7\xe3\x81\x99";
  EXPECT_EQ(Util::CharsLen(src.c_str(), src.size()), 9);
}

TEST(UtilTest, SubString) {
  // "私の名前は中野です"
  const string src = "\xe7\xa7\x81\xe3\x81\xae\xe5\x90\x8d\xe5\x89\x8d\xe3\x81"
                     "\xaf\xe4\xb8\xad\xe9\x87\x8e\xe3\x81\xa7\xe3\x81\x99";
  string result;

  result.clear();
  Util::SubString(src, 0, 2, &result);
  // "私の"
  EXPECT_EQ(result, "\xe7\xa7\x81\xe3\x81\xae");

  result.clear();
  Util::SubString(src, 4, 1, &result);
  // "は"
  EXPECT_EQ(result, "\xe3\x81\xaf");

  result.clear();
  Util::SubString(src, 5, 3, &result);
  // "中野で"
  EXPECT_EQ(result, "\xe4\xb8\xad\xe9\x87\x8e\xe3\x81\xa7");

  result.clear();
  Util::SubString(src, 6, 10, &result);
  // "野です"
  EXPECT_EQ(result, "\xe9\x87\x8e\xe3\x81\xa7\xe3\x81\x99");

  result.clear();
  Util::SubString(src, 4, 2, &result);
  // "は中"
  EXPECT_EQ(result, "\xe3\x81\xaf\xe4\xb8\xad");

  result.clear();
  Util::SubString(src, 2, string::npos, &result);
  // "名前は中野です"
  EXPECT_EQ(result, "\xe5\x90\x8d\xe5\x89\x8d\xe3\x81\xaf\xe4\xb8\xad\xe9\x87"
                    "\x8e\xe3\x81\xa7\xe3\x81\x99");

  result.clear();
  Util::SubString(src, 5, string::npos, &result);
  // "中野です"
  EXPECT_EQ(result, "\xe4\xb8\xad\xe9\x87\x8e\xe3\x81\xa7\xe3\x81\x99");
}

TEST(UtilTest, StripUTF8BOM) {
  string line;

  // Should be stripped.
  line = "\xef\xbb\xbf" "abc";
  Util::StripUTF8BOM(&line);
  EXPECT_EQ("abc", line);

  // Should be stripped.
  line = "\xef\xbb\xbf";
  Util::StripUTF8BOM(&line);
  EXPECT_EQ("", line);

  // BOM in the middle of text. Shouldn't be stripped.
  line = "a" "\xef\xbb\xbf" "bc";
  Util::StripUTF8BOM(&line);
  EXPECT_EQ("a" "\xef\xbb\xbf" "bc", line);

  // Incomplete BOM. Shouldn't be stripped.
  line = "\xef\xbb" "abc";
  Util::StripUTF8BOM(&line);
  EXPECT_EQ("\xef\xbb" "abc", line);

  // String shorter than the BOM. Do nothing.
  line = "a";
  Util::StripUTF8BOM(&line);
  EXPECT_EQ("a", line);

  // Empty string. Do nothing.
  line = "";
  Util::StripUTF8BOM(&line);
  EXPECT_EQ("", line);
}

TEST(UtilTest, IsUTF16BOM) {
  EXPECT_FALSE(Util::IsUTF16BOM(""));
  EXPECT_FALSE(Util::IsUTF16BOM("abc"));
  EXPECT_TRUE(Util::IsUTF16BOM("\xfe\xff"));
  EXPECT_TRUE(Util::IsUTF16BOM("\xff\xfe"));
  EXPECT_TRUE(Util::IsUTF16BOM("\xfe\xff "));
  EXPECT_TRUE(Util::IsUTF16BOM("\xff\xfe "));
  EXPECT_FALSE(Util::IsUTF16BOM(" \xfe\xff"));
  EXPECT_FALSE(Util::IsUTF16BOM(" \xff\xfe"));
  EXPECT_FALSE(Util::IsUTF16BOM("\xff\xff"));
}

TEST(UtilTest, SimpleItoa) {
  EXPECT_EQ("0", Util::SimpleItoa(0));
  EXPECT_EQ("123", Util::SimpleItoa(123));
  EXPECT_EQ("-1", Util::SimpleItoa(-1));

  char buf[100];
  snprintf(buf, arraysize(buf), "%d", INT_MAX);
  EXPECT_EQ(buf, Util::SimpleItoa(INT_MAX));

  snprintf(buf, arraysize(buf), "%d", INT_MIN);
  EXPECT_EQ(buf, Util::SimpleItoa(INT_MIN));
}

TEST(UtilTest, SimpleAtoi) {
  EXPECT_EQ(0, Util::SimpleAtoi("0"));
  EXPECT_EQ(123, Util::SimpleAtoi("123"));
  EXPECT_EQ(-1, Util::SimpleAtoi("-1"));
}

TEST(UtilTest, SafeStrToUInt32) {
  uint32 value = 0xDEADBEEF;

  EXPECT_TRUE(Util::SafeStrToUInt32("0", &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(Util::SafeStrToUInt32(" \t\r\n\v\f0 \t\r\n\v\f", &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(Util::SafeStrToUInt32("012345678", &value));
  EXPECT_EQ(12345678, value);
  EXPECT_TRUE(Util::SafeStrToUInt32("4294967295", &value));
  EXPECT_EQ(4294967295u, value);  // max of 32-bit unsigned integer

  EXPECT_FALSE(Util::SafeStrToUInt32("-0", &value));
  EXPECT_FALSE(Util::SafeStrToUInt32("4294967296", &value));  // overflow
  EXPECT_FALSE(Util::SafeStrToUInt32("0x1234", &value));
  EXPECT_FALSE(Util::SafeStrToUInt32("3e", &value));
  EXPECT_FALSE(Util::SafeStrToUInt32("0.", &value));
  EXPECT_FALSE(Util::SafeStrToUInt32(".0", &value));
  EXPECT_FALSE(Util::SafeStrToUInt32("", &value));
}

TEST(UtilTest, SafeStrToUInt64) {
  uint64 value = 0xDEADBEEF;

  EXPECT_TRUE(Util::SafeStrToUInt64("0", &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(Util::SafeStrToUInt64(" \t\r\n\v\f0 \t\r\n\v\f", &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(Util::SafeStrToUInt64("012345678", &value));
  EXPECT_EQ(12345678, value);
  EXPECT_TRUE(Util::SafeStrToUInt64("18446744073709551615", &value));
  EXPECT_EQ(18446744073709551615ull, value);  // max of 64-bit unsigned integer

  EXPECT_FALSE(Util::SafeStrToUInt64("-0", &value));
  EXPECT_FALSE(Util::SafeStrToUInt64("18446744073709551616",  // overflow
                                     &value));
  EXPECT_FALSE(Util::SafeStrToUInt64("0x1234", &value));
  EXPECT_FALSE(Util::SafeStrToUInt64("3e", &value));
  EXPECT_FALSE(Util::SafeStrToUInt64("0.", &value));
  EXPECT_FALSE(Util::SafeStrToUInt64(".0", &value));
  EXPECT_FALSE(Util::SafeStrToUInt64("", &value));
}

TEST(UtilTest, HiraganaToKatakana) {
  {
    // "わたしのなまえはなかのですうまーよろしゅう"
    const string input = "\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae\xe3"
                         "\x81\xaa\xe3\x81\xbe\xe3\x81\x88\xe3\x81\xaf\xe3\x81"
                         "\xaa\xe3\x81\x8b\xe3\x81\xae\xe3\x81\xa7\xe3\x81\x99"
                         "\xe3\x81\x86\xe3\x81\xbe\xe3\x83\xbc\xe3\x82\x88\xe3"
                         "\x82\x8d\xe3\x81\x97\xe3\x82\x85\xe3\x81\x86";
    string output;
    Util::HiraganaToKatakana(input, &output);
    // "ワタシノナマエハナカノデスウマーヨロシュウ"
    EXPECT_EQ("\xe3\x83\xaf\xe3\x82\xbf\xe3\x82\xb7\xe3\x83\x8e\xe3\x83\x8a\xe3"
              "\x83\x9e\xe3\x82\xa8\xe3\x83\x8f\xe3\x83\x8a\xe3\x82\xab\xe3\x83"
              "\x8e\xe3\x83\x87\xe3\x82\xb9\xe3\x82\xa6\xe3\x83\x9e\xe3\x83\xbc"
              "\xe3\x83\xa8\xe3\x83\xad\xe3\x82\xb7\xe3\x83\xa5\xe3\x82\xa6",
              output);
  }

  {
    // "グーグル工藤よろしくabc"
    const string input = "\xe3\x82\xb0\xe3\x83\xbc\xe3\x82\xb0\xe3\x83\xab\xe5"
                         "\xb7\xa5\xe8\x97\xa4\xe3\x82\x88\xe3\x82\x8d\xe3\x81"
                         "\x97\xe3\x81\x8f\x61\x62\x63";
    string output;
    Util::HiraganaToKatakana(input, &output);
    // "グーグル工藤ヨロシクabc"
    EXPECT_EQ("\xe3\x82\xb0\xe3\x83\xbc\xe3\x82\xb0\xe3\x83\xab\xe5\xb7\xa5\xe8"
              "\x97\xa4\xe3\x83\xa8\xe3\x83\xad\xe3\x82\xb7\xe3\x82\xaf\x61\x62"
              "\x63", output);
  }
}

TEST(UtilTest, NormalizeVoicedSoundMark) {
  // "僕のう゛ぁいおりん"
  const string input = "\xe5\x83\x95\xe3\x81\xae\xe3\x81\x86\xe3\x82\x9b\xe3"
                       "\x81\x81\xe3\x81\x84\xe3\x81\x8a\xe3\x82\x8a\xe3\x82"
                       "\x93";
  string output;
  Util::NormalizeVoicedSoundMark(input, &output);
  // "僕のゔぁいおりん"
  EXPECT_EQ("\xe5\x83\x95\xe3\x81\xae\xe3\x82\x94\xe3\x81\x81\xe3\x81\x84\xe3"
            "\x81\x8a\xe3\x82\x8a\xe3\x82\x93", output);
}

TEST(UtilTest, IsFullWidthSymbolInHalfWidthKatakana) {
  // "グーグル"
  EXPECT_FALSE(Util::IsFullWidthSymbolInHalfWidthKatakana("\xe3\x82\xb0\xe3\x83"
                                                          "\xbc\xe3\x82\xb0\xe3"
                                                          "\x83\xab"));
  // "ー"
  EXPECT_TRUE(Util::IsFullWidthSymbolInHalfWidthKatakana("\xe3\x83\xbc"));
  // "。"
  EXPECT_TRUE(Util::IsFullWidthSymbolInHalfWidthKatakana("\xe3\x80\x82"));
  // "グーグル。"
  EXPECT_FALSE(Util::IsFullWidthSymbolInHalfWidthKatakana("\xe3\x82\xb0\xe3\x83"
                                                          "\xbc\xe3\x82\xb0\xe3"
                                                          "\x83\xab\xe3\x80"
                                                          "\x82"));
}

TEST(UtilTest, IsHalfWidthKatakanaSymbol) {
  // "ｸﾞｰｸﾞﾙ"
  EXPECT_FALSE(Util::IsHalfWidthKatakanaSymbol("\xef\xbd\xb8\xef\xbe\x9e\xef"
                                               "\xbd\xb0\xef\xbd\xb8\xef\xbe"
                                               "\x9e\xef\xbe\x99"));
  // "ｰ"
  EXPECT_TRUE(Util::IsHalfWidthKatakanaSymbol("\xef\xbd\xb0"));
  // "｡"
  EXPECT_TRUE(Util::IsHalfWidthKatakanaSymbol("\xef\xbd\xa1"));
  // "､"
  EXPECT_TRUE(Util::IsHalfWidthKatakanaSymbol("\xef\xbd\xa4"));
  // "グーグル｡"
  EXPECT_FALSE(Util::IsHalfWidthKatakanaSymbol("\xe3\x82\xb0\xe3\x83\xbc\xe3"
                                               "\x82\xb0\xe3\x83\xab\xef\xbd"
                                               "\xa1"));
  // "､｡"
  // "not 。、"
  EXPECT_TRUE(Util::IsHalfWidthKatakanaSymbol("\xef\xbd\xa4\xef\xbd\xa1"));
}

TEST(UtilTest, FullWidthAndHalfWidth) {
  string output;

  Util::FullWidthToHalfWidth("", &output);
  CHECK_EQ("", output);

  Util::HalfWidthToFullWidth("", &output);
  CHECK_EQ("", output);

  Util::HalfWidthToFullWidth("abc[]?.", &output);
  // "ａｂｃ［］？．"
  CHECK_EQ("\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83\xef\xbc\xbb\xef\xbc\xbd\xef"
           "\xbc\x9f\xef\xbc\x8e", output);

  // "ｲﾝﾀｰﾈｯﾄ｢」"
  Util::HalfWidthToFullWidth("\xef\xbd\xb2\xef\xbe\x9d\xef\xbe\x80\xef\xbd\xb0"
                             "\xef\xbe\x88\xef\xbd\xaf\xef\xbe\x84\xef\xbd\xa2"
                             "\xe3\x80\x8d", &output);
  // "インターネット「」"
  CHECK_EQ("\xe3\x82\xa4\xe3\x83\xb3\xe3\x82\xbf\xe3\x83\xbc\xe3\x83\x8d\xe3"
           "\x83\x83\xe3\x83\x88\xe3\x80\x8c\xe3\x80\x8d", output);

  // "ｲﾝﾀｰﾈｯﾄグーグル"
  Util::HalfWidthToFullWidth("\xef\xbd\xb2\xef\xbe\x9d\xef\xbe\x80\xef\xbd\xb0"
                             "\xef\xbe\x88\xef\xbd\xaf\xef\xbe\x84\xe3\x82\xb0"
                             "\xe3\x83\xbc\xe3\x82\xb0\xe3\x83\xab", &output);
  // "インターネットグーグル"
  CHECK_EQ("\xe3\x82\xa4\xe3\x83\xb3\xe3\x82\xbf\xe3\x83\xbc\xe3\x83\x8d\xe3"
           "\x83\x83\xe3\x83\x88\xe3\x82\xb0\xe3\x83\xbc\xe3\x82\xb0\xe3\x83"
           "\xab", output);

  // "ａｂｃ［］？．"
  Util::FullWidthToHalfWidth("\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83\xef\xbc\xbb"
                             "\xef\xbc\xbd\xef\xbc\x9f\xef\xbc\x8e", &output);
  CHECK_EQ("abc[]?.", output);

  // "インターネット"
  Util::FullWidthToHalfWidth("\xe3\x82\xa4\xe3\x83\xb3\xe3\x82\xbf\xe3\x83\xbc"
                             "\xe3\x83\x8d\xe3\x83\x83\xe3\x83\x88", &output);
  // "ｲﾝﾀｰﾈｯﾄ"
  CHECK_EQ("\xef\xbd\xb2\xef\xbe\x9d\xef\xbe\x80\xef\xbd\xb0\xef\xbe\x88\xef"
           "\xbd\xaf\xef\xbe\x84", output);

  // "ｲﾝﾀｰﾈｯﾄグーグル"
  Util::FullWidthToHalfWidth("\xef\xbd\xb2\xef\xbe\x9d\xef\xbe\x80\xef\xbd\xb0"
                             "\xef\xbe\x88\xef\xbd\xaf\xef\xbe\x84\xe3\x82\xb0"
                             "\xe3\x83\xbc\xe3\x82\xb0\xe3\x83\xab", &output);
  // "ｲﾝﾀｰﾈｯﾄｸﾞｰｸﾞﾙ"
  CHECK_EQ("\xef\xbd\xb2\xef\xbe\x9d\xef\xbe\x80\xef\xbd\xb0\xef\xbe\x88\xef"
           "\xbd\xaf\xef\xbe\x84\xef\xbd\xb8\xef\xbe\x9e\xef\xbd\xb0\xef\xbd"
           "\xb8\xef\xbe\x9e\xef\xbe\x99", output);

  // spaces
  // " 　"
  Util::FullWidthToHalfWidth("\x20\xe3\x80\x80", &output);
  CHECK_EQ("  ", output);

  // " 　"
  Util::HalfWidthToFullWidth("\x20\xe3\x80\x80", &output);
  // "　　"
  CHECK_EQ("\xe3\x80\x80\xe3\x80\x80", output);

  // spaces are treated as Ascii here
  // " 　"
  Util::FullWidthAsciiToHalfWidthAscii("\x20\xe3\x80\x80", &output);
  CHECK_EQ("  ", output);

  // " 　"
  Util::HalfWidthAsciiToFullWidthAscii("\x20\xe3\x80\x80", &output);
  // "　　"
  CHECK_EQ("\xe3\x80\x80\xe3\x80\x80", output);

  // " 　"
  Util::FullWidthKatakanaToHalfWidthKatakana("\x20\xe3\x80\x80", &output);
  // " 　"
  CHECK_EQ("\x20\xe3\x80\x80", output);

  // " 　"
  Util::HalfWidthKatakanaToFullWidthKatakana("\x20\xe3\x80\x80", &output);
  // " 　"
  CHECK_EQ("\x20\xe3\x80\x80", output);
}

TEST(UtilTest, KanjiNumberToArabicNumber) {
  {
    // "十"
    string kanji = "\xe5\x8d\x81";
    string arabic;
    Util::KanjiNumberToArabicNumber(kanji, &arabic);
    EXPECT_EQ("10", arabic);
  }

  {
    // "百"
    string kanji = "\xe7\x99\xbe";
    string arabic;
    Util::KanjiNumberToArabicNumber(kanji, &arabic);
    EXPECT_EQ("100", arabic);
  }

  {
    // "千"
    string kanji = "\xe5\x8d\x83";
    string arabic;
    Util::KanjiNumberToArabicNumber(kanji, &arabic);
    EXPECT_EQ("1000", arabic);
  }

  {
    // "万"
    string kanji = "\xe4\xb8\x87";
    string arabic;
    Util::KanjiNumberToArabicNumber(kanji, &arabic);
    EXPECT_EQ("10000", arabic);
  }

  {
    // "億"
    string kanji = "\xe5\x84\x84";
    string arabic;
    Util::KanjiNumberToArabicNumber(kanji, &arabic);
    EXPECT_EQ("100000000", arabic);
  }

  {
    // "兆"
    string kanji = "\xe5\x85\x86";
    string arabic;
    Util::KanjiNumberToArabicNumber(kanji, &arabic);
    EXPECT_EQ("1000000000000", arabic);
  }

  {
    // "京"
    string kanji = "\xe4\xba\xac";
    string arabic;
    Util::KanjiNumberToArabicNumber(kanji, &arabic);
    EXPECT_EQ("10000000000000000", arabic);
  }
}

TEST(UtilTest, NormalizeNumbers) {
  {
    // "一万二十五"
    const string input = "\xe4\xb8\x80\xe4\xb8\x87\xe4\xba\x8c\xe5\x8d\x81\xe4"
                         "\xba\x94";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("10025", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "千"
    const string input = "\xe5\x8d\x83";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("1000", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "十五"
    const string input = "\xe5\x8d\x81\xe4\xba\x94";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("15", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "拾"
    const string input = "\xe6\x8b\xbe";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("10", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "拾四"
    const string input = "\xe6\x8b\xbe\xe5\x9b\x9b";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("14", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "廿万廿"
    const string input = "\xe5\xbb\xbf\xe4\xb8\x87\xe5\xbb\xbf";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("200020", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "四十五"
    const string input = "\xe5\x9b\x9b\xe5\x8d\x81\xe4\xba\x94";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("45", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "五百三十四億二千五十三万五百三十二"
    const string input = "\xe4\xba\x94\xe7\x99\xbe\xe4\xb8\x89\xe5\x8d\x81\xe5"
                         "\x9b\x9b\xe5\x84\x84\xe4\xba\x8c\xe5\x8d\x83\xe4\xba"
                         "\x94\xe5\x8d\x81\xe4\xb8\x89\xe4\xb8\x87\xe4\xba\x94"
                         "\xe7\x99\xbe\xe4\xb8\x89\xe5\x8d\x81\xe4\xba\x8c";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("53420530532", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "一千京"
    const string input = "\xe4\xb8\x80\xe5\x8d\x83\xe4\xba\xac";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("10000000000000000000", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // 2^64 - 1
    const string input =
        // "千八百四十四京六千七百四十四兆七百三十七億九百五十五万千六百十五"
        "\xe5\x8d\x83\xe5\x85\xab\xe7\x99\xbe\xe5\x9b\x9b\xe5\x8d\x81\xe5\x9b"
        "\x9b\xe4\xba\xac\xe5\x85\xad\xe5\x8d\x83\xe4\xb8\x83\xe7\x99\xbe\xe5"
        "\x9b\x9b\xe5\x8d\x81\xe5\x9b\x9b\xe5\x85\x86\xe4\xb8\x83\xe7\x99\xbe"
        "\xe4\xb8\x89\xe5\x8d\x81\xe4\xb8\x83\xe5\x84\x84\xe4\xb9\x9d\xe7\x99"
        "\xbe\xe4\xba\x94\xe5\x8d\x81\xe4\xba\x94\xe4\xb8\x87\xe5\x8d\x83\xe5"
        "\x85\xad\xe7\x99\xbe\xe5\x8d\x81\xe4\xba\x94";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
  }

  {
    // 2^64
    const string input =
        // "千八百四十四京六千七百四十四兆七百三十七億九百五十五万千六百十六"
        "\xe5\x8d\x83\xe5\x85\xab\xe7\x99\xbe\xe5\x9b\x9b\xe5\x8d\x81\xe5\x9b"
        "\x9b\xe4\xba\xac\xe5\x85\xad\xe5\x8d\x83\xe4\xb8\x83\xe7\x99\xbe\xe5"
        "\x9b\x9b\xe5\x8d\x81\xe5\x9b\x9b\xe5\x85\x86\xe4\xb8\x83\xe7\x99\xbe"
        "\xe4\xb8\x89\xe5\x8d\x81\xe4\xb8\x83\xe5\x84\x84\xe4\xb9\x9d\xe7\x99"
        "\xbe\xe4\xba\x94\xe5\x8d\x81\xe4\xba\x94\xe4\xb8\x87\xe5\x8d\x83\xe5"
        "\x85\xad\xe7\x99\xbe\xe5\x8d\x81\xe5\x85\xad";
    string arabic_output, kanji_output;
    EXPECT_FALSE(Util::NormalizeNumbers(input, true,
                                        &kanji_output, &arabic_output));
  }

  {
    // "2十5"
    const string input = "\x32\xe5\x8d\x81\x35";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("25", arabic_output);
    // "二十五"
    EXPECT_EQ("\xe4\xba\x8c\xe5\x8d\x81\xe4\xba\x94", kanji_output);
  }

  {
    // "二三五"
    const string input = "\xe4\xba\x8c\xe4\xb8\x89\xe4\xba\x94";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("235", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "二三五万四三"
    const string input = "\xe4\xba\x8c\xe4\xb8\x89\xe4\xba\x94\xe4\xb8\x87\xe5"
                         "\x9b\x9b\xe4\xb8\x89";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("2350043", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "二百三五万一"
    const string input = "\xe4\xba\x8c\xe7\x99\xbe\xe4\xb8\x89\xe4\xba\x94\xe4"
                         "\xb8\x87\xe4\xb8\x80";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("2350001", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "2千四十３"
    const string input = "\x32\xe5\x8d\x83\xe5\x9b\x9b\xe5\x8d\x81\xef\xbc\x93";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("2043", arabic_output);
    // "二千四十三"
    EXPECT_EQ("\xe4\xba\x8c\xe5\x8d\x83\xe5\x9b\x9b\xe5\x8d\x81\xe4\xb8\x89",
              kanji_output);
  }

  {
    // "弐拾参"
    const string input = "\xe5\xbc\x90\xe6\x8b\xbe\xe5\x8f\x82";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("23", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "零弐拾参"
    const string input = "\xe9\x9b\xb6\xe5\xbc\x90\xe6\x8b\xbe\xe5\x8f\x82";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("23", arabic_output);
    EXPECT_EQ(input, kanji_output);
  }

  {
    // "０１２"
    const string input = "\xef\xbc\x90\xef\xbc\x91\xef\xbc\x92";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("12", arabic_output);
    // "〇一二"
    EXPECT_EQ("\xe3\x80\x87\xe4\xb8\x80\xe4\xba\x8c", kanji_output);
  }

  {
    // "０１２"
    const string input = "\xef\xbc\x90\xef\xbc\x91\xef\xbc\x92";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, false,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("012", arabic_output);
    // "〇一二"
    EXPECT_EQ("\xe3\x80\x87\xe4\xb8\x80\xe4\xba\x8c", kanji_output);
  }

  {
    // "０00"
    const string input = "\xef\xbc\x90\x30\x30";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, false,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("000", arabic_output);
    // "〇〇〇"
    EXPECT_EQ("\xe3\x80\x87\xe3\x80\x87\xe3\x80\x87", kanji_output);
  }

  {
    // "００１２"
    const string input = "\xef\xbc\x90\xef\xbc\x90\xef\xbc\x91\xef\xbc\x92";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, false,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("0012", arabic_output);
    // "〇〇一二"
    EXPECT_EQ("\xe3\x80\x87\xe3\x80\x87\xe4\xb8\x80\xe4\xba\x8c", kanji_output);
  }

  {
    // "０零０１２"
    const string input = "\xef\xbc\x90\xe9\x9b\xb6\xef\xbc\x90"
                         "\xef\xbc\x91\xef\xbc\x92";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, false,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("00012", arabic_output);
    // "〇零〇一二"
    EXPECT_EQ("\xe3\x80\x87\xe9\x9b\xb6\xe3\x80\x87\xe4\xb8\x80\xe4\xba\x8c",
              kanji_output);
  }

  {
    const string input = "0";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("0", arabic_output);
    // "〇"
    EXPECT_EQ("\xe3\x80\x87", kanji_output);
  }

  {
    const string input = "00";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, true,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("0", arabic_output);
    // "〇〇"
    EXPECT_EQ("\xe3\x80\x87\xe3\x80\x87", kanji_output);
  }

  {
    const string input = "0";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, false,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("0", arabic_output);
    // "〇"
    EXPECT_EQ("\xe3\x80\x87", kanji_output);
  }

  {
    const string input = "00";
    string arabic_output, kanji_output;
    EXPECT_TRUE(Util::NormalizeNumbers(input, false,
                                       &kanji_output, &arabic_output));
    EXPECT_EQ("00", arabic_output);
    // "〇〇"
    EXPECT_EQ("\xe3\x80\x87\xe3\x80\x87", kanji_output);
  }

  {
    // "てすと"
    const string input = "\xe3\x81\xa6\xe3\x81\x99\xe3\x81\xa8";
    string arabic_output, kanji_output;
    EXPECT_FALSE(Util::NormalizeNumbers(input, true,
                                        &kanji_output, &arabic_output));
  }

  {
    // "てすと２"
    const string input = "\xe3\x81\xa6\xe3\x81\x99\xe3\x81\xa8\xef\xbc\x92";
    string arabic_output, kanji_output;
    EXPECT_FALSE(Util::NormalizeNumbers(input, true,
                                        &kanji_output, &arabic_output));
  }
}

TEST(UtilTest, Basename) {
#ifdef OS_WINDOWS
  EXPECT_EQ("bar", Util::Basename("\\foo\\bar"));
  EXPECT_EQ("foo.txt", Util::Basename("\\foo\\bar\\foo.txt"));
  EXPECT_EQ("foo.txt", Util::Basename("foo.txt"));
  EXPECT_EQ("foo.txt", Util::Basename(".\\foo.txt"));
  EXPECT_EQ(".foo.txt", Util::Basename(".\\.foo.txt"));
  EXPECT_EQ("", Util::Basename("\\"));
  EXPECT_EQ("", Util::Basename("foo\\bar\\buz\\"));
#else
  EXPECT_EQ("bar", Util::Basename("/foo/bar"));
  EXPECT_EQ("foo.txt", Util::Basename("/foo/bar/foo.txt"));
  EXPECT_EQ("foo.txt", Util::Basename("foo.txt"));
  EXPECT_EQ("foo.txt", Util::Basename("./foo.txt"));
  EXPECT_EQ(".foo.txt", Util::Basename("./.foo.txt"));
  EXPECT_EQ("", Util::Basename("/"));
  EXPECT_EQ("", Util::Basename("foo/bar/buz/"));
#endif
}

TEST(UtilTest, Dirname) {
#ifdef OS_WINDOWS
  EXPECT_EQ("\\foo", Util::Dirname("\\foo\\bar"));
  EXPECT_EQ("\\foo\\bar", Util::Dirname("\\foo\\bar\\foo.txt"));
  EXPECT_EQ("", Util::Dirname("foo.txt"));
  EXPECT_EQ("", Util::Dirname("\\"));
#else
  EXPECT_EQ("/foo", Util::Dirname("/foo/bar"));
  EXPECT_EQ("/foo/bar", Util::Dirname("/foo/bar/foo.txt"));
  EXPECT_EQ("", Util::Dirname("foo.txt"));
  EXPECT_EQ("", Util::Dirname("/"));
#endif
}

TEST(UtilTest, NormalizeDirectorySeparator) {
#ifdef OS_WINDOWS
  EXPECT_EQ("\\foo\\bar", Util::NormalizeDirectorySeparator("\\foo\\bar"));
  EXPECT_EQ("\\foo\\bar", Util::NormalizeDirectorySeparator("/foo\\bar"));
  EXPECT_EQ("\\foo\\bar", Util::NormalizeDirectorySeparator("\\foo/bar"));
  EXPECT_EQ("\\foo\\bar", Util::NormalizeDirectorySeparator("/foo/bar"));
  EXPECT_EQ("\\foo\\bar\\", Util::NormalizeDirectorySeparator("\\foo\\bar\\"));
  EXPECT_EQ("\\foo\\bar\\", Util::NormalizeDirectorySeparator("/foo/bar/"));
  EXPECT_EQ("", Util::NormalizeDirectorySeparator(""));
  EXPECT_EQ("\\", Util::NormalizeDirectorySeparator("/"));
  EXPECT_EQ("\\", Util::NormalizeDirectorySeparator("\\"));
#else
  EXPECT_EQ("\\foo\\bar", Util::NormalizeDirectorySeparator("\\foo\\bar"));
  EXPECT_EQ("/foo\\bar", Util::NormalizeDirectorySeparator("/foo\\bar"));
  EXPECT_EQ("\\foo/bar", Util::NormalizeDirectorySeparator("\\foo/bar"));
  EXPECT_EQ("/foo/bar", Util::NormalizeDirectorySeparator("/foo/bar"));
  EXPECT_EQ("\\foo\\bar\\", Util::NormalizeDirectorySeparator("\\foo\\bar\\"));
  EXPECT_EQ("/foo/bar/", Util::NormalizeDirectorySeparator("/foo/bar/"));
  EXPECT_EQ("", Util::NormalizeDirectorySeparator(""));
  EXPECT_EQ("/", Util::NormalizeDirectorySeparator("/"));
  EXPECT_EQ("\\", Util::NormalizeDirectorySeparator("\\"));
#endif
}

TEST(MutexTest, MutexTest) {
  mozc::Mutex mutex;
  mozc::scoped_lock l(&mutex);
}

TEST(ThreadTest, ThreadTest) {
  ThreadTest test;
//  test.SetJoinable(true);
//  test.Join();
}

TEST(UtilTest, ChopReturns) {
  string line = "line\n";
  EXPECT_TRUE(Util::ChopReturns(&line));
  EXPECT_EQ("line", line);

  line = "line\r";
  EXPECT_TRUE(Util::ChopReturns(&line));
  EXPECT_EQ("line", line);

  line = "line\r\n";
  EXPECT_TRUE(Util::ChopReturns(&line));
  EXPECT_EQ("line", line);

  line = "line";
  EXPECT_FALSE(Util::ChopReturns(&line));
  EXPECT_EQ("line", line);

  line = "line1\nline2\n";
  EXPECT_TRUE(Util::ChopReturns(&line));
  EXPECT_EQ("line1\nline2", line);

  line = "line\n\n\n";
  EXPECT_TRUE(Util::ChopReturns(&line));
  EXPECT_EQ("line", line);
}

// Initialize argc and argv for unittest.
class Arguments {
 public:
  Arguments(int argc, const char** argv)
      : argc_(argc) {
    argv_ = new char *[argc];
    for (int i = 0; i < argc; ++i) {
      int len = strlen(argv[i]);
      argv_[i] = new char[len + 1];
      strncpy(argv_[i], argv[i], len + 1);
    }
  }

  virtual ~Arguments() {
    for (int i = 0; i < argc_; ++i) {
      delete [] argv_[i];
    }
    delete [] argv_;
  }

  int argc() { return argc_; }
  char **argv() { return argv_; }

 private:
  int argc_;
  char **argv_;
};

TEST(UtilTest, CommandLineRotateArguments) {
  const char *arguments[] = {"command",
                             "--key1=value1",
                             "--key2", "v2",
                             "--k3=value3"};
  Arguments arg(arraysize(arguments), arguments);
  int argc = arg.argc();
  char **argv = arg.argv();

  Util::CommandLineRotateArguments(argc, &argv);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ("--key1=value1", argv[0]);
  EXPECT_STREQ("--key2", argv[1]);
  EXPECT_STREQ("v2", argv[2]);
  EXPECT_STREQ("--k3=value3", argv[3]);
  EXPECT_STREQ("command", argv[4]);

  --argc;
  ++argv;
  Util::CommandLineRotateArguments(argc, &argv);
  EXPECT_EQ(4, argc);
  EXPECT_STREQ("v2", argv[0]);
  EXPECT_STREQ("--k3=value3", argv[1]);
  EXPECT_STREQ("command", argv[2]);
  EXPECT_STREQ("--key2", argv[3]);

  Util::CommandLineRotateArguments(argc, &argv);
  EXPECT_STREQ("--k3=value3", argv[0]);
  EXPECT_STREQ("command", argv[1]);
  EXPECT_STREQ("--key2", argv[2]);
  EXPECT_STREQ("v2", argv[3]);

  // Make sure the result of the rotations.
  argc = arg.argc();
  argv = arg.argv();
  EXPECT_EQ(5, argc);
  EXPECT_STREQ("--key1=value1", argv[0]);
  EXPECT_STREQ("--k3=value3", argv[1]);
  EXPECT_STREQ("command", argv[2]);
  EXPECT_STREQ("--key2", argv[3]);
  EXPECT_STREQ("v2", argv[4]);
}


TEST(UtilTest, CommandLineGetFlag) {
  const char *arguments[] = {"command",
                             "--key1=value1",
                             "--key2", "v2",
                             "invalid_value3",
                             "--only_key3"};
  Arguments arg(arraysize(arguments), arguments);
  int argc = arg.argc();
  char **argv = arg.argv();

  string key, value;
  int used_args = 0;

  // The first argument should be skipped because it is the command name.
  --argc;
  ++argv;

  // Parse "--key1=value1".
  EXPECT_TRUE(Util::CommandLineGetFlag(argc, argv, &key, &value, &used_args));
  EXPECT_EQ("key1", key);
  EXPECT_EQ("value1", value);
  EXPECT_EQ(1, used_args);
  argc -= used_args;
  argv += used_args;

  // Parse "--key2" and "value2".
  EXPECT_TRUE(Util::CommandLineGetFlag(argc, argv, &key, &value, &used_args));
  EXPECT_EQ("key2", key);
  EXPECT_EQ("v2", value);
  EXPECT_EQ(2, used_args);
  argc -= used_args;
  argv += used_args;

  // Parse "invalid_value3".
  EXPECT_FALSE(Util::CommandLineGetFlag(argc, argv, &key, &value, &used_args));
  EXPECT_TRUE(key.empty());
  EXPECT_TRUE(value.empty());
  EXPECT_EQ(1, used_args);
  argc -= used_args;
  argv += used_args;

  // Parse "--only_key3".
  EXPECT_TRUE(Util::CommandLineGetFlag(argc, argv, &key, &value, &used_args));
  EXPECT_EQ("only_key3", key);
  EXPECT_TRUE(value.empty());
  EXPECT_EQ(1, used_args);
  argc -= used_args;
  argv += used_args;

  EXPECT_EQ(0, argc);
}

TEST(UtilTest, EncodeURI) {
  string encoded;
  // "もずく"
  Util::EncodeURI("\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f", &encoded);
  EXPECT_EQ("%E3%82%82%E3%81%9A%E3%81%8F", encoded);

  encoded.clear();
  Util::EncodeURI("mozc", &encoded);
  EXPECT_EQ("mozc", encoded);

  encoded.clear();
  Util::EncodeURI("http://mozc/?q=Hello World", &encoded);
  EXPECT_EQ("http%3A%2F%2Fmozc%2F%3Fq%3DHello%20World", encoded);
}

TEST(UtilTest, DecodeURI) {
  string decoded;
  Util::DecodeURI("%E3%82%82%E3%81%9A%E3%81%8F", &decoded);
  // "もずく"
  EXPECT_EQ("\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f", decoded);

  decoded.clear();
  Util::DecodeURI("mozc", &decoded);
  EXPECT_EQ("mozc", decoded);

  decoded.clear();
  Util::DecodeURI("http%3A%2F%2Fmozc%2F%3Fq%3DHello+World", &decoded);
  EXPECT_EQ("http://mozc/?q=Hello World", decoded);
}

TEST(UtilTest, AppendCGIParams) {
  vector<pair<string, string> > params;
  string url;
  Util::AppendCGIParams(params, &url);
  EXPECT_TRUE(url.empty());

  params.push_back(make_pair("foo", "b a+r"));
  url = "http://mozc.com?";
  Util::AppendCGIParams(params, &url);
  EXPECT_EQ("http://mozc.com?foo=b%20a%2Br", url);

  params.push_back(make_pair("buzz", "mozc"));
  url.clear();
  Util::AppendCGIParams(params, &url);
  EXPECT_EQ("foo=b%20a%2Br&buzz=mozc", url);
}

TEST(UtilTest, Escape) {
  string escaped;
  // "らむだ"
  Util::Escape("\xe3\x82\x89\xe3\x82\x80\xe3\x81\xa0", &escaped);
  EXPECT_EQ("\\xE3\\x82\\x89\\xE3\\x82\\x80\\xE3\\x81\\xA0", escaped);
}

TEST(UnitTest, EscapeHtml) {
  string escaped;
  Util::EscapeHtml("<>&'\"abc", &escaped);
  EXPECT_EQ("&lt;&gt;&amp;&#39;&quot;abc", escaped);
}

TEST(UnitTest, EscapeCss) {
  string escaped;
  Util::EscapeCss("<>&'\"abc", &escaped);
  EXPECT_EQ("&lt;>&'\"abc", escaped);
}

TEST(UtilTest, ScriptType) {
  // "くどう"
  EXPECT_TRUE(Util::IsScriptType("\xe3\x81\x8f\xe3\x81\xa9\xe3\x81\x86",
                                 Util::HIRAGANA));
  // "京都"
  EXPECT_TRUE(Util::IsScriptType("\xe4\xba\xac\xe9\x83\xbd", Util::KANJI));
  // "モズク"
  EXPECT_TRUE(Util::IsScriptType("\xe3\x83\xa2\xe3\x82\xba\xe3\x82\xaf",
                                 Util::KATAKANA));
  // "モズクﾓｽﾞｸ"
  EXPECT_TRUE(Util::IsScriptType("\xe3\x83\xa2\xe3\x82\xba\xe3\x82\xaf\xef\xbe"
                                 "\x93\xef\xbd\xbd\xef\xbe\x9e\xef\xbd\xb8",
                                 Util::KATAKANA));
  // "ぐーぐる"
  EXPECT_TRUE(Util::IsScriptType("\xe3\x81\x90\xe3\x83\xbc\xe3\x81\x90\xe3\x82"
                                 "\x8b", Util::HIRAGANA));
  // "グーグル"
  EXPECT_TRUE(Util::IsScriptType("\xe3\x82\xb0\xe3\x83\xbc\xe3\x82\xb0\xe3\x83"
                                 "\xab", Util::KATAKANA));

  EXPECT_TRUE(Util::IsScriptType("012", Util::NUMBER));
  // "０１２012"
  EXPECT_TRUE(Util::IsScriptType("\xef\xbc\x90\xef\xbc\x91\xef\xbc\x92\x30\x31"
                                 "\x32", Util::NUMBER));
  EXPECT_TRUE(Util::IsScriptType("abcABC", Util::ALPHABET));
  // "ＡＢＣＤ"
  EXPECT_TRUE(Util::IsScriptType("\xef\xbc\xa1\xef\xbc\xa2\xef\xbc\xa3\xef\xbc"
                                 "\xa4", Util::ALPHABET));
  EXPECT_TRUE(Util::IsScriptType("@!#", Util::UNKNOWN_SCRIPT));

  // "くどカう"
  EXPECT_FALSE(Util::IsScriptType("\xe3\x81\x8f\xe3\x81\xa9\xe3\x82\xab\xe3\x81"
                                  "\x86", Util::HIRAGANA));
  // "京あ都"
  EXPECT_FALSE(Util::IsScriptType("\xe4\xba\xac\xe3\x81\x82\xe9\x83\xbd",
                                  Util::KANJI));
  // "モズあク"
  EXPECT_FALSE(Util::IsScriptType("\xe3\x83\xa2\xe3\x82\xba\xe3\x81\x82\xe3\x82"
                                  "\xaf", Util::KATAKANA));
  // "モあズクﾓｽﾞｸ"
  EXPECT_FALSE(Util::IsScriptType("\xe3\x83\xa2\xe3\x81\x82\xe3\x82\xba\xe3\x82"
                                  "\xaf\xef\xbe\x93\xef\xbd\xbd\xef\xbe\x9e\xef"
                                  "\xbd\xb8", Util::KATAKANA));
  // "012あ"
  EXPECT_FALSE(Util::IsScriptType("\x30\x31\x32\xe3\x81\x82", Util::NUMBER));
  // "０１２あ012"
  EXPECT_FALSE(Util::IsScriptType("\xef\xbc\x90\xef\xbc\x91\xef\xbc\x92\xe3\x81"
                                  "\x82\x30\x31\x32", Util::NUMBER));
  // "abcABあC"
  EXPECT_FALSE(Util::IsScriptType("\x61\x62\x63\x41\x42\xe3\x81\x82\x43",
                                  Util::ALPHABET));
  // "ＡＢあＣＤ"
  EXPECT_FALSE(Util::IsScriptType("\xef\xbc\xa1\xef\xbc\xa2\xe3\x81\x82\xef\xbc"
                                  "\xa3\xef\xbc\xa4", Util::ALPHABET));
  // "ぐーぐるグ"
  EXPECT_FALSE(Util::IsScriptType("\xe3\x81\x90\xe3\x83\xbc\xe3\x81\x90\xe3\x82"
                                  "\x8b\xe3\x82\xb0", Util::HIRAGANA));
  // "グーグルぐ"
  EXPECT_FALSE(Util::IsScriptType("\xe3\x82\xb0\xe3\x83\xbc\xe3\x82\xb0\xe3\x83"
                                  "\xab\xe3\x81\x90", Util::KATAKANA));

  // "グーグルsuggest"
  EXPECT_TRUE(Util::ContainsScriptType("\xe3\x82\xb0\xe3\x83\xbc\xe3\x82\xb0"
                                       "\xe3\x83\xab\x73\x75\x67\x67\x65\x73"
                                       "\x74", Util::ALPHABET));
  // "グーグルサジェスト"
  EXPECT_FALSE(Util::ContainsScriptType("\xe3\x82\xb0\xe3\x83\xbc\xe3\x82\xb0"
                                        "\xe3\x83\xab\xe3\x82\xb5\xe3\x82\xb8"
                                        "\xe3\x82\xa7\xe3\x82\xb9\xe3\x83\x88",
                                        Util::ALPHABET));

  // "くどう"
  EXPECT_EQ(Util::HIRAGANA, Util::GetScriptType("\xe3\x81\x8f\xe3\x81\xa9\xe3"
                                                "\x81\x86"));
  // "京都"
  EXPECT_EQ(Util::KANJI, Util::GetScriptType("\xe4\xba\xac\xe9\x83\xbd"));
  // "モズク"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xe3\x83\xa2\xe3\x82\xba\xe3"
                                                "\x82\xaf"));
  // "モズクﾓｽﾞｸ"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xe3\x83\xa2\xe3\x82\xba\xe3"
                                                "\x82\xaf\xef\xbe\x93\xef\xbd"
                                                "\xbd\xef\xbe\x9e\xef\xbd"
                                                "\xb8"));
  // "ぐーぐる"
  EXPECT_EQ(Util::HIRAGANA, Util::GetScriptType("\xe3\x81\x90\xe3\x83\xbc\xe3"
                                                "\x81\x90\xe3\x82\x8b"));
  // "グーグル"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xe3\x82\xb0\xe3\x83\xbc\xe3"
                                                "\x82\xb0\xe3\x83\xab"));
  // "!グーグル"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\x21\xe3\x82\xb0\xe3\x83"
                                                      "\xbc\xe3\x82\xb0\xe3\x83"
                                                      "\xab"));
  // "ー"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x83\xbc"));
  // "ーー"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x83\xbc\xe3\x83"
                                                      "\xbc"));
  // "゛"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x82\x9b"));
  // "゜"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x82\x9c"));

  EXPECT_EQ(Util::NUMBER, Util::GetScriptType("012"));
  // "０１２012"
  EXPECT_EQ(Util::NUMBER, Util::GetScriptType("\xef\xbc\x90\xef\xbc\x91\xef\xbc"
                                              "\x92\x30\x31\x32"));
  EXPECT_EQ(Util::ALPHABET, Util::GetScriptType("abcABC"));
  // "ＡＢＣＤ"
  EXPECT_EQ(Util::ALPHABET, Util::GetScriptType("\xef\xbc\xa1\xef\xbc\xa2\xef"
                                                "\xbc\xa3\xef\xbc\xa4"));
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("@!#"));
  // "＠！＃"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xef\xbc\xa0\xef\xbc\x81"
                                                      "\xef\xbc\x83"));

  // "ーひらがな"
  EXPECT_EQ(Util::HIRAGANA, Util::GetScriptType("\xe3\x83\xbc\xe3\x81\xb2\xe3"
                                                "\x82\x89\xe3\x81\x8c\xe3\x81"
                                                "\xaa"));
  // "ーカタカナ"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xe3\x83\xbc\xe3\x82\xab\xe3"
                                                "\x82\xbf\xe3\x82\xab\xe3\x83"
                                                "\x8a"));
  // "ｰｶﾀｶﾅ"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xef\xbd\xb0\xef\xbd\xb6\xef"
                                                "\xbe\x80\xef\xbd\xb6\xef\xbe"
                                                "\x85"));
  // "ひらがなー"
  EXPECT_EQ(Util::HIRAGANA, Util::GetScriptType("\xe3\x81\xb2\xe3\x82\x89\xe3"
                                                "\x81\x8c\xe3\x81\xaa\xe3\x83"
                                                "\xbc"));
  // "カタカナー"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xe3\x82\xab\xe3\x82\xbf\xe3"
                                                "\x82\xab\xe3\x83\x8a\xe3\x83"
                                                "\xbc"));
  // "ｶﾀｶﾅｰ"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xef\xbd\xb6\xef\xbe\x80\xef"
                                                "\xbd\xb6\xef\xbe\x85\xef\xbd"
                                                "\xb0"));

  // "あ゛っ"
  EXPECT_EQ(Util::HIRAGANA, Util::GetScriptType("\xe3\x81\x82\xe3\x82\x9b\xe3"
                                                "\x81\xa3"));
  // "あ゜っ"
  EXPECT_EQ(Util::HIRAGANA, Util::GetScriptType("\xe3\x81\x82\xe3\x82\x9c\xe3"
                                                "\x81\xa3"));
  // "ア゛ッ"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xe3\x82\xa2\xe3\x82\x9b\xe3"
                                                "\x83\x83"));
  // "ア゜ッ"
  EXPECT_EQ(Util::KATAKANA, Util::GetScriptType("\xe3\x82\xa2\xe3\x82\x9c\xe3"
                                                "\x83\x83"));

  // "くどカう"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x81\x8f\xe3\x81\xa9"
                                                      "\xe3\x82\xab\xe3\x81"
                                                      "\x86"));
  // "京あ都"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe4\xba\xac\xe3\x81\x82"
                                                      "\xe9\x83\xbd"));
  // "モズあク"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x83\xa2\xe3\x82\xba"
                                                      "\xe3\x81\x82\xe3\x82"
                                                      "\xaf"));
  // "モあズクﾓｽﾞｸ"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x83\xa2\xe3\x81\x82"
                                                      "\xe3\x82\xba\xe3\x82\xaf"
                                                      "\xef\xbe\x93\xef\xbd\xbd"
                                                      "\xef\xbe\x9e\xef\xbd"
                                                      "\xb8"));
  // "012あ"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\x30\x31\x32\xe3\x81"
                                                      "\x82"));
  // "０１２あ012"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xef\xbc\x90\xef\xbc\x91"
                                                      "\xef\xbc\x92\xe3\x81\x82"
                                                      "\x30\x31\x32"));
  // "abcABあC"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\x61\x62\x63\x41\x42\xe3"
                                                      "\x81\x82\x43"));
  // "ＡＢあＣＤ"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xef\xbc\xa1\xef\xbc\xa2"
                                                      "\xe3\x81\x82\xef\xbc\xa3"
                                                      "\xef\xbc\xa4"));
  // "ぐーぐるグ"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x81\x90\xe3\x83\xbc"
                                                      "\xe3\x81\x90\xe3\x82\x8b"
                                                      "\xe3\x82\xb0"));
  // "グーグルぐ"
  EXPECT_EQ(Util::UNKNOWN_SCRIPT, Util::GetScriptType("\xe3\x82\xb0\xe3\x83\xbc"
                                                      "\xe3\x82\xb0\xe3\x83\xab"
                                                      "\xe3\x81\x90"));
}


TEST(UtilTest, FormType) {
  // "くどう"
  EXPECT_EQ(Util::FULL_WIDTH, Util::GetFormType("\xe3\x81\x8f\xe3\x81\xa9\xe3"
                                                "\x81\x86"));
  // "京都"
  EXPECT_EQ(Util::FULL_WIDTH, Util::GetFormType("\xe4\xba\xac\xe9\x83\xbd"));
  // "モズク"
  EXPECT_EQ(Util::FULL_WIDTH, Util::GetFormType("\xe3\x83\xa2\xe3\x82\xba\xe3"
                                                "\x82\xaf"));
  // "ﾓｽﾞｸ"
  EXPECT_EQ(Util::HALF_WIDTH, Util::GetFormType("\xef\xbe\x93\xef\xbd\xbd\xef"
                                                "\xbe\x9e\xef\xbd\xb8"));
  // "ぐーぐる"
  EXPECT_EQ(Util::FULL_WIDTH, Util::GetFormType("\xe3\x81\x90\xe3\x83\xbc\xe3"
                                                "\x81\x90\xe3\x82\x8b"));
  // "グーグル"
  EXPECT_EQ(Util::FULL_WIDTH, Util::GetFormType("\xe3\x82\xb0\xe3\x83\xbc\xe3"
                                                "\x82\xb0\xe3\x83\xab"));
  // "ｸﾞｰｸﾞﾙ"
  EXPECT_EQ(Util::HALF_WIDTH, Util::GetFormType("\xef\xbd\xb8\xef\xbe\x9e\xef"
                                                "\xbd\xb0\xef\xbd\xb8\xef\xbe"
                                                "\x9e\xef\xbe\x99"));
  // "ｰ"
  EXPECT_EQ(Util::HALF_WIDTH, Util::GetFormType("\xef\xbd\xb0"));
  // "ー"
  EXPECT_EQ(Util::FULL_WIDTH, Util::GetFormType("\xe3\x83\xbc"));

  EXPECT_EQ(Util::HALF_WIDTH, Util::GetFormType("012"));
  // "０１２012"
  EXPECT_EQ(Util::UNKNOWN_FORM, Util::GetFormType("\xef\xbc\x90\xef\xbc\x91\xef"
                                                  "\xbc\x92\x30\x31\x32"));
  EXPECT_EQ(Util::HALF_WIDTH, Util::GetFormType("abcABC"));
  // "ＡＢＣＤ"
  EXPECT_EQ(Util::FULL_WIDTH, Util::GetFormType("\xef\xbc\xa1\xef\xbc\xa2\xef"
                                                "\xbc\xa3\xef\xbc\xa4"));
  EXPECT_EQ(Util::HALF_WIDTH, Util::GetFormType("@!#"));
}

TEST(UtilTest, CharacterSet) {
  // "あいうえお"
  EXPECT_EQ(Util::JISX0208, Util::GetCharacterSet("\xe3\x81\x82\xe3\x81\x84\xe3"
                                                  "\x81\x86\xe3\x81\x88\xe3\x81"
                                                  "\x8a"));
  EXPECT_EQ(Util::ASCII, Util::GetCharacterSet("abc"));
  // "abcあいう"
  EXPECT_EQ(Util::JISX0208, Util::GetCharacterSet("\x61\x62\x63\xe3\x81\x82\xe3"
                                                  "\x81\x84\xe3\x81\x86"));

  // half width katakana
  // "ｶﾀｶﾅ"
  EXPECT_EQ(Util::JISX0201, Util::GetCharacterSet("\xef\xbd\xb6\xef\xbe\x80\xef"
                                                  "\xbd\xb6\xef\xbe\x85"));
  // "ｶﾀｶﾅカタカナ"
  EXPECT_EQ(Util::JISX0208, Util::GetCharacterSet("\xef\xbd\xb6\xef\xbe\x80\xef"
                                                  "\xbd\xb6\xef\xbe\x85\xe3\x82"
                                                  "\xab\xe3\x82\xbf\xe3\x82\xab"
                                                  "\xe3\x83\x8a"));

  // 0213
  // "Ⅰ"
  EXPECT_EQ(Util::JISX0213, Util::GetCharacterSet("\xe2\x85\xa0"));
  // "①"
  EXPECT_EQ(Util::JISX0213, Util::GetCharacterSet("\xe2\x91\xa0"));
  // "㊤"
  EXPECT_EQ(Util::JISX0213, Util::GetCharacterSet("\xe3\x8a\xa4"));

  // only in CP932
  // "凬"
  EXPECT_EQ(Util::CP932, Util::GetCharacterSet("\xe5\x87\xac"));

  // only in Unicode
  // "￦"
  EXPECT_EQ(Util::UNICODE_ONLY, Util::GetCharacterSet("\xef\xbf\xa6"));
}

TEST(UtilTest, WriteByteArray) {
  {
    ostringstream os;
    const string name = "Test";
    char buf[] = "mozc";
    size_t buf_size = sizeof(buf);
    Util::WriteByteArray(name, buf, buf_size, &os);
    EXPECT_NE(os.str().find("const size_t kTest_size ="), string::npos);
#ifdef OS_WINDOWS
    EXPECT_NE(string::npos,
              os.str().find("const uint64 kTest_data_uint64[] ="));
    EXPECT_NE(string::npos,
              os.str().find("const char *kTest_data = "
                            "reinterpret_cast<const char *>("
                            "kTest_data_uint64);"));
#else
    EXPECT_NE(os.str().find("const char kTest_data[] ="), string::npos);
#endif
    LOG(INFO) << os.str();
  }

  const char kExpected[] = "const size_t ktest_size = 3;\n"
#ifdef OS_WINDOWS
      "const uint64 ktest_data_uint64[] = {\n"
      "0x636261, };\n"
      "const char *ktest_data = reinterpret_cast<const char *>("
      "ktest_data_uint64);\n"
#else
      "const char ktest_data[] =\n"
      "\"" "\\" "x61" "\\" "x62" "\\" "x63" "\"\n"
      ";\n"
#endif
      ;
  {
    ostringstream os;
    Util::WriteByteArray("test",
                         "abc",
                         3,
                         &os);
    EXPECT_EQ(kExpected, os.str());
  }

  const string filepath = Util::JoinPath(FLAGS_test_tmpdir, "testfile");
  {
    OutputFileStream ofs(filepath.c_str());
    ofs << "abc";
  }

  {
    ostringstream os;
    Util::MakeByteArrayStream("test", filepath, &os);
    EXPECT_EQ(kExpected, os.str());
  }
}

TEST(UtilTest, DirectoryExists) {
  EXPECT_TRUE(Util::DirectoryExists(FLAGS_test_tmpdir));
  const string filepath = Util::JoinPath(FLAGS_test_tmpdir, "testfile");

  // Delete filepath, if it exists.
  if (Util::FileExists(filepath)) {
    Util::Unlink(filepath);
  }
  ASSERT_FALSE(Util::FileExists(filepath));

  // Create a file.
  ofstream file(filepath.c_str());
  file << "test data" << endl;
  file.close();

  EXPECT_TRUE(Util::FileExists(filepath));
  EXPECT_FALSE(Util::DirectoryExists(filepath));

  // Delete the file.
  Util::Unlink(filepath);
  ASSERT_FALSE(Util::FileExists(filepath));
}

TEST(UtilTest, CreateDirectory) {
  EXPECT_TRUE(Util::DirectoryExists(FLAGS_test_tmpdir));
  // dirpath = FLAGS_test_tmpdir/testdir
  const string dirpath = Util::JoinPath(FLAGS_test_tmpdir, "testdir");

  // Delete dirpath, if it exists.
  if (Util::FileExists(dirpath)) {
    Util::RemoveDirectory(dirpath);
  }
  ASSERT_FALSE(Util::FileExists(dirpath));

  // Create the directory.
  EXPECT_TRUE(Util::CreateDirectory(dirpath));
  EXPECT_TRUE(Util::DirectoryExists(dirpath));

  // Delete the directory.
  ASSERT_TRUE(Util::RemoveDirectory(dirpath));
  ASSERT_FALSE(Util::FileExists(dirpath));
}

TEST(UtilTest, GetTotalPhysicalMemoryTest) {
  EXPECT_GT(Util::GetTotalPhysicalMemory(), 0);
}

#ifdef OS_WINDOWS
TEST(UtilTest, IsWindowsX64Test) {
  // just make sure we can compile it.
  Util::IsWindowsX64();
}

TEST(UtilTest, GetFileVersion) {
  const wchar_t kDllName[] = L"kernel32.dll";

  wstring path = Util::GetSystemDir();
  path += L"\\";
  path += kDllName;

  int major, minor, build, revision;
  EXPECT_TRUE(Util::GetFileVersion(path, &major, &minor, &build, &revision));
}

TEST(UtilTest, GetFileVersionStringTest) {
  const wchar_t kDllName[] = L"kernel32.dll";

  wstring path = Util::GetSystemDir();
  path += L"\\";
  path += kDllName;

  const string version_string = Util::GetFileVersionString(path);

  vector<string> numbers;
  Util::SplitStringUsing(version_string, ".", &numbers);

  // must be 4 digits.
  ASSERT_EQ(numbers.size(), 4);

  // must be integer.
  uint32 dummy = 0;
  ASSERT_TRUE(Util::SafeStrToUInt32(numbers[0], &dummy));
  ASSERT_TRUE(Util::SafeStrToUInt32(numbers[1], &dummy));
  ASSERT_TRUE(Util::SafeStrToUInt32(numbers[2], &dummy));
  ASSERT_TRUE(Util::SafeStrToUInt32(numbers[3], &dummy));
}

TEST(UtilTest, UTF8ToWide) {
  const string input_utf8 = "abc";
  wstring output_wide;
  Util::UTF8ToWide(input_utf8, &output_wide);

  string output_utf8;
  Util::WideToUTF8(output_wide, &output_utf8);
  EXPECT_EQ("abc", output_utf8);
}
#endif  // OS_WINDOWS

TEST(UtilTest, AtomicRename) {
  // just test rename operation works as intended
  const string from = Util::JoinPath(FLAGS_test_tmpdir,
                                     "atomic_rename_test_from");
  const string to = Util::JoinPath(FLAGS_test_tmpdir,
                                   "atomic_rename_test_to");
  Util::Unlink(from);
  Util::Unlink(to);

  // |from| is not found
  EXPECT_FALSE(Util::AtomicRename(from, to));
  {
    OutputFileStream ofs(from.c_str());
    EXPECT_TRUE(ofs);
    ofs << "test" << endl;
  }

  EXPECT_TRUE(Util::AtomicRename(from, to));

  // from is deleted
  EXPECT_FALSE(Util::FileExists(from));
  EXPECT_FALSE(!Util::FileExists(to));

  {
    InputFileStream ifs(to.c_str());
    EXPECT_TRUE(ifs);
    string line;
    getline(ifs, line);
    EXPECT_EQ("test", line);
  }

  EXPECT_FALSE(Util::AtomicRename(from, to));

  Util::Unlink(from);
  Util::Unlink(to);

  // overwrite the file
  {
    OutputFileStream ofs1(from.c_str());
    ofs1 << "test";
    OutputFileStream ofs2(to.c_str());
    ofs2 << "test";
  }
  EXPECT_TRUE(Util::AtomicRename(from, to));

  Util::Unlink(from);
  Util::Unlink(to);
}

TEST(UtilTest, Issue2190350) {
  string result = "";
  Util::UTF8ToSJIS("a", &result);
  EXPECT_EQ(1, result.length());
  EXPECT_EQ("a", result);

  result = "";
  Util::SJISToUTF8("a", &result);
  EXPECT_EQ(1, result.length());
  EXPECT_EQ("a", result);
}
}  // namespace mozc