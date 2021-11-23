import * as React from 'react'
import { UserAccountType, BuySendSwapViewTypes, EthereumChain } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { create } from 'ethereum-blockies'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { Tooltip } from '../../shared'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper,
  AccountAddress,
  AccountAndAddress,
  AccountCircle,
  AccountName,
  NameAndIcon,
  OvalButton,
  OvalButtonText,
  CaratDownIcon,
  SwitchIcon
} from './style'

export interface Props {
  selectedAccount: UserAccountType
  selectedNetwork: EthereumChain
  onChangeSwapView: (view: BuySendSwapViewTypes) => void
}

function SwapHeader (props: Props) {
  const { selectedAccount, selectedNetwork, onChangeSwapView } = props

  const onShowAccounts = () => {
    onChangeSwapView('acounts')
  }

  const onShowNetworks = () => {
    onChangeSwapView('networks')
  }

  const onCopyToClipboard = async () => {
    await copyToClipboard(selectedAccount.address)
  }

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount])

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle onClick={onShowAccounts} orb={orb}>
          <SwitchIcon />
        </AccountCircle>
        <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
          <AccountAndAddress onClick={onCopyToClipboard}>
            <AccountName>{reduceAccountDisplayName(selectedAccount.name, 11)}</AccountName>
            <AccountAddress>{reduceAddress(selectedAccount.address)}</AccountAddress>
          </AccountAndAddress>
        </Tooltip>
      </NameAndIcon>
      <Tooltip text={selectedNetwork.chainName}>
        <OvalButton onClick={onShowNetworks}>
          <OvalButtonText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</OvalButtonText>
          <CaratDownIcon />
        </OvalButton>
      </Tooltip>
    </StyledWrapper >
  )
}

export default SwapHeader
