import styled from 'styled-components'
import UnCheckStar from '../../../assets/svg-icons/star-unchecked.svg'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  disabled?: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  padding: 0px 12px;
`

export const NavOutline = styled.div`
  display: flex;
  height: 36px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-evenly;
  border: 1px solid rgba(255,255,255,0.5);
  border-radius: 12px;
  margin-bottom: 15px;
  max-width: 300px;
`

export const NavDivider = styled.div`
  display: flex;
  width: 1px;
  height: 100%;
  background-color: rgba(255,255,255,0.5);
`

export const NavButton = styled(WalletButton) <StyleProps>`
  flex: 1;
  display: flex;
  height: 100%;
  align-items: center;
  justify-content: center;
  cursor: ${(p) => p.disabled ? 'default' : 'pointer'};
  outline: none;
  border: none;
  background: none;
  pointer-events: ${(p) => p.disabled ? 'none' : 'auto'};
`

export const NavButtonText = styled.span<StyleProps>`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  opacity: ${(p) => p.disabled ? '0.6' : '1'};
`

export const AppsIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${UnCheckStar});
  mask-image: url(${UnCheckStar});
`
