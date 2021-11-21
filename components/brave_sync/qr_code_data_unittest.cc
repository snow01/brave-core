/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/qr_code_data.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_sync {

TEST(QrCodeData, Serialization) {
  const std::string json_string1 =
      R"(
{
  "version": 2,
  "sync_code_hex" : "current hex code",
  "not_after": "1637080050"
}
)";
  auto qr_code_data1 = QrCodeData::FromJson(json_string1);
  EXPECT_NE(qr_code_data1.get(), nullptr);
  EXPECT_EQ(qr_code_data1->version, 2);
  EXPECT_EQ(qr_code_data1->sync_code_hex, "current hex code");
  // Due to precission lost on serialization with `ToJavaTime`, check the diff
  EXPECT_TRUE(
      (qr_code_data1->not_after - base::Time::FromJavaTime(1637080050)) <
      base::TimeDelta::FromSeconds(1));

  const std::string json_string = qr_code_data1->ToJson();
  EXPECT_NE(json_string.length(), 0ul);

  auto qr_code_data2 = QrCodeData::FromJson(json_string);
  EXPECT_EQ(qr_code_data1->version, 2);
  EXPECT_EQ(qr_code_data1->sync_code_hex, qr_code_data2->sync_code_hex);

  EXPECT_TRUE(qr_code_data1->not_after - qr_code_data2->not_after <
              base::TimeDelta::FromSeconds(1));

  auto qr_code_data3 = QrCodeData::FromJson("not a json");
  EXPECT_EQ(qr_code_data3.get(), nullptr);
}

TEST(QrCodeData, CreateWithActualDate) {
  auto qr_code_data1 = QrCodeData::CreateWithActualDate("sync_code_hex");
  EXPECT_GE(qr_code_data1->not_after, base::Time::Now());
  EXPECT_LE(qr_code_data1->not_after,
            base::Time::Now() + base::TimeDelta::FromMinutes(
                                    QrCodeData::kMinutesFromNowForValidCode));
}

}  // namespace brave_sync
