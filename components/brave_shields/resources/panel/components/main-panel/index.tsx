import * as React from 'react'

import * as S from './style'
import Toggle from '../../../../../web-components/toggle'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import AdvancedControlsContent from '../advanced-controls-content'

function MainPanel () {
  const [isAdvCtrlsExpanded, setIsisAdvCtrlsExpanded] = React.useState(true)

  return (
    <S.Box>
      <S.PanelHeader>
        <S.SiteTitle>brave.com</S.SiteTitle>
      </S.PanelHeader>
      <S.ToggleBox>
        <Toggle
          brand='shields'
          size='lg'
          accessibleLabel='Enable Brave Shields'
        />
        <S.StatusText>Brave Shields <span>UP</span></S.StatusText>
        <S.BlockCountBox>
          <S.BlockCount>21</S.BlockCount>
          <S.BlockNote>
            Trackers, ads, and more blocked <a href="#">Learn more</a>
          </S.BlockNote>
        </S.BlockCountBox>
        <S.Footnote>
          If this site seems broken, try Shields down. Note: this may reduce Brave privacy protections.
        </S.Footnote>
      </S.ToggleBox>

      <S.AdvancedControlsButton
        id='advanced-controls'
        aria-expanded={isAdvCtrlsExpanded}
        aria-controls='advanced-controls-content'
        onClick={() => setIsisAdvCtrlsExpanded(x => !x)}
        isOpen={isAdvCtrlsExpanded}
      >
        Advanced Controls
        <CaratStrongDownIcon />
      </S.AdvancedControlsButton>

      {isAdvCtrlsExpanded && <AdvancedControlsContent />}
    </S.Box>
  )
}

export default MainPanel
