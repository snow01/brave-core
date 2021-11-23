/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_request_helper.h"

#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"

namespace {

absl::optional<base::Value::ListStorage> GetParamsList(
    const std::string& json) {
  auto json_value =
      base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value || !json_value->is_dict())
    return absl::nullopt;

  base::Value* params = json_value->FindListPath(brave_wallet::kParams);
  if (!params || !params->is_list())
    return absl::nullopt;

  return std::move(*params).TakeList();
}

absl::optional<base::Value> GetObjectFromParamsList(const std::string& json) {
  auto list = GetParamsList(json);
  if (!list || list->size() != 1 || !list->front().is_dict())
    return absl::nullopt;

  return list->front().Clone();
}

// This is a best effort parsing of the data
brave_wallet::mojom::TxDataPtr ValueToTxData(const base::Value& tx_value,
                                             std::string* from_out) {
  CHECK(from_out);
  auto tx_data = brave_wallet::mojom::TxData::New();
  const base::DictionaryValue* params_dict = nullptr;
  if (!tx_value.GetAsDictionary(&params_dict) || !params_dict)
    return nullptr;

  const std::string* from = params_dict->FindStringKey("from");
  if (from)
    *from_out = *from;

  const std::string* to = params_dict->FindStringKey("to");
  if (to)
    tx_data->to = *to;

  const std::string* gas = params_dict->FindStringKey("gas");
  if (gas)
    tx_data->gas_limit = *gas;

  const std::string* gas_price = params_dict->FindStringKey("gasPrice");
  if (gas_price)
    tx_data->gas_price = *gas_price;

  const std::string* value = params_dict->FindStringKey("value");
  if (value)
    tx_data->value = *value;

  const std::string* data = params_dict->FindStringKey("data");
  if (data) {
    // If data is specified it's best to make sure it's valid
    if (data->length() <= 2 || data->substr(0, 2) != "0x")
      return nullptr;
    std::string hex_substr = data->substr(2);
    std::vector<uint8_t> bytes;
    // HexStringToBytes needs an even number of bytes
    if (hex_substr.length() % 2 == 1)
      hex_substr = "0" + hex_substr;
    if (!base::HexStringToBytes(hex_substr, &bytes))
      return nullptr;
    tx_data->data = bytes;
  }

  return tx_data;
}

// null request ID when unspecified is expected
const base::Value kDefaultRequestIdWhenUnspecified("1");
const char kRequestJsonRPC[] = "2.0";

}  // namespace

namespace brave_wallet {

mojom::TxDataPtr ParseEthSendTransactionParams(const std::string& json,
                                               std::string* from) {
  CHECK(from);
  from->clear();

  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj)
    return nullptr;
  return ValueToTxData(*param_obj, from);
}

mojom::TxData1559Ptr ParseEthSendTransaction1559Params(const std::string& json,
                                                       std::string* from) {
  CHECK(from);
  from->clear();
  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj)
    return nullptr;

  auto tx_data = mojom::TxData1559::New();
  auto base_data_ret = ValueToTxData(*param_obj, from);
  if (!base_data_ret)
    return nullptr;

  tx_data->base_data = std::move(base_data_ret);
  const base::DictionaryValue* params_dict = nullptr;
  if (!param_obj->GetAsDictionary(&params_dict) || !params_dict)
    return nullptr;

  const std::string* max_priority_fee_per_gas =
      params_dict->FindStringKey("maxPriorityFeePerGas");
  if (max_priority_fee_per_gas)
    tx_data->max_priority_fee_per_gas = *max_priority_fee_per_gas;

  const std::string* max_fee_per_gas =
      params_dict->FindStringKey("maxFeePerGas");
  if (max_fee_per_gas)
    tx_data->max_fee_per_gas = *max_fee_per_gas;

  return tx_data;
}

bool ShouldCreate1559Tx(brave_wallet::mojom::TxData1559Ptr tx_data_1559,
                        bool network_supports_eip1559,
                        const std::vector<mojom::AccountInfoPtr>& account_infos,
                        const std::string& address) {
  bool keyring_supports_eip1559 = true;
  auto account_it = std::find_if(account_infos.begin(), account_infos.end(),
                                 [&](const mojom::AccountInfoPtr& account) {
                                   return base::EqualsCaseInsensitiveASCII(
                                       account->address, address);
                                 });

  // Only ledger hardware keyring supports EIP-1559 at the moment.
  if (account_it != account_infos.end() && (*account_it)->hardware &&
      (*account_it)->hardware->vendor != mojom::kLedgerHardwareVendor) {
    keyring_supports_eip1559 = false;
  }

  // Network or keyring without EIP1559 support.
  if (!network_supports_eip1559 || !keyring_supports_eip1559)
    return false;

  // Network with EIP1559 support and EIP1559 gas fields are specified.
  if (tx_data_1559 && !tx_data_1559->max_priority_fee_per_gas.empty() &&
      !tx_data_1559->max_fee_per_gas.empty())
    return true;

  // Network with EIP1559 support and legacy gas fields are specified.
  if (tx_data_1559 && !tx_data_1559->base_data->gas_price.empty())
    return false;

  // Network with EIP1559 support and no gas fields are specified.
  return true;
}

bool GetEthJsonRequestInfo(const std::string& json,
                           base::Value* id,
                           std::string* method,
                           std::string* params) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  if (id) {
    const base::Value* found_id = response_dict->FindPath(kId);
    if (found_id)
      *id = found_id->Clone();
    else
      *id = base::Value();
  }

  if (method) {
    const std::string* found_method = response_dict->FindStringPath(kMethod);
    if (!found_method)
      return false;
    *method = *found_method;
  }

  if (params) {
    const base::Value* found_params = response_dict->FindListPath(kParams);
    if (!found_params)
      return false;
    base::JSONWriter::Write(*found_params, params);
  }

  return true;
}

bool NormalizeEthRequest(const std::string& input_json,
                         std::string* output_json) {
  CHECK(output_json);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          input_json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v)
    return false;

  base::DictionaryValue* out_dict;
  if (!records_v->GetAsDictionary(&out_dict))
    return false;

  const base::Value* found_id = out_dict->FindPath(kId);
  if (!found_id) {
    ALLOW_UNUSED_LOCAL(
        out_dict->SetPath("id", kDefaultRequestIdWhenUnspecified.Clone()));
  }

  ALLOW_UNUSED_LOCAL(out_dict->SetStringPath("jsonrpc", kRequestJsonRPC));
  base::JSONWriter::Write(*out_dict, output_json);

  return true;
}

bool ParseEthSignParams(const std::string& json,
                        std::string* address,
                        std::string* message) {
  if (!address || !message)
    return false;

  auto list = GetParamsList(json);
  if (!list || list->size() != 2)
    return false;

  const std::string* address_str = (*list)[0].GetIfString();
  const std::string* message_str = (*list)[1].GetIfString();
  if (!address_str || !message_str)
    return false;

  *address = *address_str;
  *message = *message_str;

  return true;
}

bool ParsePersonalSignParams(const std::string& json,
                             std::string* address,
                             std::string* message) {
  if (!address || !message)
    return false;

  // personal_sign allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 2)
    return false;

  // personal_sign has the reversed order
  const std::string* message_str = (*list)[0].GetIfString();
  const std::string* address_str = (*list)[1].GetIfString();
  if (!address_str || !message_str)
    return false;

  *address = *address_str;
  if (IsValidHexString(*message_str)) {
    *message = *message_str;
  } else {
    *message = ToHex(*message_str);
  }

  return true;
}

bool ParseEthSignTypedDataParams(const std::string& json,
                                 std::string* address,
                                 std::string* message_out,
                                 std::vector<uint8_t>* message_to_sign_out,
                                 base::Value* domain_out,
                                 EthSignTypedDataHelper::Version version) {
  if (!address || !message_out || !domain_out || !message_to_sign_out)
    return false;

  auto list = GetParamsList(json);
  if (!list || list->size() != 2)
    return false;

  const std::string* address_str = (*list)[0].GetIfString();
  const std::string* typed_data_str = (*list)[1].GetIfString();
  if (!address_str || !typed_data_str)
    return false;

  auto typed_data =
      base::JSONReader::Read(*typed_data_str, base::JSON_ALLOW_TRAILING_COMMAS);
  if (!typed_data || !typed_data->is_dict())
    return false;

  const std::string* primary_type = typed_data->FindStringKey("primaryType");
  if (!primary_type)
    return false;

  const base::Value* domain = typed_data->FindKey("domain");
  if (!domain)
    return false;

  const base::Value* message = typed_data->FindKey("message");
  if (!message)
    return false;

  *address = *address_str;
  if (!base::JSONWriter::Write(*message, message_out))
    return false;

  const base::Value* types = typed_data->FindKey("types");
  if (!types)
    return false;
  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(*types, version);
  if (!helper)
    return false;

  auto message_to_sign =
      helper->GetTypedDataMessageToSign(*primary_type, *message, *domain);
  if (!message_to_sign)
    return false;
  *message_to_sign_out = *message_to_sign;

  *domain_out = domain->Clone();

  return true;
}

bool ParseSwitchEthereumChainParams(const std::string& json,
                                    std::string* chain_id) {
  if (!chain_id)
    return false;

  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj || !param_obj->is_dict())
    return false;

  const std::string* chain_id_str = param_obj->FindStringKey("chainId");
  if (!chain_id_str)
    return false;

  if (!IsValidHexString(*chain_id_str))
    return false;

  *chain_id = *chain_id_str;

  return true;
}

}  // namespace brave_wallet
