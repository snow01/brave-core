import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import GrantClaim from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('Grant claim tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><GrantClaim id='claim' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#claim').length
      expect(assertion).toBe(1)
    })
  })
})
