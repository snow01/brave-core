import styled from 'styled-components'
import { ArrowUpIcon } from 'brave-ui/components/icons'
import { AssetIconProps, AssetIconFactory } from '../../../shared/style'

interface StyleProps {
  isDown: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
`

export const BalanceTitle = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: normal;
  color: ${(p) => p.theme.color.text03};
`

export const BalanceText = styled.span`
  font-family: Poppins;
  font-size: 32px;
  font-weight: 600;
  margin-bottom: 20px;
  color: ${(p) => p.theme.color.text01};
  height: 32px;
`

export const PriceText = styled.span`
  font-family: Poppins;
  font-size: 24px;
  font-weight: 600;
  line-height: 36px;
  letter-spacing: 0.02em;
  margin-right: 10px;
  color: ${(p) => p.theme.color.text01};
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin: 20px 0px;
`

export const BalanceRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const InfoColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin: 10px 0px 20px 10px;
`

export const AssetRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  margin-bottom: 9px;
`

export const PriceRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const AssetNameText = styled.span`
  font-family: Poppins;
  font-size: 20px;
  line-height: 30px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text01};
`

export const DetailText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text03};
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const SubDivider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
  margin-bottom: 12px;
`

export const DividerText = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  font-weight: 600;
  margin-bottom: 10px;
  color: ${(p) => p.theme.color.text03};
`

export const PercentBubble = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-conent: center;
  flex-direction: row;
  padding: 4px 8px;
  border-radius: 8px;
  background-color: ${(p) => p.isDown ? '#EE6374' : '#2AC194'};
`

export const PercentText = styled.span`
  font-family: Poppins;
  font-size: 11px;
  line-height: 17px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const ArrowIcon = styled(ArrowUpIcon) <Partial<StyleProps>>`
  width: 12px;
  height: 12px;
  margin-right: 2px;
  transform: ${(p) => p.isDown ? 'rotate(270deg)' : 'rotate(90deg)'};
  color: ${(p) => p.theme.palette.white};
`

export const EmptyTransactionContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100px;
`

export const TransactionPlaceholderText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text03};
  margin-left: 10px;
`

export const AssetBalanceDisplay = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text02};
`

export const DividerRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
`

export const Spacer = styled.div`
  display: flex;
  height: 2px;
  width: 100%;
  margin-top: 10px;
`

export const CoinGeckoText = styled.span`
  font-family: Arial;
  font-size: 10px;
  font-weight: normal;
  color: ${(p) => p.theme.color.text03};
  margin: 15px 0px;
`
