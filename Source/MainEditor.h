/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef _MAINEDITOR_H
	#define _MAINEDITOR_H

	#include <cpl/Common.h>
	#include "PluginProcessor.h"
	#include <cpl/GraphicComponents.h>
	#include <cpl/ComponentContainers.h>
	#include <cpl/CBaseControl.h>
	//#include <cpl/NewStuffAndLook.h>
	#include <map>
	#include <stack>
	#include "SignalizerDesign.h"
	#include <array>

	namespace Signalizer
	{

		class MainEditor
		: 
			public		AudioProcessorEditor, 
			private		juce::Timer, 
			private		juce::HighResolutionTimer,
			protected	cpl::CBaseControl::PassiveListener,
			private		cpl::CBaseControl::ValueFormatter,
			public		cpl::TopView, 
			protected	cpl::CTextTabBar<>::CTabBarListener,
			private		juce::ComponentBoundsConstrainer
		{

		public:

			MainEditor(SignalizerAudioProcessor * e);
			~MainEditor();

			// overrides
			juce::Component * getWindow() override { return this; }
			void valueChanged(const cpl::CBaseControl * cbc) override;
			void load(cpl::CSerializer & se, long long int version) override;
			void save(cpl::CSerializer & se, long long int version) override;
			void paint(Graphics& g) override;
			void paintOverChildren(Graphics & g) override;
			void resized() override;
			void timerCallback() override;
			void hiResTimerCallback() override;
			void resizeEnd() override;
			void resizeStart() override;
			void focusGained(FocusChangeType cause) override;
			void focusLost(FocusChangeType cause) override;
			std::unique_ptr<juce::Component> createEditor() override;
			void panelOpened(cpl::CTextTabBar<> * obj) override;
			void panelClosed(cpl::CTextTabBar<> * obj) override;
			void tabSelected(cpl::CTextTabBar<> * obj, int index) override;
			void suspend() override;
			void resume() override;

			// functionality
			void setRefreshRate(int rateInMs);
			void updatePresetList();
			// no parameter = fetch antialiasing from UI combo box
			void setAntialiasing(int multiSamplingLevel = -1);
			// doesn't actually change anything - only updates the selected value in the preset list.
			void setSelectedPreset(juce::File newPreset);
		protected:
			bool stringToValue(const cpl::CBaseControl * ctrl, const std::string & valString, cpl::iCtrlPrec_t & val) override;
			bool valueToString(const cpl::CBaseControl * ctrl, std::string & valString, cpl::iCtrlPrec_t val) override;
			void onObjectDestruction(const cpl::Utility::DestructionServer<cpl::CBaseControl>::ObjectProxy & destroyedObject) override;

		private:

			// the z-ordering system ensures this is basically a FIFO system
			void pushEditor(juce::Component * editor);
			void pushEditor(std::unique_ptr<juce::Component> editor);
			juce::Component * getTopEditor() const;
			void popEditor();
			void clearEditors();

			cpl::View * viewFromIndex(std::size_t index);
			void addTab(const std::string & name);
			void restoreTab();

			int getRenderEngine();

			void initUI();

			cpl::iCtrlPrec_t oldRefreshRate;
			bool unFocused, idleInBack, isEditorVisible;
			cpl::CSerializer viewSettings;
			SignalizerAudioProcessor * engine;
			ResizableCornerComponent rcc;
			cpl::View * currentView;
			juce::Path rightButtonOutlines;
			int refreshRate;
			std::size_t selTab, oldTab;
			std::map<std::string, std::unique_ptr<cpl::SubView>> views;
			std::stack<std::unique_ptr<juce::Component>> editorStack;

			juce::OpenGLContext oglc;

			// controls
			cpl::CKnobSlider krefreshRate;
			cpl::CComboBox krenderEngine, kpresetList, kantialias;
			cpl::CSVGButton ksettings, kfreeze, ksync, kidle;
			cpl::CButton kstableFps, kloadPreset, ksavePreset, ksaveDefaultPreset, kloadDefaultPreset, kvsync;
			// other UI stuff
			cpl::CTextTabBar<> tabs;

			std::array<cpl::CColourControl, cpl::CLookAndFeel_CPL::numColours>  colourControls;

			// default view
			Signalizer::CDefaultView defaultView;
		};
	};

#endif  // MainEditor
