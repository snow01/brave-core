// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"

#include <string>
#include <utility>

#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"
#include "brave/browser/ui/webui/new_tab_page/top_sites_message_handler.h"
#include "brave/components/brave_new_tab/resources/grit/brave_new_tab_generated_map.h"
#include "brave/components/brave_today/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/components/dh_news_ffi/browser/brave_news_controller.h"
#endif

BraveNewTabUI::BraveNewTabUI(content::WebUI* web_ui, const std::string& name)
    : ui::MojoWebUIController(
          web_ui,
          true /* Needed for legacy non-mojom message handler */) {
  Profile* profile = Profile::FromWebUI(web_ui);

  if (brave::ShouldNewTabShowBlankpage(profile)) {
    content::WebUIDataSource* source =
        content::WebUIDataSource::Create(name);
    source->SetDefaultResource(IDR_BRAVE_BLANK_NEW_TAB_HTML);
    content::WebUIDataSource::Add(profile, source);
  } else {
    content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
        web_ui, name, kBraveNewTabGenerated, kBraveNewTabGeneratedSize,
        IDR_BRAVE_NEW_TAB_HTML);
    web_ui->AddMessageHandler(
        base::WrapUnique(BraveNewTabMessageHandler::Create(source, profile)));
    web_ui->AddMessageHandler(
        base::WrapUnique(new TopSitesMessageHandler(profile)));
  }

  web_ui->OverrideTitle(l10n_util::GetStringUTF16(IDS_NEW_TAB_TITLE));
}

BraveNewTabUI::~BraveNewTabUI() {
}

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
void BraveNewTabUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  DCHECK(profile);
  // Wire up JS mojom to service
  auto* brave_news_controller =
      brave_news::BraveNewsControllerFactory::GetForContext(profile);
  if (brave_news_controller) {
    brave_news_controller->Bind(std::move(receiver));
  }
}
#endif

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewTabUI)
