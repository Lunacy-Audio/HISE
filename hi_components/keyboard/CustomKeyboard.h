/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef __JUCE_HEADER_BD6D51421EADA068__
#define __JUCE_HEADER_BD6D51421EADA068__

namespace hise { using namespace juce;

class MidiKeyboardFocusTraverser : public KeyboardFocusTraverser
{
	Component *getDefaultComponent(Component *parentComponent) override;
};

class ComponentWithMidiKeyboardTraverser : public Component
{
public:

	KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); };
};


class CustomKeyboardLookAndFeelBase : public LookAndFeel_V3
{
public:

	virtual ~CustomKeyboardLookAndFeelBase() {}
	
	virtual void drawKeyboardBackground(Graphics &g, int width, int height) = 0;

	virtual void drawWhiteNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour) = 0;
	virtual void drawBlackNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour) = 0;

};

class CustomKeyboardLookAndFeel: public CustomKeyboardLookAndFeelBase
{
public:

	CustomKeyboardLookAndFeel();

	virtual void drawKeyboardBackground(Graphics &g, int width, int height);

	virtual void drawWhiteNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour);
	virtual void drawBlackNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour);

	// Binary resources:
	static const char* black_key_off_png;
	static const int black_key_off_pngSize;
	static const char* black_key_on_png;
	static const int black_key_on_pngSize;
	static const char* white_key_off_png;
	static const int white_key_off_pngSize;
	static const char* white_key_on_png;
	static const int white_key_on_pngSize;

	bool useVectorGraphics = false;
	bool useFlatStyle = false;

	Colour bgColour;
	Colour topLineColour;
	Colour overlayColour;
	Colour activityColour;

private:

	//==============================================================================
	Image cachedImage_black_key_off_png;
	Image cachedImage_black_key_on_png;
	Image cachedImage_white_key_off_png;
	Image cachedImage_white_key_on_png;

};

class KeyboardBase
{
public:

	virtual bool isMPEKeyboard() const = 0;

	virtual void setLowestKeyBase(int lowKey) = 0;

	Component* asComponent() { return dynamic_cast<Component*>(this); }
	const Component* asComponent() const { return dynamic_cast<const Component*>(this); }

	virtual float getKeyWidthBase() const = 0;
	virtual bool isShowingOctaveNumbers() const = 0;
	virtual int getRangeStartBase() const = 0;
	virtual int getRangeEndBase() const = 0;
	virtual bool isUsingCustomGraphics() const { return false; }
	virtual double getBlackNoteLengthProportionBase() const = 0;
	virtual bool isToggleModeEnabled() const { return false; }
	virtual int getMidiChannelBase() const = 0;

	
	virtual void setUseCustomGraphics(bool) = 0;
	virtual void setRangeBase(int min, int max) = 0;
	virtual void setKeyWidthBase(float w) = 0;

	virtual void setShowOctaveNumber(bool /*shouldShow*/) {};
	virtual void setBlackNoteLengthProportionBase(float /*ratio*/) {};
	virtual void setEnableToggleMode(bool /*isOn*/) {};
	virtual void setMidiChannelBase(int /*midiChannel*/) = 0;

	virtual void setUseVectorGraphics(bool shouldUseVectorGraphics, bool useFlatStyle) { ignoreUnused(shouldUseVectorGraphics, useFlatStyle); }
	virtual bool isUsingVectorGraphics() const { return true; };
	virtual bool isUsingFlatStyle() const { return false; };

	virtual ~KeyboardBase() {};
};

class CustomKeyboard : public MidiKeyboardComponent,
					   public SafeChangeListener,
					   public ButtonListener,
					   public KeyboardBase
					   
{
public:
    //==============================================================================
    CustomKeyboard (MainController* mc);
    virtual ~CustomKeyboard();

	void buttonClicked(Button *b) override
	{
		if (b->getName() == "OctaveUp")
		{
            lowKey += 12;
        }
            
		else
		{
            lowKey -= 12;
        }
		
        setAvailableRange(lowKey, lowKey + 19);
	}

	void paint(Graphics &g) override;;


	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		
		repaint();
	}

	
	void mouseDown(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	bool isMPEKeyboard() const override { return false; }

	bool isUsingCustomGraphics() const noexcept override { return useCustomGraphics; };
	void setUseCustomGraphics(bool shouldUseCustomGraphics) override;

	void setShowOctaveNumber(bool shouldDisplayOctaveNumber) override { displayOctaveNumber = shouldDisplayOctaveNumber; }
	bool isShowingOctaveNumbers() const override { return displayOctaveNumber; }

	void setLowestKeyBase(int lowKey_) override { setLowestVisibleKey(lowKey_); }

	float getKeyWidthBase() const override { return getKeyWidth(); };
	void setKeyWidthBase(float w) override { setKeyWidth(w); }

	int getRangeStartBase() const override { return lowKey; };
	int getRangeEndBase() const override { return hiKey; };

	int getMidiChannelBase() const override { return getMidiChannel(); }
	void setMidiChannelBase(int newChannel) override 
	{ 
		setMidiChannel(newChannel); 
		BigInteger mask = 0;
		mask.setBit(newChannel-1, true);
		setMidiChannelsToDisplay(mask.toInteger());
	}

	int getLowKey() const { return lowKey; }
	int getHiKey() const { return hiKey; }
	
	void setRangeBase(int min, int max) override { setRange(min, max); }

	

	void setBlackNoteLengthProportionBase(float ratio) override { setBlackNoteLengthProportion(ratio); }
	double getBlackNoteLengthProportionBase() const override { return getBlackNoteLengthProportion(); }

	bool isToggleModeEnabled() const override { return toggleMode; };
	void setEnableToggleMode(bool shouldBeEnabled) override { toggleMode = shouldBeEnabled; }

	void setUseVectorGraphics(bool shouldUseVectorGraphics, bool useFlatStyle=false) override;

	bool isUsingVectorGraphics() const override { return laf.useVectorGraphics; }

	bool isUsingFlatStyle() const override { return laf.useFlatStyle; }

	void setRange(int lowKey_, int hiKey_)
	{
		lowKey = jlimit<int>(0, 100, lowKey_);
		hiKey = jlimit<int>(10, 128, hiKey_);

		setAvailableRange(lowKey, hiKey);
	}

	Rectangle<float> getRectangleForKeyPublic(int midiNoteNumber)
	{
		return getRectangleForKey(midiNoteNumber);
	}
	
protected:

	void drawWhiteNote (int midiNoteNumber, Graphics &g, Rectangle<float> area, bool isDown, bool isOver, Colour lineColour, Colour textColour) override;

	void drawBlackNote (int midiNoteNumber, Graphics &g, Rectangle<float> area, bool isDown, bool isOver, Colour noteFillColour) override;



private:

	Array<PooledImage> upImages;
	Array<PooledImage> downImages;

	MainController* mc;

	CustomKeyboardLookAndFeel laf;

	CustomKeyboardState *state;
 
	bool useCustomGraphics = false;
	bool useVectorGraphics = false;

    bool narrowKeys;
    
    int lowKey = 12;
	int hiKey = 127;

	bool displayOctaveNumber = false;

	bool toggleMode = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomKeyboard)
};

class ComponentWithKeyboard
{
public:
    
    virtual ~ComponentWithKeyboard() {};
    
    virtual Component *getKeyboard() const = 0;
};


} // namespace hise

#endif   
