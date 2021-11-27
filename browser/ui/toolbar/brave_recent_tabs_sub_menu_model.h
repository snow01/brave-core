/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_RECENT_TABS_SUB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_RECENT_TABS_SUB_MENU_MODEL_H_

#include "brave/base/macros.h"
#include "chrome/browser/ui/toolbar/recent_tabs_sub_menu_model.h"

class Browser;

namespace ui {
class AcceleratorProvider;
}

class BraveRecentTabsSubMenuModel : public RecentTabsSubMenuModel {
 public:
  BraveRecentTabsSubMenuModel(ui::AcceleratorProvider* accelerator_provider,
                         Browser* browser);

  ~BraveRecentTabsSubMenuModel() override;

  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveRecentTabsSubMenuModel);
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_RECENT_TABS_SUB_MENU_MODEL_H_
