
#ifndef _CVECTORSCOPE_H
	#define _CVECTORSCOPE_H

	#include <cpl/Common.h>
	#include <cpl/CAudioBuffer.h>
	#include <cpl/GraphicComponents.h>
	#include <cpl/Utility.h>
	#include <cpl/gui/Controls.h>
	#include <memory>

	namespace Signalizer
	{


		class CVectorScope 
		: 
			public cpl::SubView, 
			public juce::OpenGLRenderer,
			protected cpl::CBaseControl::PassiveListener,
			protected cpl::CBaseControl::ValueFormatter,
			public cpl::Utility::CNoncopyable
		{

		public:

			CVectorScope(cpl::AudioBuffer & data);
			virtual ~CVectorScope();

			// Component overrides
			void paint(Graphics & g) override;
			void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
			void mouseDoubleClick(const MouseEvent& event) override;
			void mouseDrag(const MouseEvent& event) override;
			void mouseUp(const MouseEvent& event) override;
			void mouseDown(const MouseEvent& event) override;
			void resized() override;
			// OpenGLRender overrides
			void newOpenGLContextCreated() override;
			void renderOpenGL() override;
			void openGLContextClosing() override;

			// View overrides
			juce::Component * getWindow() override;
			void suspend() override;
			void resume() override;
			void freeze() override;
			void unfreeze() override;
			void repaintMainContent() override;
			void attachToOpenGL(juce::OpenGLContext & ctx) override;
			void detachFromOpenGL(juce::OpenGLContext & ctx) override;
			std::unique_ptr<juce::Component> createEditor() override;
			// CSerializer overrides
			void load(cpl::CSerializer::Builder & builder, long long int version) override;
			void save(cpl::CSerializer::Archiver & archive, long long int version) override;

			// cbasecontrol overrides
			void valueChanged(const cpl::CBaseControl *) override;
			bool stringToValue(const cpl::CBaseControl * ctrl, const std::string & buffer, cpl::iCtrlPrec_t & value) override;
			bool valueToString(const cpl::CBaseControl * ctrl, std::string & buffer, cpl::iCtrlPrec_t value) override;
			void onObjectDestruction(const cpl::CBaseControl::ObjectProxy & destroyedObject) override;

		private:
			
			double getGain();
			double mapScaleToFraction(double dbs);
			void initPanelAndControls();
		
			// guis and whatnot
			cpl::CBoxFilter<double, 60> avgFps;

			cpl::CButton kantiAlias, kfadeOld, kdrawLines, kdrawGraph;
			cpl::CKnobSlider kwindow, krotation, kzrotation, kgain;
			cpl::CColourControl kdrawingColour, kgraphColour, kbackgroundColour, kskeletonColour;
			cpl::CTransformWidget ktransform;

			juce::MouseCursor displayCursor;
			// vars
			long long lastFrameTick, renderCycles;
			float cosrol;
			float sinrol;
			bool firstResize, isFrozen;
			int windowSize;

			// data
			cpl::AudioBuffer & audioStream;
			cpl::AudioBuffer audioStreamCopy;

			// unused.
			std::unique_ptr<char> textbuf;
			unsigned long long processorSpeed; // clocks / sec
			bool debug;
			juce::Point<float> lastMousePos;
		};
	
	};

#endif