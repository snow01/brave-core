/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;

import androidx.annotation.NonNull;
import androidx.core.app.NotificationCompat;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;

import org.json.JSONException;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.CollectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.UnownedUserDataSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ApplicationLifetime;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveHelper;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveSyncInformers;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.CrossPromotionalModalDialogFragment;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.SetDefaultBrowserActivity;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.browsing_data.BrowsingDataBridge;
import org.chromium.chrome.browser.browsing_data.BrowsingDataType;
import org.chromium.chrome.browser.browsing_data.ClearBrowsingDataFragmentAdvanced;
import org.chromium.chrome.browser.browsing_data.TimePeriod;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.layouts.Layout;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerChrome;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.compositor.layouts.phone.StackLayout;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.dependency_injection.ChromeActivityComponent;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.informers.BraveAndroidSyncDisabledInformer;
import org.chromium.chrome.browser.notifications.BraveSetDefaultBrowserNotificationService;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.onboarding.BraveTalkOptInPopupListener;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;
import org.chromium.chrome.browser.onboarding.OnboardingActivity;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.v2.HighlightDialogFragment;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.rate.RateDialogFragment;
import org.chromium.chrome.browser.rate.RateUtils;
import org.chromium.chrome.browser.settings.BraveRewardsPreferences;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.share.ShareDelegate.ShareOrigin;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl;
import org.chromium.chrome.browser.util.BraveDbUtil;
import org.chromium.chrome.browser.util.BraveReferrer;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.vpn.BraveVpnCalloutDialogFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.InAppPurchaseWrapper;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.widget.Toast;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;

/**
 * Brave's extension for ChromeActivity
 */
@JNINamespace("chrome::android")
public abstract class BraveActivity<C extends ChromeActivityComponent> extends ChromeActivity
        implements BrowsingDataBridge.OnClearBrowsingDataListener, BraveVpnObserver {
    public static final int SITE_BANNER_REQUEST_CODE = 33;
    public static final int VERIFY_WALLET_ACTIVITY_REQUEST_CODE = 34;
    public static final int USER_WALLET_ACTIVITY_REQUEST_CODE = 35;
    public static final String ADD_FUNDS_URL = "chrome://rewards/#add-funds";
    public static final String REWARDS_SETTINGS_URL = "chrome://rewards/";
    public static final String BRAVE_REWARDS_SETTINGS_URL = "brave://rewards/";
    public static final String REWARDS_AC_SETTINGS_URL = "chrome://rewards/contribute";
    public static final String REWARDS_LEARN_MORE_URL = "https://brave.com/faq-rewards/#unclaimed-funds";
    public static final String BRAVE_TERMS_PAGE =
            "https://basicattentiontoken.org/user-terms-of-service/";
    public static final String P3A_URL = "https://brave.com/p3a";
    public static final String BRAVE_PRIVACY_POLICY = "https://brave.com/privacy/#rewards";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_CLEAR_ON_EXIT = "clear_on_exit";
    public static final String OPEN_URL = "open_url";

    public static final String BRAVE_PRODUCTION_PACKAGE_NAME = "com.brave.browser";
    public static final String BRAVE_BETA_PACKAGE_NAME = "com.brave.browser_beta";
    public static final String BRAVE_NIGHTLY_PACKAGE_NAME = "com.brave.browser_nightly";

    private static final int DAYS_1 = 1;
    private static final int DAYS_4 = 4;
    private static final int DAYS_5 = 5;
    private static final int DAYS_12 = 12;

    /**
     * Settings for sending local notification reminders.
     */
    public static final String CHANNEL_ID = "com.brave.browser";
    public static final String ANDROID_SETUPWIZARD_PACKAGE_NAME = "com.google.android.setupwizard";
    public static final String ANDROID_PACKAGE_NAME = "android";
    public static final String BRAVE_BLOG_URL = "https://brave.com/privacy-features/";

    // Explicitly declare this variable to avoid build errors.
    // It will be removed in asm and parent variable will be used instead.
    private UnownedUserDataSupplier<BrowserControlsManager> mBrowserControlsManagerSupplier;

    private static final List<String> yandexRegions =
            Arrays.asList("AM", "AZ", "BY", "KG", "KZ", "MD", "RU", "TJ", "TM", "UZ");

    private String mPurchaseToken = "";
    private String mProductId = "";
    private boolean mIsVerification;
    public CompositorViewHolder compositorView;
    public View inflatedSettingsBarLayout;
    public String test;

    @SuppressLint("VisibleForTests")
    public BraveActivity() {
        // Disable key checker to avoid asserts on Brave keys in debug
        SharedPreferencesManager.getInstance().disableKeyCheckerForTesting();
    }
    
    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        Log.d("bn", "ondetach onResumeWithNative");
        BraveActivityJni.get().restartStatsUpdater();
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            InAppPurchaseWrapper.getInstance().startBillingServiceConnection(BraveActivity.this);
            BraveVpnNativeWorker.getInstance().addObserver(this);
        }
    }

    @Override
    public void onPauseWithNative() {
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            BraveVpnNativeWorker.getInstance().removeObserver(this);
        }
        Log.d("bn", "ondetach onPauseWithNative");
        super.onPauseWithNative();
    }

    private NewTabPageListener newTabPageListener;

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        final TabImpl currentTab = (TabImpl) getActivityTab();
        // Handle items replaced by Brave.
        if (id == R.id.info_menu_id && currentTab != null) {
            ShareDelegate shareDelegate = (ShareDelegate) getShareDelegateSupplier().get();
            shareDelegate.share(currentTab, false, ShareOrigin.OVERFLOW_MENU);
            return true;
        }

        if (super.onMenuOrKeyboardAction(id, fromMenu)) {
            return true;
        }

        // Handle items added by Brave.
        if (currentTab == null) {
            return false;
        } else if (id == R.id.exit_id) {
            ApplicationLifetime.terminate(false);
        } else if (id == R.id.set_default_browser) {
            handleBraveSetDefaultBrowserDialog();
        } else if (id == R.id.brave_rewards_id) {
            openNewOrSelectExistingTab(REWARDS_SETTINGS_URL);
        } else if (id == R.id.brave_wallet_id) {
            openBraveWallet();
        } else if (id == R.id.request_brave_vpn_id || id == R.id.request_brave_vpn_check_id) {
            if (BraveVpnProfileUtils.getInstance().isVPNConnected(BraveActivity.this)) {
                BraveVpnUtils.showProgressDialog(
                        BraveActivity.this, getResources().getString(R.string.vpn_disconnect_text));
                BraveVpnProfileUtils.getInstance().stopVpn(BraveActivity.this);
            } else {
                BraveVpnUtils.showProgressDialog(
                        BraveActivity.this, getResources().getString(R.string.vpn_connect_text));
                if (BraveVpnPrefUtils.isSubscriptionPurchase()) {
                    verifySubscription();
                } else {
                    BraveVpnUtils.dismissProgressDialog();
                    BraveVpnUtils.openBraveVpnPlansActivity(BraveActivity.this);
                }
            }
        } else {
            return false;
        }

        return true;
    }

    private void verifySubscription() {
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            mPurchaseToken = purchase.getPurchaseToken();
            mProductId = purchase.getSkus().get(0).toString();
            BraveVpnNativeWorker.getInstance().verifyPurchaseToken(mPurchaseToken, mProductId,
                    BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
        } else {
            braveVpnVerificationFailed();
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(mPurchaseToken);
                BraveVpnPrefUtils.setProductId(mProductId);
                BraveVpnPrefUtils.setPurchaseExpiry(purchaseExpiry);
                BraveVpnPrefUtils.setSubscriptionPurchase(true);
                if (!mIsVerification) {
                    BraveVpnProfileUtils.getInstance().startStopVpn(BraveActivity.this);
                } else {
                    mIsVerification = false;
                }
            } else {
                braveVpnVerificationFailed();
            }
            mPurchaseToken = "";
            mProductId = "";
        }
    };

    private void braveVpnVerificationFailed() {
        BraveVpnPrefUtils.setPurchaseToken("");
        BraveVpnPrefUtils.setProductId("");
        BraveVpnPrefUtils.setPurchaseExpiry(0L);
        BraveVpnPrefUtils.setSubscriptionPurchase(false);
        if (BraveVpnProfileUtils.getInstance().isVPNConnected(BraveActivity.this)) {
            BraveVpnProfileUtils.getInstance().stopVpn(BraveActivity.this);
        }
        BraveVpnProfileUtils.getInstance().deleteVpnProfile(BraveActivity.this);
        Toast.makeText(BraveActivity.this, R.string.purchase_token_verification_failed,
                     Toast.LENGTH_LONG)
                .show();
        BraveVpnUtils.openBraveVpnPlansActivity(BraveActivity.this);
    }

    @Override
    public void initializeState() {
        super.initializeState();
        Log.d("bn", "ondetach initializeState  ");
        if (isNoRestoreState()) {
            CommandLine.getInstance().appendSwitch(ChromeSwitches.NO_RESTORE_STATE);
        }

        if (isClearBrowsingDataOnExit()) {
            List<Integer> dataTypes = Arrays.asList(
                    BrowsingDataType.HISTORY, BrowsingDataType.COOKIES, BrowsingDataType.CACHE);

            int[] dataTypesArray = CollectionUtil.integerListToIntArray(new ArrayList<>(dataTypes));

            // has onBrowsingDataCleared() as an @Override callback from implementing
            // BrowsingDataBridge.OnClearBrowsingDataListener
            BrowsingDataBridge.getInstance().clearBrowsingData(
                    this, dataTypesArray, TimePeriod.ALL_TIME);
        }

        setLoadedFeed(false);
        setNewsItemsFeedCards(null);
        // setNewsFeedScrollPosition(-1);
        BraveSearchEngineUtils.initializeBraveSearchEngineStates(getTabModelSelector());
    }

    public boolean loadedFeed;
    public CopyOnWriteArrayList<FeedItemsCard> newsItemsFeedCards;
    public int newsFeedScrollPosition;

    public boolean isLoadedFeed() {
        return loadedFeed;
    }

    public void setLoadedFeed(boolean loadedFeed) {
        this.loadedFeed = loadedFeed;
    }

    public CopyOnWriteArrayList<FeedItemsCard> getNewsItemsFeedCards() {
        // Log.d("bn", "persistencetest getNewsItemsFeedCards:"+newsItemsFeedCards);
        return newsItemsFeedCards;
    }

    public void setNewsItemsFeedCards(CopyOnWriteArrayList<FeedItemsCard> newsItemsFeedCards) {
        // Log.d("bn", "persistencetest setNewsItemsFeedCards:"+newsItemsFeedCards);
        this.newsItemsFeedCards = newsItemsFeedCards;
        // getNewsItemsFeedCards();
    }

    public int getNewsFeedScrollPosition() {
        return newsFeedScrollPosition;
    }

    public void setNewsFeedScrollPosition(int newsFeedScrollPosition) {
        this.newsFeedScrollPosition = newsFeedScrollPosition;
    }


    public int getImageCreditLayoutBottom() {
        ViewGroup imageCreditLayout = findViewById(R.id.image_credit_layout);
        if (imageCreditLayout != null) {
            return imageCreditLayout.getBottom();
        }

        return ConfigurationUtils.getDisplayMetrics(this).get("height");
    }

    @Override
    public void onBrowsingDataCleared() {}

    @Override
    public void onResume() {
        Log.d("bn", "ondetach onresume  ");
        super.onResume();

        Tab tab = getActivityTab();
        if (tab == null)
            return;

        // Set proper active DSE whenever brave returns to foreground.
        // If active tab is private, set private DSE as an active DSE.
        BraveSearchEngineUtils.updateActiveDSE(tab.isIncognito());
        BraveStatsUtil.removeShareStatsFile();
    }

    @Override
    public void onPause() {
        super.onPause();
        Tab tab = getActivityTab();
        if (tab == null)
            return;

        // Set normal DSE as an active DSE when brave goes in background
        // because currently set DSE is used by outside of brave(ex, brave search widget).
        if (tab.isIncognito()) {
            BraveSearchEngineUtils.updateActiveDSE(false);
        }
    }

    @Override
    public void performPostInflationStartup() {
        super.performPostInflationStartup();

        createNotificationChannel();
        setupBraveSetDefaultBrowserNotification();
    }

    @Override
    protected void initializeStartupMetrics() {
        super.initializeStartupMetrics();

        // Disable FRE for arm64 builds where ChromeActivity is the one that
        // triggers FRE instead of ChromeLauncherActivity on arm32 build.
        BraveHelper.DisableFREDRP();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        Log.d("bn", "ondetach finishNativeInitialization  ");
        if (SharedPreferencesManager.getInstance().readBoolean(
                    BravePreferenceKeys.BRAVE_DOUBLE_RESTART, false)) {
            SharedPreferencesManager.getInstance().writeBoolean(
                    BravePreferenceKeys.BRAVE_DOUBLE_RESTART, false);
            BraveRelaunchUtils.restart();
            return;
        }

        // Make sure this option is disabled
        if (PrivacyPreferencesManagerImpl.getInstance().getNetworkPredictionEnabled()) {
            PrivacyPreferencesManagerImpl.getInstance().setNetworkPredictionEnabled(false);
        }

        if (BraveRewardsHelper.hasRewardsEnvChange()) {
            BravePrefServiceBridge.getInstance().resetPromotionLastFetchStamp();
            BraveRewardsHelper.setRewardsEnvChange(false);
        }

        int appOpenCount = SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT);
        SharedPreferencesManager.getInstance().writeInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT, appOpenCount + 1);

        if (PackageUtils.isFirstInstall(this) && appOpenCount == 0) {
            checkForYandexSE();
        }

        //set bg ads to off for existing and new installations
        setBgBraveAdsDefaultOff();

        Context app = ContextUtils.getApplicationContext();
        if (null != app
                && BraveReflectionUtil.EqualTypes(this.getClass(), ChromeTabbedActivity.class)) {
            // Trigger BraveSyncWorker CTOR to make migration from sync v1 if sync is enabled
            BraveSyncWorker.get();
        }

        checkForNotificationData();

        if (!RateUtils.getInstance(this).getPrefRateEnabled()) {
            RateUtils.getInstance(this).setPrefRateEnabled(true);
            RateUtils.getInstance(this).setNextRateDateAndCount();
        }

        if (RateUtils.getInstance(this).shouldShowRateDialog())
            showBraveRateDialog();

        int visitedNewsCardsCount = SharedPreferencesManager.getInstance().readInt(
                BravePreferenceKeys.BRAVE_NEWS_CARDS_VISITED);
        Log.d("bn", "visitedNewsCardsCount:" + visitedNewsCardsCount);

        // for Brave Newsa initialize the no. of cards to 0
        SharedPreferencesManager.getInstance().writeInt(
                BravePreferenceKeys.BRAVE_NEWS_CARDS_VISITED, 0);
        SharedPreferencesManager.getInstance().writeInt(
                BravePreferenceKeys.BRAVE_NEWS_CARDS_VIEWED, 0);

        SharedPreferencesManager.getInstance().writeInt(
                BravePreferenceKeys.BRAVE_NEWS_PROMOTION_CARDS_VISITED, 0);
        SharedPreferencesManager.getInstance().writeInt(
                BravePreferenceKeys.BRAVE_NEWS_PROMOTION_CARDS_VIEWED, 0);

        SharedPreferencesManager.getInstance().writeInt(
                BravePreferenceKeys.BRAVE_NEWS_DISPLAYAD_CARDS_VISITED, 0);
        SharedPreferencesManager.getInstance().writeInt(
                BravePreferenceKeys.BRAVE_NEWS_DISPLAYAD_CARDS_VIEWED, 0);

        // TODO commenting out below code as we may use it in next release

        // if (PackageUtils.isFirstInstall(this)
        //         &&
        //         SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
        //         == 1) {
        //     Calendar calender = Calendar.getInstance();
        //     calender.setTime(new Date());
        //     calender.add(Calendar.DATE, DAYS_4);
        //     OnboardingPrefManager.getInstance().setNextOnboardingDate(
        //         calender.getTimeInMillis());
        // }

        // OnboardingActivity onboardingActivity = null;
        // for (Activity ref : ApplicationStatus.getRunningActivities()) {
        //     if (!(ref instanceof OnboardingActivity)) continue;

        //     onboardingActivity = (OnboardingActivity) ref;
        // }

        // if (onboardingActivity == null
        //         && OnboardingPrefManager.getInstance().showOnboardingForSkip(this)) {
        //     OnboardingPrefManager.getInstance().showOnboarding(this);
        //     OnboardingPrefManager.getInstance().setOnboardingShownForSkip(true);
        // }

        if (SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT) == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_12);
            OnboardingPrefManager.getInstance().setNextCrossPromoModalDate(
                calender.getTimeInMillis());
        }

        if (OnboardingPrefManager.getInstance().showCrossPromoModal()) {
            showCrossPromotionalDialog();
            OnboardingPrefManager.getInstance().setCrossPromoModalShown(true);
        }
        BraveSyncInformers.show();
        BraveAndroidSyncDisabledInformer.showInformers();

        if (!OnboardingPrefManager.getInstance().isOneTimeNotificationStarted()
                && PackageUtils.isFirstInstall(this)) {
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.HOUR_3);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.HOUR_24);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_6);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_10);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_30);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_35);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_1);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_2);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_3);
            OnboardingPrefManager.getInstance().setOneTimeNotificationStarted(true);
        }
        if (!TextUtils.isEmpty(BinanceWidgetManager.getInstance().getBinanceAccountBalance())) {
            try {
                BinanceWidgetManager.binanceAccountBalance = new BinanceAccountBalance(
                        BinanceWidgetManager.getInstance().getBinanceAccountBalance());
            } catch (JSONException e) {
                Log.e("NTP", e.getMessage());
            }
        }

        if (PackageUtils.isFirstInstall(this)
                && SharedPreferencesManager.getInstance().readInt(
                           BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_4);
            BraveRewardsHelper.setNextRewardsOnboardingModalDate(calender.getTimeInMillis());
        }
        if (BraveRewardsHelper.shouldShowRewardsOnboardingModalOnDay4()) {
            BraveRewardsHelper.setShowBraveRewardsOnboardingModal(true);
            openRewardsPanel();
            BraveRewardsHelper.setRewardsOnboardingModalShown(true);
        }

        if (SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_5);
            OnboardingPrefManager.getInstance().setNextSetDefaultBrowserModalDate(
                    calender.getTimeInMillis());
        }
        checkSetDefaultBrowserModal();
        checkFingerPrintingOnUpgrade();
        compositorView = null;
        inflatedSettingsBarLayout = null;
        Tab tab = getActivityTab();
        if (tab != null){
            // // if it's new tab add the brave news settings bar to the layout
            Log.d("bn", "inflateNewsSettingsBar tab:"+tab);
            Log.d("bn", "inflateNewsSettingsBar tab.getUrl():"+tab.getUrl()+"tab.getUrl().getSpec()"+tab.getUrl().getSpec()+"isntp:"+UrlUtilities.isNTPUrl(tab.getUrl().getSpec()));
            if (tab != null && tab.getUrl().getSpec() != null
                    && UrlUtilities.isNTPUrl(tab.getUrl().getSpec())
                    && BravePrefServiceBridge.getInstance().getNewsOptIn()) {
                inflateNewsSettingsBar();
            } else {
                Log.d("bn", "inflateNewsSettingsBar else remove move it");
                removeSetttingsBar();
            } 
        } else {
            Log.d("bn", "tab is null");
        }

        Log.d("BN", "lifecycle BraveActivity finishNativeInitialization");

        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            ConnectivityManager connectivityManager =
                    (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkRequest networkRequest =
                    new NetworkRequest.Builder()
                            .addTransportType(NetworkCapabilities.TRANSPORT_VPN)
                            .removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN)
                            .build();
            connectivityManager.registerNetworkCallback(networkRequest, mNetworkCallback);

            if (BraveVpnPrefUtils.shouldShowCallout() && !BraveVpnPrefUtils.isSubscriptionPurchase()
                            && (SharedPreferencesManager.getInstance().readInt(
                                        BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                                            == 1
                                    && !PackageUtils.isFirstInstall(this))
                    || (SharedPreferencesManager.getInstance().readInt(
                                BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                                    == 7
                            && PackageUtils.isFirstInstall(this))) {
                showVpnCalloutDialog();
            }

            if (!TextUtils.isEmpty(BraveVpnPrefUtils.getPurchaseToken())
                    && !TextUtils.isEmpty(BraveVpnPrefUtils.getProductId())) {
                mIsVerification = true;
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                        BraveVpnPrefUtils.getPurchaseToken(), BraveVpnPrefUtils.getProductId(),
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
            }
        }
    }

    private final ConnectivityManager.NetworkCallback mNetworkCallback =
            new ConnectivityManager.NetworkCallback() {
                @Override
                public void onAvailable(Network network) {
                    BraveVpnUtils.showBraveVpnNotification(BraveActivity.this);
                    BraveVpnUtils.dismissProgressDialog();
                }

                @Override
                public void onLost(Network network) {
                    BraveVpnUtils.cancelBraveVpnNotification(BraveActivity.this);
                    BraveVpnUtils.dismissProgressDialog();
                }
            };

    private void showVpnCalloutDialog() {
        BraveVpnCalloutDialogFragment braveVpnCalloutDialogFragment =
                new BraveVpnCalloutDialogFragment();
        braveVpnCalloutDialogFragment.setCancelable(false);
        braveVpnCalloutDialogFragment.show(
                getSupportFragmentManager(), "BraveVpnCalloutDialogFragment");
    }

    public boolean newsSettingsBarInflated;

    public boolean isNewsSettingsBarInflated(){
        return this.newsSettingsBarInflated;
    }

    public void setNewsSettingsBarInflated(boolean newsSettingsBarInflated) {
        this.newsSettingsBarInflated = newsSettingsBarInflated;
    }

    public void inflateNewsSettingsBar() {
        // get the main compositor view that we'll use to manipulate the views
        compositorView = findViewById(R.id.compositor_view_holder);
        ViewGroup controlContainer = findViewById(R.id.control_container);
        // Log.d("bn", "inflateNewsSettingsBar compositorView: "+compositorView+" controlContainer: "+controlContainer + "already here: "+findViewById(R.id.news_settings_bar));
        if (findViewById(R.id.news_settings_bar) != null) {
            return;
        }
        if (compositorView != null && controlContainer != null){
            Log.d("bn", "controlContainer: " + controlContainer);
            Log.d("bn", "controlContainer: " + controlContainer.getBottom());
            int[] coords = {0, 0};
            controlContainer.getLocationOnScreen(coords);
            int absoluteTop = coords[1];
            int absoluteBottom = coords[1] + controlContainer.getHeight();
            Log.d("bn", "controlContainer absoluteBottom: " + absoluteBottom);
            Log.d("bn",
                    "controlContainer controlContainer.getHeight(): " + controlContainer.getHeight());

            for (int index = 0; index < ((ViewGroup) compositorView).getChildCount(); index++) {
                View nextChild = ((ViewGroup) compositorView).getChildAt(index);
                Log.d("bn",
                        "compositorViewchildren braveactivity inflate nextchild before add:"
                                + nextChild);
            }

            LayoutInflater inflater = LayoutInflater.from(this);
            // inflate the settings bar layout
            View inflatedSettingsBarLayout = inflater.inflate(R.layout.brave_news_settings_bar_layout, null);
            RelativeLayout newContentButtonLayout = (RelativeLayout) inflater.inflate(R.layout.brave_news_load_new_content, null);
            // add the bar to the layout stack
            compositorView.addView(inflatedSettingsBarLayout, 2);
            compositorView.addView(newContentButtonLayout, 3);
            inflatedSettingsBarLayout.setAlpha(0f);
            FrameLayout.LayoutParams inflatedLayoutParams =
                    new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, 100);

            FrameLayout.LayoutParams newContentButtonLayoutParams =
                    new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);

            // position bellow the control_container element (nevigation bar) with 25dp compensation
            inflatedLayoutParams.setMargins(0, controlContainer.getBottom() - 25, 0, 0);
            newContentButtonLayoutParams.setMargins(0, 400, 0, 0);
            newContentButtonLayoutParams.gravity = Gravity.CENTER_HORIZONTAL;
            newContentButtonLayout.setGravity(Gravity.CENTER_HORIZONTAL);
            inflatedSettingsBarLayout.setLayoutParams(inflatedLayoutParams);
            newContentButtonLayout.setLayoutParams(newContentButtonLayoutParams);

            inflatedSettingsBarLayout.setVisibility(View.VISIBLE);

            compositorView.invalidate();

            for (int index = 0; index < ((ViewGroup) compositorView).getChildCount(); index++) {
                View nextChild = ((ViewGroup) compositorView).getChildAt(index);
                Log.d("bn",
                        "compositorViewchildren braveactivity inflate nextchild after add:"
                                + nextChild);
            }
        } else {
            Log.d("bn", "inflateNewsSettingsBar null containers ");
        }
    }

    public void removeSetttingsBar() {
        CompositorViewHolder compositorView = findViewById(R.id.compositor_view_holder);
        for (int index = 0; index < ((ViewGroup) compositorView).getChildCount(); index++) {
            View nextChild = ((ViewGroup) compositorView).getChildAt(index);
            Log.d("bn",
                    "compositorViewchildren settings bar remove nextchild before remove:"
                            + nextChild);
        }
        if (compositorView != null) {
            View settingsBar = compositorView.getChildAt(2);
            if (settingsBar != null){                
                if (settingsBar.getId() == R.id.news_settings_bar) {
                    Log.d("bn", "settings bar remove:" + settingsBar.getId());
                    Log.d("bn", "settings bar remove:" + R.id.news_settings_bar);
                    Log.d("bn", "settings bar remove:" + R.layout.brave_news_settings_bar_layout);
                    // if (settingsBar.getId == "news_settings_bar"){
                    compositorView.removeView(settingsBar);
                    // }
                }
            }
        }
    }

    public void setBackground(Bitmap bgWallpaper) {
        CompositorViewHolder compositorView = findViewById(R.id.compositor_view_holder);
        // compositorView.setBackgroundResource(R.drawable.img2);

        ViewGroup root = (ViewGroup) compositorView.getChildAt(1);
        ScrollView scrollView = (ScrollView) root.getChildAt(0);
        scrollView.setId(View.generateViewId());

        Log.d("BN", "compositor child at 0:" + compositorView.getChildAt(0));
        Log.d("BN", "compositor child at 1:" + compositorView.getChildAt(1));
        Log.d("BN", "compositor child at 1 scrollView :" + root.getChildAt(0));
        Log.d("BN", "compositor child at  1 firtst id :" + scrollView.getId());

        // int bwidth=bgWallpaper.getWidth();
        // int bheight=bgWallpaper.getHeight();
        // int swidth=scrollView.getWidth();
        // int sheight=scrollView.getHeight();
        // int new_width=swidth;
        // int new_height = (int) Math.floor((double) bheight *( (double) new_width / (double)
        // bwidth)); Bitmap newbitMap = Bitmap.createScaledBitmap(bgWallpaper,new_width,new_height,
        // true);

        scrollView.setBackground(new BitmapDrawable(bgWallpaper));
    }

    private void checkFingerPrintingOnUpgrade() {
        if (!PackageUtils.isFirstInstall(this)
                && SharedPreferencesManager.getInstance().readInt(
                           BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 0) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            boolean value = sharedPreferences.getBoolean(
                    BravePrivacySettings.PREF_FINGERPRINTING_PROTECTION, true);
            if (value) {
                BravePrefServiceBridge.getInstance().setFingerprintingControlType(
                        BraveShieldsContentSettings.DEFAULT);
            } else {
                BravePrefServiceBridge.getInstance().setFingerprintingControlType(
                        BraveShieldsContentSettings.ALLOW_RESOURCE);
            }
        }
    }

    private void openBraveWallet() {
        Intent braveWalletIntent = new Intent(this, BraveWalletActivity.class);
        braveWalletIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(braveWalletIntent);
    }

    private void checkSetDefaultBrowserModal() {
        boolean shouldShowDefaultBrowserModal =
                (OnboardingPrefManager.getInstance().getNextSetDefaultBrowserModalDate() > 0
                        && System.currentTimeMillis()
                                > OnboardingPrefManager.getInstance()
                                          .getNextSetDefaultBrowserModalDate());
        boolean shouldShowDefaultBrowserModalAfterP3A =
                OnboardingPrefManager.getInstance().shouldShowDefaultBrowserModalAfterP3A();
        if (!BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(this)
                && (shouldShowDefaultBrowserModalAfterP3A || shouldShowDefaultBrowserModal)) {
            Intent setDefaultBrowserIntent = new Intent(this, SetDefaultBrowserActivity.class);
            setDefaultBrowserIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(setDefaultBrowserIntent);
            if (shouldShowDefaultBrowserModal) {
                OnboardingPrefManager.getInstance().setNextSetDefaultBrowserModalDate(0);
            }
            if (shouldShowDefaultBrowserModalAfterP3A) {
                OnboardingPrefManager.getInstance().setShowDefaultBrowserModalAfterP3A(false);
            }
        }
    }

    private void checkForYandexSE() {
        String countryCode = Locale.getDefault().getCountry();
        if (yandexRegions.contains(countryCode)) {
            TemplateUrl yandexTemplateUrl =
                    BraveSearchEngineUtils.getTemplateUrlByShortName(OnboardingPrefManager.YANDEX);
            if (yandexTemplateUrl != null) {
                BraveSearchEngineUtils.setDSEPrefs(yandexTemplateUrl, false);
                BraveSearchEngineUtils.setDSEPrefs(yandexTemplateUrl, true);
            }
        }
    }

    private void checkForNotificationData() {
        Intent notifIntent = getIntent();
        if (notifIntent != null && notifIntent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE) != null) {
            String notificationType = notifIntent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
            switch (notificationType) {
            case RetentionNotificationUtil.HOUR_3:
            case RetentionNotificationUtil.HOUR_24:
            case RetentionNotificationUtil.EVERY_SUNDAY:
                checkForBraveStats();
                break;
            case RetentionNotificationUtil.DAY_6:
            case RetentionNotificationUtil.BRAVE_STATS_ADS_TRACKERS:
            case RetentionNotificationUtil.BRAVE_STATS_DATA:
            case RetentionNotificationUtil.BRAVE_STATS_TIME:
                if (getActivityTab() != null && getActivityTab().getUrl().getSpec() != null
                        && !UrlUtilities.isNTPUrl(getActivityTab().getUrl().getSpec())) {
                    getTabCreator(false).launchUrl(
                            UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                }
                break;
            case RetentionNotificationUtil.DAY_10:
            case RetentionNotificationUtil.DAY_30:
            case RetentionNotificationUtil.DAY_35:
                openRewardsPanel();
                break;
            }
        }
    }

    public void checkForBraveStats() {
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            BraveStatsUtil.showBraveStats();
        } else {
            if (getActivityTab() != null && getActivityTab().getUrl().getSpec() != null
                    && !UrlUtilities.isNTPUrl(getActivityTab().getUrl().getSpec())) {
                OnboardingPrefManager.getInstance().setFromNotification(true);
                if (getTabCreator(false) != null) {
                    getTabCreator(false).launchUrl(
                            UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                }
            } else {
                showOnboardingV2(false);
            }
        }
    }

    public void showOnboardingV2(boolean fromStats) {
        try {
            OnboardingPrefManager.getInstance().setNewOnboardingShown(true);
            FragmentManager fm = getSupportFragmentManager();
            HighlightDialogFragment fragment = (HighlightDialogFragment) fm
                                               .findFragmentByTag(HighlightDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = fm.beginTransaction();

            if (fragment != null) {
                transaction.remove(fragment);
            }

            fragment = new HighlightDialogFragment();
            Bundle fragmentBundle = new Bundle();
            fragmentBundle.putBoolean(OnboardingPrefManager.FROM_STATS, fromStats);
            fragment.setArguments(fragmentBundle);
            transaction.add(fragment, HighlightDialogFragment.TAG_FRAGMENT);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("HighlightDialogFragment", e.getMessage());
        }
    }

    public void hideRewardsOnboardingIcon() {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.hideRewardsOnboardingIcon();
        }
    }

    private void createNotificationChannel() {
        Context context = ContextUtils.getApplicationContext();
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = "Brave Browser";
            String description = "Notification channel for Brave Browser";
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
            channel.setDescription(description);
            // Register the channel with the system; you can't change the importance
            // or other notification behaviors after this
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    private void setupBraveSetDefaultBrowserNotification() {
        // Post task to IO thread because isBraveSetAsDefaultBrowser may cause
        // sqlite file IO operation underneath
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
            Context context = ContextUtils.getApplicationContext();
            if (BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(this)) {
                // Don't ask again
                return;
            }
            Intent intent = new Intent(context, BraveSetDefaultBrowserNotificationService.class);
            context.sendBroadcast(intent);
        });
    }

    private boolean isNoRestoreState() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLOSE_TABS_ON_EXIT, false);
    }

    private boolean isClearBrowsingDataOnExit() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLEAR_ON_EXIT, false);
    }

    public void handleBraveSetDefaultBrowserDialog() {
        /* (Albert Wang): Default app settings didn't get added until API 24
         * https://developer.android.com/reference/android/provider/Settings#ACTION_MANAGE_DEFAULT_APPS_SETTINGS
         */
        Intent browserIntent =
            new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        boolean supportsDefault = Build.VERSION.SDK_INT >= Build.VERSION_CODES.N;
        ResolveInfo resolveInfo = getPackageManager().resolveActivity(
                                      browserIntent, supportsDefault ? PackageManager.MATCH_DEFAULT_ONLY : 0);
        Context context = ContextUtils.getApplicationContext();
        if (BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(this)) {
            Toast toast = Toast.makeText(
                              context, R.string.brave_already_set_as_default_browser, Toast.LENGTH_LONG);
            toast.show();
            return;
        }
        if (supportsDefault) {
            if (resolveInfo.activityInfo.packageName.equals(ANDROID_SETUPWIZARD_PACKAGE_NAME)
                    || resolveInfo.activityInfo.packageName.equals(ANDROID_PACKAGE_NAME)) {
                LayoutInflater inflater = getLayoutInflater();
                View layout = inflater.inflate(R.layout.brave_set_default_browser_dialog,
                                               (ViewGroup) findViewById(R.id.brave_set_default_browser_toast_container));

                Toast toast = new Toast(context, layout);
                toast.setDuration(Toast.LENGTH_LONG);
                toast.setGravity(Gravity.TOP, 0, 40);
                toast.show();
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(BRAVE_BLOG_URL));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            } else {
                Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            }
        } else {
            if (resolveInfo.activityInfo.packageName.equals(ANDROID_SETUPWIZARD_PACKAGE_NAME)
                    || resolveInfo.activityInfo.packageName.equals(ANDROID_PACKAGE_NAME)) {
                // (Albert Wang): From what I've experimented on 6.0,
                // default browser popup is in the middle of the screen for
                // these versions. So we shouldn't show the toast.
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(BRAVE_BLOG_URL));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            } else {
                Toast toast = Toast.makeText(
                                  context, R.string.brave_default_browser_go_to_settings, Toast.LENGTH_LONG);
                toast.show();
                return;
            }
        }
    }

    public void OnRewardsPanelDismiss() {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.onRewardsPanelDismiss();
        }
    }

    public void dismissRewardsPanel() {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.dismissRewardsPanel();
        }
    }

    public void dismissShieldsTooltip() {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.dismissShieldsTooltip();
        }
    }

    public void openRewardsPanel() {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.openRewardsPanel();
        }
    }

    public void openBraveTalkOptInPopup(BraveTalkOptInPopupListener popupListener) {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.openBraveTalkOptInPopup(popupListener);
        }
    }

    public void onBraveTalkOptInPopupDismiss() {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.onBraveTalkOptInPopupDismiss();
        }
    }

    public void closeBraveTalkOptInPopup() {
        BraveToolbarLayoutImpl layout = (BraveToolbarLayoutImpl) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.closeBraveTalkOptInPopup();
        }
    }

    public Tab selectExistingTab(String url) {
        Tab tab = getActivityTab();
        if (tab != null && tab.getUrl().getSpec().equals(url)) {
            return tab;
        }

        TabModel tabModel = getCurrentTabModel();
        int tabIndex = TabModelUtils.getTabIndexByUrl(tabModel, url);

        // Find if tab exists
        if (tabIndex != TabModel.INVALID_TAB_INDEX) {
            tab = tabModel.getTabAt(tabIndex);
            // Set active tab
            tabModel.setIndex(tabIndex, TabSelectionType.FROM_USER);
            return tab;
        } else {
            return null;
        }
    }

    public Tab openNewOrSelectExistingTab(String url) {
        TabModel tabModel = getCurrentTabModel();
        int tabRewardsIndex = TabModelUtils.getTabIndexByUrl(tabModel, url);

        Log.d("bn", "lifecycle BraveActivity openNewOrSelectExistingTab");
        Tab tab = selectExistingTab(url);
        if (tab != null) {

            Log.d("bn", "lifecycle BraveActivity openNewOrSelectExistingTab openNewOrSelectExistingTab");
            return tab;
        } else { // Open a new tab

            Log.d("bn", "lifecycle BraveActivity openNewOrSelectExistingTab open new");
            return getTabCreator(false).launchUrl(url, TabLaunchType.FROM_CHROME_UI);
        }
    }

    private void showBraveRateDialog() {
        RateDialogFragment mRateDialogFragment = new RateDialogFragment();
        mRateDialogFragment.setCancelable(false);
        mRateDialogFragment.show(getSupportFragmentManager(), "RateDialogFragment");
    }

    private void showCrossPromotionalDialog() {
        CrossPromotionalModalDialogFragment mCrossPromotionalModalDialogFragment = new CrossPromotionalModalDialogFragment();
        mCrossPromotionalModalDialogFragment.setCancelable(false);
        mCrossPromotionalModalDialogFragment.show(getSupportFragmentManager(), "CrossPromotionalModalDialogFragment");
    }

    static public ChromeTabbedActivity getChromeTabbedActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof ChromeTabbedActivity)) continue;

            return (ChromeTabbedActivity)ref;
        }

        return null;
    }

    static public BraveActivity getBraveActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof BraveActivity)) continue;

            return (BraveActivity)ref;
        }

        return null;
    }

    @Override
    public void onActivityResult (int requestCode, int resultCode,
                                  Intent data) {
        if (resultCode == RESULT_OK &&
                (requestCode == VERIFY_WALLET_ACTIVITY_REQUEST_CODE ||
                 requestCode == USER_WALLET_ACTIVITY_REQUEST_CODE ||
                 requestCode == SITE_BANNER_REQUEST_CODE) ) {
            dismissRewardsPanel();
            String open_url = data.getStringExtra(BraveActivity.OPEN_URL);
            if (! TextUtils.isEmpty(open_url)) {
                openNewOrSelectExistingTab(open_url);
            }
        } else if (resultCode == RESULT_OK
                && requestCode == BraveVpnProfileUtils.BRAVE_VPN_PROFILE_REQUEST_CODE
                && BraveVpnUtils.isBraveVpnFeatureEnable()) {
            BraveVpnProfileUtils.getInstance().startVpn(BraveActivity.this);
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == BraveStatsUtil.SHARE_STATS_WRITE_EXTERNAL_STORAGE_PERM
                && grantResults.length != 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            BraveStatsUtil.shareStats(R.layout.brave_stats_share_layout);
        }
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    /**
     * Disable background ads on Android. Issue #8641.
     */
    private void setBgBraveAdsDefaultOff() {
        SharedPreferences sharedPreferences =
            ContextUtils.getAppSharedPreferences();
        boolean exists = sharedPreferences.contains(
                             BraveRewardsPreferences.PREF_ADS_SWITCH_DEFAULT_HAS_BEEN_SET);
        if (!exists) {
            SharedPreferences.Editor sharedPreferencesEditor =
                sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(
                BraveRewardsPreferences.PREF_ADS_SWITCH, false);
            sharedPreferencesEditor.putBoolean(
                BraveRewardsPreferences.PREF_ADS_SWITCH_DEFAULT_HAS_BEEN_SET, true);
            sharedPreferencesEditor.apply();
        }
    }

    @Override
    public void performPreInflationStartup() {
        BraveDbUtil dbUtil = BraveDbUtil.getInstance();
        if (dbUtil.dbOperationRequested()) {
            AlertDialog dialog = new AlertDialog.Builder(this)
            .setMessage(dbUtil.performDbExportOnStart() ? "Exporting database, please wait..."
                        : "Importing database, please wait...")
            .setCancelable(false)
            .create();
            dialog.setCanceledOnTouchOutside(false);
            if (dbUtil.performDbExportOnStart()) {
                dbUtil.setPerformDbExportOnStart(false);
                dbUtil.ExportRewardsDb(dialog);
            } else if (dbUtil.performDbImportOnStart() && !dbUtil.dbImportFile().isEmpty()) {
                dbUtil.setPerformDbImportOnStart(false);
                dbUtil.ImportRewardsDb(dialog, dbUtil.dbImportFile());
            }
            dbUtil.cleanUpDbOperationRequest();
        }
        super.performPreInflationStartup();
    }

    @Override
    protected @LaunchIntentDispatcher.Action int maybeDispatchLaunchIntent(
        Intent intent, Bundle savedInstanceState) {
        boolean notificationUpdate = IntentUtils.safeGetBooleanExtra(
                                         intent, BravePreferenceKeys.BRAVE_UPDATE_EXTRA_PARAM, false);
        if (notificationUpdate) {
            SetUpdatePreferences();
        }

        return super.maybeDispatchLaunchIntent(intent, savedInstanceState);
    }

    private void SetUpdatePreferences() {
        Calendar currentTime = Calendar.getInstance();
        long milliSeconds = currentTime.getTimeInMillis();

        SharedPreferences sharedPref =
            getApplicationContext().getSharedPreferences(
                BravePreferenceKeys.BRAVE_NOTIFICATION_PREF_NAME, 0);
        SharedPreferences.Editor editor = sharedPref.edit();

        editor.putLong(BravePreferenceKeys.BRAVE_MILLISECONDS_NAME, milliSeconds);
        editor.apply();
    }

    public void hideOverview(LayoutManagerChrome layoutManager) {
        Layout activeLayout = layoutManager.getActiveLayout();
        if (activeLayout instanceof StackLayout) {
            ((StackLayout) activeLayout).commitOutstandingModelState(LayoutManagerImpl.time());
        }
    }

    public ObservableSupplier<BrowserControlsManager> getBrowserControlsManagerSupplier() {
        return mBrowserControlsManagerSupplier;
    }

    public int getToolbarBottom() {
        View toolbarShadow = findViewById(R.id.toolbar_shadow);
        assert toolbarShadow != null;
        if (toolbarShadow != null) {
            return toolbarShadow.getBottom();
        }
        return 0;
    }

    @NativeMethods
    interface Natives {
        void restartStatsUpdater();
    }
}
