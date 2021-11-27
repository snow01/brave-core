/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CRYPTO_DOT_COM_CRYPTO_DOT_COM_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_CRYPTO_DOT_COM_CRYPTO_DOT_COM_SERVICE_FACTORY_H_

#include "brave/base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class CryptoDotComService;
class Profile;

class CryptoDotComServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static CryptoDotComService* GetForProfile(Profile* profile);
  static CryptoDotComServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<CryptoDotComServiceFactory>;

  CryptoDotComServiceFactory();
  ~CryptoDotComServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(CryptoDotComServiceFactory);
};

#endif  // BRAVE_BROWSER_CRYPTO_DOT_COM_CRYPTO_DOT_COM_SERVICE_FACTORY_H_
