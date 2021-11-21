/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/qr_code_data.h"

#include <memory>
#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"

// Example of the JSON:
// {
//   "version": 2,
//   "sync_code_hex" : "<current hex code>",
//   "not_after": "1637080050"
// }

namespace brave_sync {

namespace {

bool ParseTimeValue(const base::Value* value, base::Time* field) {
  DCHECK(value->is_string());
  std::string not_after_string = value->GetString();

  if (not_after_string.empty()) {
    VLOG(1) << "Missing 'not after time'";
    return false;
  }

  int64_t not_after_int;
  if (!base::StringToInt64(not_after_string, &not_after_int)) {
    VLOG(1) << "Wrong 'not after time'";
    return false;
  }

  *field = base::Time::FromJavaTime(not_after_int);

  return true;
}
}  // namespace

QrCodeData::QrCodeData() : version(kCurrentQrCodeDataVersion) {}

QrCodeData::QrCodeData(const std::string& sync_code_hex,
                       const base::Time& not_after)
    : version(kCurrentQrCodeDataVersion),
      sync_code_hex(sync_code_hex),
      not_after(not_after) {}

std::unique_ptr<QrCodeData> QrCodeData::CreateWithActualDate(
    const std::string& sync_code_hex) {
  return std::unique_ptr<QrCodeData>(new QrCodeData(
      sync_code_hex, base::Time::Now() + base::TimeDelta::FromMinutes(
                                             kMinutesFromNowForValidCode)));
}

void QrCodeData::RegisterJSONConverter(
    base::JSONValueConverter<QrCodeData>* converter) {
  converter->RegisterIntField("version", &QrCodeData::version);
  converter->RegisterStringField("sync_code_hex", &QrCodeData::sync_code_hex);
  converter->RegisterCustomValueField<base::Time>(
      "not_after", &QrCodeData::not_after, &ParseTimeValue);
}

std::unique_ptr<base::DictionaryValue> QrCodeData::ToValue() const {
  auto dict = std::make_unique<base::DictionaryValue>();

  dict->SetInteger("version", version);
  dict->SetString("sync_code_hex", sync_code_hex);
  dict->SetString("not_after", base::NumberToString(not_after.ToJavaTime()));

  return dict;
}

std::string QrCodeData::ToJson() {
  auto dict = ToValue();
  CHECK(dict);

  std::string json_string;
  if (!base::JSONWriter::Write(*dict.get(), &json_string)) {
    VLOG(1) << "Writing QR data to JSON failed";
    json_string = std::string();
  }

  return json_string;
}

std::unique_ptr<QrCodeData> QrCodeData::FromJson(
    const std::string& json_string) {
  auto qr_data = std::unique_ptr<QrCodeData>(new QrCodeData());
  absl::optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value) {
    VLOG(1) << "Read QR data from JSON failed";
    return nullptr;
  }

  base::JSONValueConverter<QrCodeData> converter;
  if (!converter.Convert(*value, qr_data.get())) {
    VLOG(1) << "Parse QR data JSON failed";
    return nullptr;
  }
  return qr_data;
}

}  // namespace brave_sync
