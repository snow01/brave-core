import * as React from 'react'
import * as S from './style'

import MainPanel from '../components/main-panel'

export default {
  title: 'ShieldsV2/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  }
}

export const _Main = () => {
  return (
    <S.PanelFrame>
      <MainPanel />
    </S.PanelFrame>
  )
}
