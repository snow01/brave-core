/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  AssetPrice,
  EthereumChain,
  ERCToken,
  TransactionInfo,
  TransactionStatus,
  TransactionType,
  WalletAccountType
} from '../../constants/types'
import {
  formatBalance,
  formatFiatBalance,
  formatFiatGasFee,
  formatGasFee,
  formatGasFeeFromFiat,
  hexToNumber
} from '../../utils/format-balances'
import usePricing from './pricing'
import useAddressLabels, { SwapExchangeProxy } from './address-labels'
import { getLocale } from '../../../common/locale'
import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'

interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFee: string
  gasFeeFiat: string
  isEIP1559Transaction: boolean
}

interface ParsedTransaction extends ParsedTransactionFees {
  // Common fields
  hash: string
  createdTime: TimeDelta
  status: TransactionStatus
  sender: string
  senderLabel: string
  recipient: string
  recipientLabel: string
  fiatValue: string
  fiatTotal: string
  nativeCurrencyTotal: string
  value: string
  symbol: string
  decimals: number
  insufficientFundsError: boolean
  sameAddressError?: string
  contractAddressError?: string
  erc721ERCToken?: ERCToken
  erc721TokenId?: string
  isSwap?: boolean

  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
}

export function useTransactionFeesParser (selectedNetwork: EthereumChain, networkSpotPrice: string) {
  return React.useCallback((transactionInfo: TransactionInfo): ParsedTransactionFees => {
    const { txData } = transactionInfo
    const { baseData: { gasLimit, gasPrice }, maxFeePerGas, maxPriorityFeePerGas } = txData

    const isEIP1559Transaction = maxPriorityFeePerGas !== '' && maxFeePerGas !== ''
    const gasFee = isEIP1559Transaction
      ? formatGasFee(maxFeePerGas, gasLimit, selectedNetwork.decimals)
      : formatGasFee(gasPrice, gasLimit, selectedNetwork.decimals)

    return {
      gasLimit,
      gasPrice,
      maxPriorityFeePerGas,
      maxFeePerGas,
      gasFee,
      gasFeeFiat: formatFiatGasFee(gasFee, networkSpotPrice),
      isEIP1559Transaction
    }
  }, [selectedNetwork, networkSpotPrice])
}

export function useTransactionParser (
  selectedNetwork: EthereumChain,
  accounts: WalletAccountType[],
  spotPrices: AssetPrice[],
  visibleTokens: ERCToken[],
  fullTokenList?: ERCToken[]
) {
  const findSpotPrice = usePricing(spotPrices)
  const getAddressLabel = useAddressLabels(accounts)
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, findSpotPrice(selectedNetwork.symbol))

  const findToken = React.useCallback((contractAddress: string) => {
    return visibleTokens.find((token) => token.contractAddress.toLowerCase() === contractAddress.toLowerCase())
  }, [visibleTokens])

  const checkForSameAddressError = (to: string, from: string): string | undefined => {
    // If value is the same as the selectedAccounts Wallet Address
    return to.toLowerCase() === from.toLowerCase()
      ? getLocale('braveWalletSameAddressError')
      : undefined
  }

  const checkForContractAddressError = (to: string): string | undefined => {
    // If value is a Tokens Contract Address
    return fullTokenList?.some(token => token.contractAddress.toLowerCase() === to.toLowerCase())
      ? getLocale('braveWalletContractAddressError')
      : undefined
  }

  return React.useCallback((transactionInfo: TransactionInfo) => {
    const { txArgs, txData, fromAddress } = transactionInfo
    const { baseData } = txData
    const { value, to } = baseData
    const account = accounts.find((account) => account.address.toLowerCase() === fromAddress.toLowerCase())
    const accountsNativeFiatBalance = accounts.find((account) => account.address.toLowerCase() === fromAddress.toLowerCase())?.fiatBalance
    const usersTokenInfo = account?.tokens.find((asset) => asset.asset.contractAddress.toLowerCase() === to.toLowerCase())

    switch (true) {
      // transfer(address recipient, uint256 amount) → bool
      case transactionInfo.txType === TransactionType.ERC20Transfer: {
        const [address, amount] = txArgs
        const token = findToken(to)
        const price = findSpotPrice(token?.symbol ?? '')
        const sendAmountFiat = formatFiatBalance(amount, token?.decimals ?? 18, price)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)
        const accountsTokenFiatBalance = usersTokenInfo?.fiatBalance
        const insufficientNativeFunds = Number(gasFeeFiat) > Number(accountsNativeFiatBalance)
        const insufficientTokenFunds = Number(sendAmountFiat) > Number(accountsTokenFiatBalance)

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: transactionInfo.fromAddress,
          senderLabel: getAddressLabel(transactionInfo.fromAddress),
          recipient: address,
          recipientLabel: getAddressLabel(address),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(sendAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: formatBalance(amount, token?.decimals ?? 18),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          insufficientFundsError: insufficientNativeFunds || insufficientTokenFunds,
          sameAddressError: checkForSameAddressError(address, transactionInfo.fromAddress),
          contractAddressError: checkForContractAddressError(address),
          ...feeDetails
        } as ParsedTransaction
      }

      case transactionInfo.txType === TransactionType.ERC721TransferFrom:
      case transactionInfo.txType === TransactionType.ERC721SafeTransferFrom: {
        const [fromAddress, toAddress, tokenID] = txArgs
        const token = findToken(to)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = gasFeeFiat
        const insufficientNativeFunds = Number(gasFeeFiat) > Number(accountsNativeFiatBalance)

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(transactionInfo.fromAddress),
          recipient: toAddress,
          recipientLabel: getAddressLabel(toAddress),
          fiatValue: '0.00', // Display NFT values in the future
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(totalAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: '1', // Can only send 1 erc721 at a time
          symbol: token?.symbol ?? '',
          decimals: 0,
          insufficientFundsError: insufficientNativeFunds,
          erc721ERCToken: token,
          erc721TokenId: hexToNumber(tokenID ?? ''),
          sameAddressError: checkForSameAddressError(toAddress, fromAddress),
          contractAddressError: checkForContractAddressError(toAddress),
          ...feeDetails
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case transactionInfo.txType === TransactionType.ERC20Approve: {
        const [address, amount] = txArgs
        const token = findToken(to)
        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = Number(gasFeeFiat).toFixed(2)
        const insufficientNativeFunds = Number(gasFeeFiat) > Number(accountsNativeFiatBalance)
        const formatedValue = formatBalance(amount, token?.decimals ?? 18)
        const userTokenBalance = usersTokenInfo?.assetBalance ?? ''
        const allowanceValue = Number(amount) > Number(userTokenBalance) ? getLocale('braveWalletTransactionApproveUnlimited') : formatedValue

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: transactionInfo.fromAddress,
          senderLabel: getAddressLabel(transactionInfo.fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: (0).toFixed(2),
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: (0).toFixed(2),
          value: allowanceValue,
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          approvalTarget: address,
          approvalTargetLabel: getAddressLabel(address),
          insufficientFundsError: insufficientNativeFunds,
          ...feeDetails
        } as ParsedTransaction
      }

      // FIXME: swap needs a real parser to figure out the From and To details.
      case transactionInfo.txData.baseData.to.toLowerCase() === SwapExchangeProxy:
      case transactionInfo.txType === TransactionType.ETHSend:
      default: {
        const networkPrice = findSpotPrice(selectedNetwork.symbol)
        const sendAmountFiat = formatFiatBalance(value, selectedNetwork.decimals, networkPrice)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: transactionInfo.fromAddress,
          senderLabel: getAddressLabel(transactionInfo.fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(sendAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: formatBalance(value, selectedNetwork.decimals),
          symbol: selectedNetwork.symbol,
          decimals: selectedNetwork?.decimals ?? 18,
          insufficientFundsError: Number(totalAmountFiat) > Number(accountsNativeFiatBalance),
          sameAddressError: checkForSameAddressError(to, transactionInfo.fromAddress),
          contractAddressError: checkForContractAddressError(to),
          isSwap: transactionInfo.txData.baseData.to.toLowerCase() === SwapExchangeProxy,
          ...feeDetails
        } as ParsedTransaction
      }
    }
  }, [selectedNetwork, spotPrices, findToken])
}
