/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "base/containers/flat_map.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsP3ABrowserTest.*

namespace rewards_browsertest {

using namespace std::chrono_literals;

class RewardsP3ABrowserTest : public InProcessBrowserTest,
                              public brave_rewards::RewardsServiceObserver {
 public:
  RewardsP3ABrowserTest() {
    contribution_ = std::make_unique<RewardsBrowserTestContribution>();
    promotion_ = std::make_unique<RewardsBrowserTestPromotion>();
    response_ = std::make_unique<RewardsBrowserTestResponse>();
    histogram_tester_.reset(new base::HistogramTester);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<RewardsBrowserTestContextHelper>(browser());

    // HTTP resolver
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&rewards_browsertest_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile));
    rewards_service_->AddObserver(this);

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(base::BindRepeating(
        &RewardsP3ABrowserTest::GetTestResponse, base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    // Other
    promotion_->Initialize(browser(), rewards_service_);
    contribution_->Initialize(browser(), rewards_service_);

    rewards_browsertest_util::SetOnboardingBypassed(browser());
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void GetTestResponse(const std::string& url,
                       int32_t method,
                       int* response_status_code,
                       std::string* response,
                       base::flat_map<std::string, std::string>* headers) {
    response_->SetExternalBalance(contribution_->GetExternalBalance());
    response_->Get(url, method, response_status_code, response);
  }

  void FetchBalance() {
    base::RunLoop run_loop;
    rewards_service_->FetchBalance(base::BindLambdaForTesting(
        [&](ledger::type::Result result, ledger::type::BalancePtr balance) {
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void WaitForRewardsInitialization() {
    if (rewards_initialized_) {
      return;
    }

    wait_for_rewards_initialization_loop_.reset(new base::RunLoop);
    wait_for_rewards_initialization_loop_->Run();
  }

  // Disabling ads happens synchronously, but enable needs to round-trip
  // through the ledger utility process. Wait until changes have propagated.
  void WaitForRewardsEnable() {
    if (ads_enabled_) {
      std::cerr << "WaitForRewardsEnable - already enabled\n";
      return;
    }

    PrefService* prefs = browser()->profile()->GetPrefs();
    // TODO (rillian) It would be much better to have a deterministic
    // way to continue here. Calling `sleep` slows down the test, and
    // it's hard to know how long to wait.
    auto tries = 0;
    while (!prefs->GetBoolean("brave.brave_ads.enabled")) {
      std::cerr << "  WaitForRewardsEnable sleeping 200 milliseconds...\n";
      std::this_thread::sleep_for(200ms);
      std::cerr << "  WaitForRewardsEnable spinning message loop...\n";
      content::RunAllPendingInMessageLoop();
      // The state change should propagate relatively quickly. Don't
      // wait for the overall timeout if we're hung.
      if (++tries > 5) {
        FAIL() << "Timeout waiting for Brave Ads to respond.";
      }
    }
    std::cerr << "  WaitForRewardsEnable change has propagated.\n";
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void OnRewardsInitialized(
      brave_rewards::RewardsService* rewards_service) override {
    rewards_initialized_ = true;
  }

  void OnAdsEnabled(brave_rewards::RewardsService* service,
      bool ads_enabled) override {
    ads_enabled_ = ads_enabled;
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  bool rewards_initialized_ = false;
  std::unique_ptr<base::RunLoop> wait_for_rewards_initialization_loop_;
  bool ads_enabled_ = false;
};

using brave_rewards::p3a::RewardsEnabledDuration;

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, RewardsDisabled) {
  rewards_browsertest_util::StartProcess(rewards_service_);

  WaitForRewardsInitialization();

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletBalance.2", 1, 1);
  histogram_tester_->ExpectBucketCount("Brave.Rewards.AutoContributionsState.2",
                                       1, 1);
  histogram_tester_->ExpectBucketCount("Brave.Rewards.TipsState.2", 1, 1);
  histogram_tester_->ExpectBucketCount("Brave.Rewards.EnabledDuration",
        RewardsEnabledDuration::kNever, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, RewardsEnabledDuration) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  // Access prefs so we can back-date records.
  std::cerr << "Fetching prefs...\n";
  PrefService* prefs = browser()->profile()->GetPrefs();
  std::cerr << brave_rewards::prefs::kEnabledTimestamp << " = "
    << prefs->GetTime(brave_rewards::prefs::kEnabledTimestamp) << std::endl;

  std::cerr << "Waiting for Rewards initialization...";
  WaitForRewardsInitialization();
  std::cerr << " ok\n";

  // Rewards have never been enabled.
  std::cerr << "Checking initial state...";
  histogram_tester_->ExpectBucketCount("Brave.Rewards.EnabledDuration",
      RewardsEnabledDuration::kNever, 1);
  std::cerr << " ok\n";

  // Turn rewards on.
  std::cerr << "Enabling rewards...\n";
  rewards_service_->SetAdsEnabled(true);
  rewards_service_->SetAutoContributeEnabled(true);

  WaitForRewardsEnable();
  ASSERT_TRUE(prefs->GetBoolean("brave.brave_ads.enabled"));

  // Expect two measurements of the same value since we record for
  // both the ads and autocontribute preference changes.
  histogram_tester_->ExpectBucketCount("Brave.Rewards.EnabledDuration",
      RewardsEnabledDuration::kStillEnabled, 2);

  // Turn rewards off. Should record the shortest duration.
  std::cerr << "Disabling rewards...\n";
  rewards_service_->SetAdsEnabled(false);
  rewards_service_->SetAutoContributeEnabled(false);
  // FIXME: This doesn't work. Ad disable is dispatched synchonously,
  // but RewardsServiceImpl::IsRewardsEnabled() returns true as long
  // as AutoContribute is still enabled, and so we record kStillEnabled
  // there. Disabling AutoContribute is async, but if we spin the
  // message loop, it DCHECKs because it's being shut down before it
  // finished initializing.
  WaitForRewardsInitialization();
  ASSERT_FALSE(prefs->GetBoolean("brave.brave_ads.enabled"));
      std::cerr << "  Waiting 500 ms...\n";
      std::this_thread::sleep_for(500ms);
      std::cerr << "  Spinning message loop...\n";
      // DCHECK because AutoCrontribute isn't initialized.
      content::RunAllPendingInMessageLoop();
  ASSERT_FALSE(prefs->GetBoolean("brave.rewards.ac.enabled"));
  histogram_tester_->ExpectBucketCount("Brave.Rewards.EnabledDuration",
      RewardsEnabledDuration::kHours, 1);

  // Turn rewards on.
  std::cerr << "Enabling rewards...\n";
  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);
  WaitForRewardsEnable();
  ASSERT_TRUE(prefs->GetBoolean("brave.brave_ads.enabled"));
  // We record kStillEnabled each time we turn rewards on,
  // so the expected count increments over what we saw the
  // the last time.
  histogram_tester_->ExpectBucketCount("Brave.Rewards.EnabledDuration",
      RewardsEnabledDuration::kStillEnabled, 4);

  // Adjust the stored timestamp to measure a longer duration.
  auto yesterday = base::Time::Now() - base::TimeDelta::FromDays(1);
  std::cerr << "Backdating timestamp to " << yesterday << std::endl;
  prefs->SetTime(brave_rewards::prefs::kEnabledTimestamp, yesterday);
  std::cerr << "Disabling rewards...\n";
  rewards_service_->SetAutoContributeEnabled(false);
  rewards_service_->SetAdsEnabled(false);
  histogram_tester_->ExpectBucketCount("Brave.Rewards.EnabledDuration",
      RewardsEnabledDuration::kDays, 1);

  std::cerr << "--- END of RewardsEnabledDuration browser test ---\n";
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedNoGrantsClaimedNoFundsAdded) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletState", 1, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedGrantsClaimedNoFundsAdded) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  FetchBalance();

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletState", 2, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedNoGrantsClaimedFundsAdded) {
  response_->SetUserFundsBalance(20.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(histogram_tester_->GetBucketCount("Brave.Rewards.WalletState", 3),
            0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedGrantsClaimedFundsAdded) {
  response_->SetUserFundsBalance(20.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  FetchBalance();

  EXPECT_GT(histogram_tester_->GetBucketCount("Brave.Rewards.WalletState", 4),
            0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletDisabledAfterCreation) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  rewards_service_->SetAdsEnabled(false);
  rewards_service_->SetAutoContributeEnabled(false);

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletState", 5, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, WalletBalanceLessThan10BAT) {
  response_->SetUserFundsBalance(9.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(
      histogram_tester_->GetBucketCount("Brave.Rewards.WalletBalance.2", 2), 0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, WalletBalanceLessThan50BAT) {
  response_->SetUserFundsBalance(20.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(
      histogram_tester_->GetBucketCount("Brave.Rewards.WalletBalance.2", 3), 0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, WalletBalanceMoreThan50BAT) {
  response_->SetUserFundsBalance(60.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(
      histogram_tester_->GetBucketCount("Brave.Rewards.WalletBalance.2", 4), 0);
}

}  // namespace rewards_browsertest
