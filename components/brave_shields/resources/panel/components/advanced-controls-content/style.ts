import styled from 'styled-components'
import globalIconUrl from '../../assets/globe.svg'
import listsIconUrl from '../../assets/lists.svg'

export const SettingsBox = styled.div`
  padding: 17px var(--pad-x) 0 var(--pad-x);

  a {
    font-size: 13px;
    font-weight: 600;
    text-decoration: none;
    display: flex;
    align-items: center;
    margin-bottom: 20px;
    line-height: 0;
  }
`

export const SettingsTitle = styled.div`
  color: ${(p) => p.theme.color.text02};
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  margin-bottom: 14px;
`

export const SettingsDesc = styled(SettingsTitle)`
  color: ${(p) => p.theme.color.text03};
`

export const GlobeIcon = styled.i`
  display: block;
  width: 17px;
  height: 17px;
  background-image: url(${globalIconUrl});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: top;
  margin-right: 5px;
`

export const ListIcon = styled(GlobeIcon)`
  height: 12px;
  background-image: url(${listsIconUrl});
`

export const ControlGroup = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 22px;

  label {
    display: flex;
    align-items: center;
    font-family: Poppins;
    font-size: 12px;

    span {
      margin-right: 5px;
    }
  }
`

export const ControlCount = styled.button`
  --color: ${(p) => p.theme.color.interactive05};
  background-color: transparent;
  border: 0;
  padding: 0;
  display: inline-flex;
  align-items: center;
  font-family: Poppins;
  font-size: 13px;
  font-weight: 600;
  color: var(--color);
  margin-right: 5px;

  svg {
    width: 18px;
    height: auto;
    margin-right: 5px;

    path {
      fill: var(--color);
    }
  }

  &:disabled,
  [disabled] {
    --color: ${(p) => p.theme.color.disabled};
  }

`
