/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"

#include <memory>
#include <utility>

#include "base/notreached.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/services/storage/public/mojom/local_storage_control.mojom.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/storage_partition.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/rrect_f.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "base/json/json_reader.h"

VpnLoginStatusDelegate::VpnLoginStatusDelegate() = default;
VpnLoginStatusDelegate::~VpnLoginStatusDelegate() = default;

constexpr char kSkuOrigin[] = "https://account.brave.software/";

void VpnLoginStatusDelegate::OnGetAll(
    std::vector<blink::mojom::KeyValuePtr> out_data) {
  LOG(ERROR) << "BSC]] OnGetAll (" << out_data.size() << " entries)";

  for (size_t i = 0; i < out_data.size(); i++) {
    std::string the_key = "";
    std::string the_value = "";
    for (size_t j = 1; j < out_data[i]->key.size(); j++) {
      the_key += ((char)out_data[i]->key[j]);
    }
    for (size_t j = 1; j < out_data[i]->value.size(); j++) {
      the_value += ((char)out_data[i]->value[j]);
    }
    LOG(ERROR) << "BSC]] KEY: `" << the_key << "`";
    LOG(ERROR) << "BSC]] VALUE: `" << the_value << "`";

    storage_area_remote_.reset();

    base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          the_value, base::JSONParserOptions::JSON_PARSE_RFC);
    absl::optional<base::Value>& records_v = value_with_error.value;
    if (!records_v) {
      LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << the_value;
      return;
    }
  }
}

void VpnLoginStatusDelegate::LoadingStateChanged(content::WebContents* source,
                                                 bool to_different_document) {
  if (!source->IsLoading()) {
    auto* main_frame = source->GetMainFrame();
    auto* storage = main_frame->GetStoragePartition();
    auto* lsc = storage->GetLocalStorageControl();

    lsc->BindStorageArea(
        url::Origin::Create(GURL(kSkuOrigin)),
        storage_area_remote_.BindNewPipeAndPassReceiver());

    storage_area_remote_->GetAll(
        mojo::NullRemote(), base::BindOnce(&VpnLoginStatusDelegate::OnGetAll,
                                           base::Unretained(this)));
  }
}

namespace {

constexpr int kButtonRadius = 47;

class BraveVPNButtonHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  explicit BraveVPNButtonHighlightPathGenerator(const gfx::Insets& insets)
      : HighlightPathGenerator(insets) {}

  BraveVPNButtonHighlightPathGenerator(
      const BraveVPNButtonHighlightPathGenerator&) = delete;
  BraveVPNButtonHighlightPathGenerator& operator=(
      const BraveVPNButtonHighlightPathGenerator&) = delete;

  // views::HighlightPathGenerator overrides:
  absl::optional<gfx::RRectF> GetRoundRect(const gfx::RectF& rect) override {
    return gfx::RRectF(rect, kButtonRadius);
  }
};

}  // namespace

BraveVPNButton::BraveVPNButton(Profile* profile)
    : ToolbarButton(base::BindRepeating(&BraveVPNButton::OnButtonPressed,
                                        base::Unretained(this))),
      service_(BraveVpnServiceFactory::GetForProfile(profile)),
      webui_bubble_manager_(this, profile, GURL(kVPNPanelURL), 1, true) {
  DCHECK(service_);
  observation_.Observe(service_);

  // Replace ToolbarButton's highlight path generator.
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveVPNButtonHighlightPathGenerator>(
                GetToolbarInkDropInsets(this)));

  label()->SetText(
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_TOOLBAR_BUTTON_TEXT));
  gfx::FontList font_list = views::Label::GetDefaultFontList();
  constexpr int kFontSize = 12;
  label()->SetFontList(
      font_list.DeriveWithSizeDelta(kFontSize - font_list.GetFontSize()));

  // Set image positions first. then label.
  SetHorizontalAlignment(gfx::ALIGN_LEFT);

  UpdateButtonState();

  // USED TO CHECK IF THEY ARE LOGGED IN LOL
  content::WebContents::CreateParams params(profile);
  contents_ = content::WebContents::Create(params);
  contents_delegate_.reset(new VpnLoginStatusDelegate);
  contents_->SetDelegate(contents_delegate_.get());
}
// TODO(bsclifton): clean up contents
BraveVPNButton::~BraveVPNButton() = default;

void BraveVPNButton::OnConnectionStateChanged(bool connected) {
  UpdateButtonState();
}

void BraveVPNButton::OnConnectionCreated() {
  // Do nothing.
}

void BraveVPNButton::OnConnectionRemoved() {
  // Do nothing.
}

void BraveVPNButton::UpdateColorsAndInsets() {
  if (const auto* tp = GetThemeProvider()) {
    const gfx::Insets paint_insets =
        gfx::Insets((height() - GetLayoutConstant(LOCATION_BAR_HEIGHT)) / 2);
    SetBackground(views::CreateBackgroundFromPainter(
        views::Painter::CreateSolidRoundRectPainter(
            tp->GetColor(ThemeProperties::COLOR_TOOLBAR), kButtonRadius,
            paint_insets)));

    SetEnabledTextColors(tp->GetColor(
        IsConnected()
            ? BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_CONNECTED
            : BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_DISCONNECTED));

    std::unique_ptr<views::Border> border = views::CreateRoundedRectBorder(
        1, kButtonRadius, gfx::Insets(),
        tp->GetColor(BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_BORDER));
    constexpr gfx::Insets kTargetInsets{4, 6};
    const gfx::Insets extra_insets = kTargetInsets - border->GetInsets();
    SetBorder(views::CreatePaddedBorder(std::move(border), extra_insets));
  }

  constexpr int kBraveAvatarImageLabelSpacing = 4;
  SetImageLabelSpacing(kBraveAvatarImageLabelSpacing);
}

void BraveVPNButton::UpdateButtonState() {
  constexpr SkColor kColorConnected = SkColorSetRGB(0x51, 0xCF, 0x66);
  constexpr SkColor kColorDisconnected = SkColorSetRGB(0xAE, 0xB1, 0xC2);
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(kVpnIndicatorIcon, IsConnected()
                                                        ? kColorConnected
                                                        : kColorDisconnected));
}

bool BraveVPNButton::IsConnected() {
  return service_->is_connected();
}

void BraveVPNButton::OnButtonPressed(const ui::Event& event) {
  ShowBraveVPNPanel();

  if (contents_) {
    std::string endpoint_url = std::string(kSkuOrigin);
    endpoint_url.append("skus/");

    GURL url = GURL(endpoint_url.c_str());
    std::string extra_headers =
        "Authorization: Basic BASE64_ENCODED_USER:PASSWORD_HERE";
    contents_->GetController().LoadURL(url, content::Referrer(),
                                       ui::PAGE_TRANSITION_TYPED,
                                       extra_headers);
  }
}

void BraveVPNButton::ShowBraveVPNPanel() {
  if (webui_bubble_manager_.GetBubbleWidget()) {
    webui_bubble_manager_.CloseBubble();
    return;
  }

  webui_bubble_manager_.ShowBubble();
}

BEGIN_METADATA(BraveVPNButton, LabelButton)
END_METADATA
