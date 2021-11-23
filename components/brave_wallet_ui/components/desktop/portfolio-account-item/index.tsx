import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'
import { create } from 'ethereum-blockies'
import { Tooltip } from '../../shared'
import { getLocale } from '../../../../common/locale'
import { EthereumChain } from '../../../constants/types'
import { TransactionPopup } from '../'
// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AccountName,
  AccountAddress,
  AccountAndAddress,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AccountCircle,
  MoreButton,
  MoreIcon,
  RightSide
} from './style'
import { TransactionPopupItem } from '../transaction-popup'

export interface Props {
  address: string
  fiatBalance: string
  assetBalance: string
  assetTicker: string
  selectedNetwork: EthereumChain
  name: string
}

const PortfolioAccountItem = (props: Props) => {
  const { address, name, assetBalance, fiatBalance, assetTicker, selectedNetwork } = props
  const [showAccountPopup, setShowAccountPopup] = React.useState<boolean>(false)
  const onCopyToClipboard = async () => {
    await copyToClipboard(address)
  }

  const orb = React.useMemo(() => {
    return create({ seed: address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [address])

  const onClickViewOnBlockExplorer = () => {
    const exporerURL = selectedNetwork.blockExplorerUrls[0]
    if (exporerURL && address) {
      const url = `${exporerURL}/address/${address}`
      window.open(url, '_blank')
    } else {
      alert(getLocale('braveWalletTransactionExplorerMissing'))
    }
  }

  const onShowTransactionPopup = () => {
    setShowAccountPopup(true)
  }

  const onHideTransactionPopup = () => {
    if (showAccountPopup) {
      setShowAccountPopup(false)
    }
  }

  return (
    <StyledWrapper onClick={onHideTransactionPopup}>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
          <AccountAndAddress onClick={onCopyToClipboard}>
            <AccountName>{name}</AccountName>
            <AccountAddress>{reduceAddress(address)}</AccountAddress>
          </AccountAndAddress>
        </Tooltip>
      </NameAndIcon>
      <RightSide>
        <BalanceColumn>
          <FiatBalanceText>{formatFiatAmountWithCommasAndDecimals(fiatBalance)}</FiatBalanceText>
          <AssetBalanceText>{formatTokenAmountWithCommasAndDecimals(assetBalance, assetTicker)}</AssetBalanceText>
        </BalanceColumn>
        <MoreButton onClick={onShowTransactionPopup}>
          <MoreIcon />
        </MoreButton>
        {showAccountPopup &&
          <TransactionPopup>
            <TransactionPopupItem
              onClick={onClickViewOnBlockExplorer}
              text={getLocale('braveWalletTransactionExplorer')}
            />
          </TransactionPopup>
        }
      </RightSide>
    </StyledWrapper>
  )
}

export default PortfolioAccountItem
