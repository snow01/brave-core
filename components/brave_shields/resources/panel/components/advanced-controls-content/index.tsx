import * as React from 'react'

import * as S from './style'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import Toggle from '../../../../../web-components/toggle'

function GlobalSettings() {
  return (
    <S.SettingsBox>
      <S.SettingsTitle>Global Shield settings</S.SettingsTitle>
      <a href='brave://settings/shields' target='_blank'>
        <S.GlobeIcon />
        Change global Shield defaults
      </a>
      <a href='brave://adblock' target='_blank'>
        <S.ListIcon />
        Customize Adblock lists
      </a>
    </S.SettingsBox>
  )
}

function AdvancedControlsContent () {
  return (
    <section
    id='advanced-controls-content'
    aria-labelledby='advanced-controls'
  >
    <S.SettingsBox>
      <S.SettingsTitle>brave.com</S.SettingsTitle>
      <S.SettingsDesc>These settings affect web compatibility on this site.</S.SettingsDesc>
      <S.ControlGroup>
        <S.ControlCount
          aria-label='Trackers and ads'
          aria-expanded='false'
        >
          <CaratStrongDownIcon />
          <span>10</span>
        </S.ControlCount>
        <select aria-label="Choose a type for trackers and ads">
          <option>Trackers & ads blocked (standard)</option>
          <option>Trackers & ads blocked (aggressive)</option>
          <option>Allow all trackers and ads</option>
        </select>
      </S.ControlGroup>
      <S.ControlGroup>
        <S.ControlCount
          aria-label='Connection upgraded to HTTPS'
          aria-expanded='false'
          disabled
        >
          <CaratStrongDownIcon />
          <span>0</span>
        </S.ControlCount>
        <label>
          <span>Connection upgraded to HTTPS</span>
          <Toggle
            size='sm'
            accessibleLabel='Enable HTTPS'
          />
        </label>
      </S.ControlGroup>
    </S.SettingsBox>
    <GlobalSettings />
  </section>
  )
}

export default AdvancedControlsContent
