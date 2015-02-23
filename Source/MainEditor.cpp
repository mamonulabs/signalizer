/*
==============================================================================

This file was auto-generated by the Introjucer!

It contains the basic startup code for a Juce application.

==============================================================================
*/

#include "MainEditor.h"
#include "CVectorScope.h"
//#include "COscilloscope.h"
//#include "CSpectrum.h"
#include "SignalizerDesign.h"
#include <cpl/CPresetManager.h>

namespace cpl
{
	const ProgramInfo programInfo
	{
		"Signalizer", 
		"0.1",
		0x001000,
		"Janus Thorborg",
		"sgn",
		false,
		""

	};

};

namespace Signalizer
{
	const static int defaultLength = 700, defaultHeight = 480;
	const static std::vector<std::string> RenderingEnginesList = { "openGL", "Software" };

	const char * ViewIndexToMap[] = 
	{
		"Vectorscope",
		"Oscilloscope",
		"Spectrum",
		"Statistics"

	};

	enum class ViewTypes
	{
		Vectorscope,
		Oscilloscope,
		Spectrum,
		end
	};

	enum class Editors
	{
		GlobalSettings,
		Vectorscope,
		Oscilloscope,
		Spectrum,
		end
	};
	enum class RenderTypes
	{
		openGL,
		Software,
		end
	};

	enum class Utility
	{
		Freeze,
		Sync,
		IdleInBack,
		end

	};
	
	const static std::array<int, 5> AntialisingLevels =
	{
		1,
		2,
		4,
		8,
		16
	};
	
	const static std::vector<std::string> AntialisingStringLevels =
	{
		"1",
		"2",
		"4",
		"8",
		"16"
	};

	MainEditor::MainEditor(SignalizerAudioProcessor * e)
	:
		engine(e),
		AudioProcessorEditor(e),
		TopView(this),
		rcc(this, this),
		krenderEngine("Rendering Engine", RenderingEnginesList),
		krefreshRate("Refresh Rate"),
		refreshRate(0),
		oldRefreshRate(0),
		unFocused(true), 
		idleInBack(false),
		isEditorVisible(false),
		selTab(0),
		oldTab(0),
		currentView(nullptr),
		kstableFps("Stable FPS"),
		ksavePreset("Save current..."),
		kloadPreset("Load preset..."),
		ksaveDefaultPreset("Save as default"),
		kloadDefaultPreset("Load default")

	{
		setMinimumSize(50, 50);
		setBounds(0, 0, defaultLength, defaultHeight);
		initUI();

		updatePresetList();
		kpresetList.bInterpretAndSet("default", true);
		oglc.setContinuousRepainting(false);
	}


	std::unique_ptr<juce::Component> MainEditor::createEditor()
	{
		auto content = new Signalizer::CContentPage();
		if (auto page = content->addPage("Settings", "icons/svg/wrench.svg"))
		{
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				section->addControl(&krefreshRate, 0);
				section->addControl(&krenderEngine, 0);
				section->addControl(&kstableFps, 1);
				section->addControl(&kantialias, 1);

				page->addSection(section, "Control");
			}
		}
		if (auto page = content->addPage("Colours", "icons/svg/painting.svg"))
		{
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				int numKnobsPerLine = colourControls.size() / 2;
				int rem = colourControls.size() % 2;

				for (int y = 0; y < 2; ++y)
				{
					for (int x = 0; x < numKnobsPerLine; ++x)
					{
						section->addControl(&colourControls[y * numKnobsPerLine + x], y);
					}
				}
				if (rem)
					section->addControl(&colourControls[colourControls.size() - 1], 0);
				page->addSection(section, "Colour Scheme");
			}
		}
		if (auto page = content->addPage("Presets", "icons/svg/save.svg"))
		{
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				section->addControl(&ksavePreset, 0);
				section->addControl(&ksaveDefaultPreset, 1);
				section->addControl(&kloadPreset, 2);
				section->addControl(&kloadDefaultPreset, 3);
				page->addSection(section, "Presets");
			}
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				updatePresetList();
				section->addControl(&kpresetList, 0);
				page->addSection(section, "Current presets");
			}
		}

		return std::unique_ptr<juce::Component>(content);
	}

	void MainEditor::focusGained(FocusChangeType cause)
	{
		if (idleInBack)
		{
			if (unFocused)
			{
				krefreshRate.bSetValue(oldRefreshRate);
			}
		}

		unFocused = false;
	}
	void MainEditor::focusLost(FocusChangeType cause)
	{
		oldRefreshRate = krefreshRate.bGetValue();
		if (idleInBack)
		{
			if (!unFocused)
			{
				krefreshRate.bSetValue(0.5f);
			}
			unFocused = true;
		}
	}

	void MainEditor::onObjectDestruction(const cpl::Utility::DestructionServer<cpl::CBaseControl>::ObjectProxy & destroyedObject)
	{
		// no-op.
	}

	void MainEditor::pushEditor(juce::Component * editor)
	{
		pushEditor(std::unique_ptr<juce::Component>(editor));
		
	}
	void MainEditor::pushEditor(std::unique_ptr<juce::Component> newEditor)
	{
		if (newEditor.get())
			editorStack.push(std::move(newEditor));

		// beware of move construction! newEditor is now invalid!
		if (auto editor = getTopEditor())
		{
			addAndMakeVisible(editor);
			resized();
			repaint();
		}
	}
	juce::Component * MainEditor::getTopEditor() const
	{
		return editorStack.empty() ? nullptr : editorStack.top().get();
	}
	void MainEditor::popEditor()
	{
		if (!editorStack.empty())
		{
			editorStack.pop();

			if(tabs.isOpen())
				tabs.closePanel();
		}
	}
	void MainEditor::clearEditors()
	{
		while (!editorStack.empty())
			editorStack.pop();
	}
	cpl::View * MainEditor::viewFromIndex(std::size_t index)
	{
		auto it = views.end();
		if ((ViewTypes)index < ViewTypes::end)
			it = views.find(ViewIndexToMap[index]);

		return (it != views.end()) ? it->second.get() : nullptr;
	}

	void MainEditor::setRefreshRate(int rate)
	{
		refreshRate = cpl::Math::confineTo(rate, 10, 1000);
		if (kstableFps.bGetValue() > 0.5)
		{
			juce::Timer::stopTimer();
			juce::HighResolutionTimer::startTimer(refreshRate);
		}
		else
		{
			juce::HighResolutionTimer::stopTimer();
			juce::Timer::startTimer(refreshRate);
		}
		if (currentView)
			currentView->setApproximateRefreshRate(refreshRate);

	}
	void MainEditor::resume()
	{
		setRefreshRate(refreshRate);
	}

	void MainEditor::suspend()
	{
		juce::HighResolutionTimer::stopTimer();
		juce::Timer::stopTimer();
	}

	void MainEditor::valueChanged(const cpl::CBaseControl * c)
	{
		// bail out early if we aren't showing anything.
		if (!currentView)
			return;

		auto value = c->bGetValue();

		// freezing of displays
		if (c == &kfreeze)
		{
			if (value > 0.5)
			{
				currentView->freeze();
			}
			else
			{
				currentView->unfreeze();
			}
		}
		// syncing of audio stream with views
		else if (c == &ksync)
		{
			if (value > 0.5)
			{
				currentView->setSyncing(true);
			}
			else
			{
				currentView->setSyncing(false);
			}
		}
		// lower display rate if we are unfocused
		else if (c == &kidle)
		{
			idleInBack = value > 0.5 ? true : false;
		}

		else if (c == &ksettings)
		{
			if (value > 0.5)
			{
				// spawn the global setting editor
				tabs.openPanel();
				pushEditor(this->createEditor());
			}
			else
			{
				// -- remove it
				popEditor();
			}
		}
		// change of refresh rate
		else if (c == &krefreshRate)
		{
			refreshRate = cpl::Math::round<int>(cpl::Math::UnityScale::exp(value, 10.0, 1000.0));
			setRefreshRate(refreshRate);
		}
		// change of the rendering engine
		else if (c == &krenderEngine)
		{
			auto index = cpl::distribute<RenderTypes>(value);

			switch (index)
			{
			case RenderTypes::Software:

				if (currentView && currentView->isOpenGL())
				{
					currentView->detachFromOpenGL(oglc);
					if (oglc.isAttached())
						oglc.detach();
				}


				break;
			case RenderTypes::openGL:
				if (oglc.isAttached())
				{
					if (currentView && !currentView->isOpenGL())
					{
						// ?? freaky
						if (cpl::View * unknownView = dynamic_cast<cpl::View *>(oglc.getTargetComponent()))
							unknownView->detachFromOpenGL(oglc);
						oglc.detach();
					}
				}
				if (currentView)
					currentView->attachToOpenGL(oglc);
				break;
			}
		}
		else if (c == &ksavePreset)
		{
			cpl::CSerializer::Archiver archive;
			save(archive, 1);
			juce::File location;
			if(cpl::CPresetManager::instance().savePresetAs(archive, location))
			{
				updatePresetList();
				setSelectedPreset(location);
			}

		}
		else if (c == &kloadPreset)
		{
			cpl::CSerializer::Archiver builder;
			juce::File location;
			if (cpl::CPresetManager::instance().loadPresetAs(builder, location))
			{
				load(builder, builder.getMasterVersion());
				setSelectedPreset(location);
			}
		}
		else if (c == &kloadDefaultPreset)
		{
			cpl::CSerializer::Archiver builder;
			juce::File location;
			if (cpl::CPresetManager::instance().loadDefaultPreset(builder, location))
			{
				load(builder, builder.getMasterVersion());
				setSelectedPreset(location);
			}
		}
		else if (c == &ksaveDefaultPreset)
		{
			cpl::CSerializer::Archiver archive;
			save(archive, 1);
			juce::File location;
			if (cpl::CPresetManager::instance().saveDefaultPreset(archive, location))
			{
				updatePresetList();
				setSelectedPreset(location);
			}
		}
		else if (c == &kpresetList)
		{
			auto index = kpresetList.getZeroBasedSelIndex();
			auto & presets = cpl::CPresetManager::instance().getPresets();

			if (presets.size())
			{
				index = cpl::Math::confineTo(index, 0, (int)presets.size() - 1);
				juce::File location;
				cpl::CSerializer::Builder builder;
				if (cpl::CPresetManager::instance().loadPreset(presets[index].getFullPathName().toStdString(), builder, location))
				{
					load(builder, builder.getMasterVersion());
					setSelectedPreset(location);
				}
			}


		}
		else if (c == &kantialias)
		{
			setAntialiasing();
		}
		// check if it was one of the colours
		for (unsigned i = 0; i < colourControls.size(); ++i)
		{
			if (c == &colourControls[i])
			{
				// change colour and broadcast event.
				cpl::CLookAndFeel_CPL::defaultLook().getSchemeColour(i).colour = juce::Colour(colourControls[i].getColour());
				cpl::CLookAndFeel_CPL::defaultLook().updateColours();
				repaint();
			}

		}

	}

	void MainEditor::setSelectedPreset(juce::File location)
	{
		std::string newValue = location.getFileNameWithoutExtension().toStdString();
		kpresetList.bInterpretAndSet(newValue, true);
	}

	void MainEditor::panelOpened(cpl::CTextTabBar<> * obj)
	{
		isEditorVisible = true;
		if (cpl::View * view = viewFromIndex(selTab))
		{
			pushEditor(view->createEditor());
		}
		resized();
		repaint();
	}
	void MainEditor::panelClosed(cpl::CTextTabBar<> * obj)
	{
		clearEditors();
		ksettings.setToggleState(false, NotificationType::dontSendNotification);
		resized();
		repaint();
	}
	
	void MainEditor::setAntialiasing(int multisamplingLevel)
	{

		int sanitizedLevel = 1;
		
		if(multisamplingLevel == -1)
		{
			auto val = cpl::Math::confineTo(kantialias.getZeroBasedSelIndex(), 0, AntialisingLevels.size() - 1);
			sanitizedLevel = AntialisingLevels[val];
		}
		else
		{
			for(int i = 0; i < AntialisingLevels.size(); ++i)
			{
				if(AntialisingLevels[i] == multisamplingLevel)
				{
					sanitizedLevel = AntialisingLevels[i];
					break;
				}
			}
		}
		
		if(sanitizedLevel > 0)
		{
			OpenGLPixelFormat fmt;
			fmt.multisamplingLevel = sanitizedLevel;
			// true if a view exists and it is attached
			bool reattach = false;
			if(currentView)
			{
				if(currentView->isOpenGL())
				{
					currentView->detachFromOpenGL(oglc);
					reattach = true;
				}
			}
			
			oglc.setPixelFormat(fmt);
			
			if(reattach)
			{
				currentView->attachToOpenGL(oglc);
			}
			
		}
		
	}

	int MainEditor::getRenderEngine()
	{
		return (int)cpl::distribute<RenderTypes>(krenderEngine.bGetValue());
	}

	void MainEditor::tabSelected(cpl::CTextTabBar<> * obj, int index)
	{
		if (ksettings.getToggleState())
			ksettings.setToggleState(false, NotificationType::sendNotification);

		// see if the new view exists.

		auto const & mappedView = Signalizer::ViewIndexToMap[index];
		auto it = views.find(mappedView);

		cpl::SubView * view = nullptr;

		if (it == views.end())
		{

			// insert the new view into the map
			switch ((ViewTypes)index)
			{
			case ViewTypes::Vectorscope:
				view = new Signalizer::CVectorScope(engine->audioBuffer);
				break;
			case ViewTypes::Oscilloscope:
			//	view = new COscilloscope(engine->audioBuffer);
				break;
			case ViewTypes::Spectrum:
			//	view = new CSpectrum(engine->audioBuffer);
				break;
			default:
				break;
			}
			if (!view)
				return;
			views.emplace(mappedView, std::unique_ptr<cpl::SubView>(view));
			auto & key = viewSettings.getKey("Serialized Views").getKey(mappedView);
			if (!key.isEmpty())
				view->load(key, key.getMasterVersion());
		}
		else
		{
			view = it->second.get();
		}

		// deattach old view
		if (currentView)
		{
			if (oglc.isAttached())
			{
				currentView->detachFromOpenGL(oglc);
				oglc.detach();
			}
			removeChildComponent(currentView->getWindow());
			currentView->suspend();
		}

		view->resume();
		currentView = view;
		addAndMakeVisible(view);

		if ((RenderTypes)getRenderEngine() == RenderTypes::openGL)
		{
			// init all openGL stuff.
			
			setAntialiasing();
			
			view->attachToOpenGL(oglc);
		}
		resized();
		oldTab = selTab;

		selTab = index;
	}

	void MainEditor::restoreTab()
	{
		tabSelected(&tabs, oldTab);
	}

	void MainEditor::addTab(const std::string & name)
	{
		tabs.addTab(name);
	}

	void MainEditor::save(cpl::CSerializer & data, long long int version)
	{

		data << krefreshRate;
		data << krenderEngine;
		data << ksync;
		data << kfreeze;
		data << kidle;
		data << getBounds();
		data << isEditorVisible;
		data << selTab;
		data << kantialias;

		for (auto & colour : colourControls)
		{
			data.getKey("Colours").getKey(colour.bGetTitle()) << colour;
		}

		// save any view data
		for (auto & viewPair : views)			// watch out, or it'll save the std::unique_ptr!
			data.getKey("Serialized Views").getKey(viewPair.first) << viewPair.second.get();
			//viewPair.second->save(data.getKey("Serialized Views").getKey(viewPair.first), version);

	}

	void MainEditor::load(cpl::CSerializer & data, long long version)
	{
		viewSettings = data;
		//cpl::iCtrlPrec_t dataVal(0);
		juce::Rectangle<int> bounds;

		data >> krefreshRate;
		data >> krenderEngine;
		data >> ksync;
		data >> kfreeze;
		data >> kidle;
		data >> bounds;
		data >> isEditorVisible;
		data >> selTab;
		data >> kantialias;

		for (auto & colour : colourControls)
		{
			auto & content = viewSettings.getKey("Colours").getKey(colour.bGetTitle());
			if (!content.isEmpty())
				content >> colour;
		}

		// sanitize bounds...
		setBounds(bounds.constrainedWithin(
			juce::Desktop::getInstance().getDisplays().getDisplayContaining(bounds.getPosition()).userArea
		));
		// will take care of opening the correct view
		tabs.setSelectedTab(selTab);

	}

	bool MainEditor::stringToValue(const cpl::CBaseControl * ctrl, const std::string & valString, cpl::iCtrlPrec_t & val)
	{
		if (ctrl == &krefreshRate)
		{
			char * endPtr = nullptr;
			auto newVal = strtod(valString.c_str(), &endPtr);
			if (endPtr > valString.c_str())
			{
				newVal = cpl::Math::confineTo
				(
					cpl::Math::UnityScale::Inv::exp
					(
						cpl::Math::confineTo(newVal, 10.0, 1000.0), 
						10.0, 
						1000.0
					),
					0.0,
					1.0
				);
				val = newVal;
				return true;
			}
		}
		return false;
	}

	bool MainEditor::valueToString(const cpl::CBaseControl * ctrl, std::string & valString, cpl::iCtrlPrec_t val)
	{
		if (ctrl == &krefreshRate)
		{
			auto refreshRate = cpl::Math::round<int>(cpl::Math::UnityScale::exp(val, 10.0, 1000.0));
			valString = std::to_string(refreshRate) + " ms";
			return true;
		}
		return false;
	}


	MainEditor::~MainEditor()
	{
		notifyDestruction();
		if (oglc.isAttached())
			oglc.detach();

	}
	void MainEditor::resizeEnd()
	{
		resized();
		resume();
	}

	void MainEditor::resizeStart()
	{
		suspend();
	}




	void MainEditor::resized()
	{
		auto const elementSize = 26;
		auto const elementBorder = 1; // border around all elements, from which the background shines through
		auto const buttonSize = elementSize - elementBorder * 2;
		rcc.setBounds(getWidth() - 15, getHeight() - 15, 15, 15);
		// dont resize while user is dragging
		if (rcc.isMouseButtonDown())
			return;
		// resize panel to width

		auto const width = getWidth();
		auto leftBorder = width - elementSize + elementBorder;
		ksettings.setBounds(1, 1, buttonSize, buttonSize);

		kfreeze.setBounds(leftBorder, 1, buttonSize, buttonSize);
		leftBorder -= elementSize - elementBorder;
		ksync.setBounds(leftBorder, 1, buttonSize, buttonSize);
		leftBorder -= elementSize - elementBorder;
		kidle.setBounds(leftBorder, 1, buttonSize, buttonSize);
		tabs.setBounds(ksettings.getBounds().getRight() + elementBorder, 0, 
			getWidth() - (ksettings.getWidth() + getWidth() - leftBorder + elementBorder * 3),
			elementSize - elementBorder);


		/*rightButtonOutlines.clear();
		rightButtonOutlines.addLineSegment(juce::Line<float>(ksettings.getRight(), 1.f, ksettings.getRight(), (float)elementSize - 1), 0.1f);
		// add a line underneath to seperate..
		if (ksettings.bGetValue() > 0.1f)
			rightButtonOutlines.addLineSegment(juce::Line<float>(1, ksettings.getBottom() - 0.2f, ksettings.getRight(), ksettings.getBottom() - 0.2f), 0.1f);
		rightButtonOutlines.addLineSegment(juce::Line<float>(tabs.getRight(), 1.f, tabs.getRight(), (float)elementSize - 1), 0.1f);
		rightButtonOutlines.addLineSegment(juce::Line<float>(kidle.getRight(), 1.f, kidle.getRight(), (float)elementSize - 1), 0.1f);
		rightButtonOutlines.addLineSegment(juce::Line<float>(ksync.getRight(), 1.f, ksync.getRight(), (float)elementSize - 1), 0.1f);
		*/
		//rightButtonOutlines.addRectangle(ksettings.getBounds());
		//rightButtonOutlines.addRectangle(tabs.getBounds());
		//rightButtonOutlines.addRectangle(juce::Rectangle<int>(kidle.getX(), 0, elementSize * 3, elementSize));


		auto editor = getTopEditor();
		if (editor)
		{
			auto maxHeight = elementSize * 5;

			// content pages knows their own (dynamic) size.
			if (auto signalizerEditor = dynamic_cast<Signalizer::CContentPage *>(editor))
			{
				maxHeight = std::max(0, std::min(maxHeight, signalizerEditor->getSuggestedSize().second));
			}
			editor->setBounds(elementBorder, tabs.getBottom(), getWidth() - elementBorder * 2, maxHeight);

		}
		if (currentView)
		{
			auto y = tabs.getHeight() + (editor ? editor->getHeight() : 0) + elementBorder;
			currentView->getWindow()->setBounds(0, y, getWidth(), getHeight() - y);
		}

		//rightButtonOutlines.addRectangle(juce::Rectangle<float>(0.5f, 0.5f, getWidth() - 1.5f, editor ? editor->getBottom() : elementSize - 1.5f));
	}

	void MainEditor::updatePresetList()
	{
		auto & presetList = cpl::CPresetManager::instance().getPresets();
		std::vector<std::string> shortList;
		for (auto & preset : presetList)
		{
			if (preset.existsAsFile())
			{
				shortList.push_back(preset.getFileNameWithoutExtension().toStdString());
			}
		}

		kpresetList.setValues(shortList);
	}
	void MainEditor::timerCallback()
	{
		
		if (currentView)
		{
			//const MessageManagerLock mml;

			if (idleInBack)
			{
				if (!hasKeyboardFocus(true))
					focusLost(FocusChangeType::focusChangedDirectly);
				else if (unFocused)
					focusGained(FocusChangeType::focusChangedDirectly);
			}

			currentView->repaintMainContent();
		}
	}
	void MainEditor::hiResTimerCallback()
	{
		if (currentView)
		{
			const MessageManagerLock mml;

			if (idleInBack)
			{
				if (!hasKeyboardFocus(true))
					focusLost(FocusChangeType::focusChangedDirectly);
				else if (unFocused)
					focusGained(FocusChangeType::focusChangedDirectly);
			}

			currentView->repaintMainContent();
		}

	}



	//==============================================================================
	void MainEditor::paint(Graphics& g)
	{
		//if (::IsDebuggerPresent())
		//	DebugBreak();
		g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
		g.fillAll();
	}

	void MainEditor::paintOverChildren(Graphics & g)
	{
		juce::PathStrokeType pst(1, PathStrokeType::JointStyle::beveled);
		g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
		//g.strokePath(rightButtonOutlines, pst);

	}

	void MainEditor::initUI()
	{
		auto & lnf = cpl::CLookAndFeel_CPL::defaultLook();
		// add listeners
		krefreshRate.bAddFormatter(this);
		kfreeze.bAddPassiveChangeListener(this);
		kidle.bAddPassiveChangeListener(this);
		krenderEngine.bAddPassiveChangeListener(this);
		krefreshRate.bAddPassiveChangeListener(this);
		ksettings.bAddPassiveChangeListener(this);
		tabs.addListener(this);
		kloadPreset.bAddPassiveChangeListener(this);
		ksavePreset.bAddPassiveChangeListener(this);
		ksaveDefaultPreset.bAddPassiveChangeListener(this);
		kloadDefaultPreset.bAddPassiveChangeListener(this);
		kantialias.bAddPassiveChangeListener(this);
		kpresetList.bAddPassiveChangeListener(this);
		// design
		kfreeze.setImage("icons/svg/snow1.svg");
		kidle.setImage("icons/svg/idle.svg");
		ksettings.setImage("icons/svg/gears.svg");
		ksync.setImage("icons/svg/sync2.svg");

		kstableFps.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height);
		kloadPreset.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height / 2);
		ksavePreset.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height / 2);
		ksaveDefaultPreset.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height / 2);
		kloadDefaultPreset.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height / 2);
		kstableFps.setToggleable(true);

		kpresetList.bSetTitle("Preset from list:");
		kantialias.bSetTitle("Antialiasing");
		// setup
		krenderEngine.setValues(RenderingEnginesList);
		kantialias.setValues(AntialisingStringLevels);

		// initiate colours
		for (unsigned i = 0; i < colourControls.size(); ++i)
		{
			auto & schemeColour = lnf.getSchemeColour(i);
			colourControls[i].setColour(schemeColour.colour.getARGB());
			colourControls[i].bSetTitle(schemeColour.name);
			colourControls[i].bSetDescription(schemeColour.description);
			colourControls[i].bAddPassiveChangeListener(this);

		}

		// add stuff
		addAndMakeVisible(ksettings);
		addAndMakeVisible(kfreeze);
		addAndMakeVisible(ksync);
		addAndMakeVisible(tabs);
		addAndMakeVisible(kidle);
		tabs.setOrientation(tabs.Horizontal);
		tabs.addTab("VectorScope").addTab("Oscilloscope").addTab("Spectrum").addTab("Statistics");

		// additions
		addAndMakeVisible(rcc);
		rcc.setAlwaysOnTop(true);
		currentView = &defaultView;
		addAndMakeVisible(defaultView);
		krefreshRate.bSetValue(0.12);


		// descriptions
		kstableFps.bSetDescription("Stabilize frame rate using a high precision timer.");
		kantialias.bSetDescription("Set the level of hardware antialising applied.");
		kloadPreset.bSetDescription("Load a preset from a file.");
		ksavePreset.bSetDescription("Save the current state as a preset to a file.");
		ksaveDefaultPreset.bSetDescription("Save the current state as the default preset (the one loaded from a fresh instance)");
		kloadDefaultPreset.bSetDescription("Load the default preset.");
		kpresetList.bSetDescription("Load a preset from the preset directory...");
		krefreshRate.bSetDescription("How often the view is redrawn.");
		ksync.bSetDescription("Synchronizes audio streams with view drawing; may incur buffer underruns for low settings.");
		kidle.bSetDescription("If set, lowers the frame rate of the view if this plugin is not in the front.");
		ksettings.bSetDescription("Open the global settings for the plugin (presets, themes and graphics).");
		kfreeze.bSetDescription("Stops the view from updating, allowing you to examine the current point in time.");

		resized();
	}
};