/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2013 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "LfpDisplayCanvas.h"

#include <math.h>

LfpDisplayCanvas::LfpDisplayCanvas(LfpDisplayNode* processor_) :
	timebase(1.0f), displayGain(1.0f), timeOffset(0.0f), processor(processor_),
	screenBufferIndex(0), displayBufferIndex(0)
{

	nChans = processor->getNumInputs();
	sampleRate = processor->getSampleRate();
	std::cout << "Setting num inputs on LfpDisplayCanvas to " << nChans << std::endl;

	displayBuffer = processor->getDisplayBufferAddress();
	displayBufferSize = displayBuffer->getNumSamples();
	std::cout << "Setting displayBufferSize on LfpDisplayCanvas to " << displayBufferSize << std::endl;

	screenBuffer = new AudioSampleBuffer(MAX_N_CHAN, MAX_N_SAMP);

	viewport = new Viewport();
	lfpDisplay = new LfpDisplay(this, viewport);
	timescale = new LfpTimescale(this);

	viewport->setViewedComponent(lfpDisplay, false);
	viewport->setScrollBarsShown(true, false);

	scrollBarThickness = viewport->getScrollBarThickness();

	addAndMakeVisible(viewport);
	addAndMakeVisible(timescale);

	lfpDisplay->setNumChannels(nChans);

}

LfpDisplayCanvas::~LfpDisplayCanvas()
{

	deleteAndZero(screenBuffer);
}

void LfpDisplayCanvas::resized()
{



	timescale->setBounds(0,0,getWidth()-scrollBarThickness,30);
	viewport->setBounds(0,30,getWidth(),getHeight()-90);

	lfpDisplay->setBounds(0,0,getWidth()-scrollBarThickness, lfpDisplay->getTotalHeight());

}

void LfpDisplayCanvas::beginAnimation()
{
	std::cout << "Beginning animation." << std::endl;

	displayBufferSize = displayBuffer->getNumSamples();

	screenBufferIndex = 0;
	
	startCallbacks();
}

void LfpDisplayCanvas::endAnimation()
{
	std::cout << "Ending animation." << std::endl;
	
	stopCallbacks();
}

void LfpDisplayCanvas::update()
{
	nChans = processor->getNumInputs();
	sampleRate = processor->getSampleRate();

	std::cout << "Setting num inputs on LfpDisplayCanvas to " << nChans << std::endl;

	refreshScreenBuffer();

	lfpDisplay->setNumChannels(nChans);
	lfpDisplay->setBounds(0,0,getWidth()-scrollBarThickness*2, lfpDisplay->getTotalHeight());


	repaint();

}


void LfpDisplayCanvas::setParameter(int param, float val)
{
	if (param == 0) {
		timebase = val;
		refreshScreenBuffer();
	} else {
		displayGain = val; //* 0.0001f;
	}

	repaint();
}

void LfpDisplayCanvas::refreshState()
{
	// called when the component's tab becomes visible again
	displayBufferIndex = processor->getDisplayBufferIndex();
	screenBufferIndex = 0;

}

void LfpDisplayCanvas::refreshScreenBuffer()
{

	screenBufferIndex = 0;

	screenBuffer->clear();

	// int w = lfpDisplay->getWidth(); 
	// //std::cout << "Refreshing buffer size to " << w << "pixels." << std::endl;

	// for (int i = 0; i < w; i++)
	// {
	// 	float x = float(i);

	// 	for (int n = 0; n < nChans; n++)
	// 	{
	// 		waves[n][i*2] = x;
	// 		waves[n][i*2+1] = 0.5f; // line in center of display
	// 	}
	// }

}

void LfpDisplayCanvas::updateScreenBuffer()
{
	// copy new samples from the displayBuffer into the screenBuffer (waves)

	lastScreenBufferIndex = screenBufferIndex;

	int maxSamples = lfpDisplay->getWidth();

	int index = processor->getDisplayBufferIndex();

	int nSamples = index - displayBufferIndex;

	if (nSamples < 0) // buffer has reset to 0
	{
		nSamples = (displayBufferSize - displayBufferIndex) + index;
	}

	float ratio = sampleRate * timebase / float(getWidth());

	// this number is crucial:
	int valuesNeeded = (int) float(nSamples) / ratio;

	float subSampleOffset = 0.0;
	int nextPos = (displayBufferIndex + 1) % displayBufferSize;

	if (valuesNeeded > 0 && valuesNeeded < 1000) {

	    for (int i = 0; i < valuesNeeded; i++)
	    {
	    	float gain = 1.0;
	    	float alpha = (float) subSampleOffset;
	    	float invAlpha = 1.0f - alpha;

	    	screenBuffer->clear(screenBufferIndex, 1);

	    	for (int channel = 0; channel < nChans; channel++) {

				gain = 1.0f / (processor->channels[channel]->bitVolts * float(0x7fff));

				screenBuffer->addFrom(channel, // destChannel
									  screenBufferIndex, // destStartSample
									  displayBuffer->getSampleData(channel, displayBufferIndex), // source
									  1, // numSamples
									  invAlpha*gain*displayGain); // gain

				screenBuffer->addFrom(channel, // destChannel
									  screenBufferIndex, // destStartSample
									  displayBuffer->getSampleData(channel, nextPos), // source
									  1, // numSamples
									  alpha*gain*displayGain); // gain

	        	//waves[channel][screenBufferIndex*2+1] = 
	        	//	*(displayBuffer->getSampleData(channel, displayBufferIndex))*invAlpha*gain*displayGain;

	        	//waves[channel][screenBufferIndex*2+1] += 
	        	//	*(displayBuffer->getSampleData(channel, nextPos))*alpha*gain*displayGain;

	        	//waves[channel][screenBufferIndex*2+1] += 0.5f; // to center in viewport

	       	}

	       	//// now do the event channel
	       ////	waves[nChans][screenBufferIndex*2+1] = 
	       //		*(displayBuffer->getSampleData(nChans, displayBufferIndex));


	       	subSampleOffset += ratio;

	       	while (subSampleOffset >= 1.0)
	       	{
	       		if (++displayBufferIndex >= displayBufferSize)
	       			displayBufferIndex = 0;
	       		
	       		nextPos = (displayBufferIndex + 1) % displayBufferSize;
	       		subSampleOffset -= 1.0;
	       	}

	       	screenBufferIndex++;
	       	screenBufferIndex %= maxSamples;

	    }

	} else {
		//std::cout << "Skip." << std::endl;
	}
}

float LfpDisplayCanvas::getXCoord(int chan, int samp)
{
	return samp;
}

float LfpDisplayCanvas::getYCoord(int chan, int samp)
{
	return *screenBuffer->getSampleData(chan, samp);
}

void LfpDisplayCanvas::paint(Graphics& g)
{

	//std::cout << "Painting" << std::endl;
	 g.setColour(Colours::grey);

	 g.fillRect(0, 0, getWidth(), getHeight());
	
	 // g.setColour(Colours::yellow);

	 // g.drawLine(screenBufferIndex, 0, screenBufferIndex, getHeight());
	
}

void LfpDisplayCanvas::refresh()
{
	updateScreenBuffer();

}

// -------------------------------------------------------------

LfpTimescale::LfpTimescale(LfpDisplayCanvas* c) : canvas(c)
{

}

LfpTimescale::~LfpTimescale()
{

}

void LfpTimescale::paint(Graphics& g)
{

	g.fillAll(Colours::black);

}


// ---------------------------------------------------------------

LfpDisplay::LfpDisplay(LfpDisplayCanvas* c, Viewport* v) : 
		canvas(c), viewport(v)
{
	channelHeight = 100;
	totalHeight = 0;

	addMouseListener(this, true);
}

LfpDisplay::~LfpDisplay()
{
	deleteAllChildren();
}

void LfpDisplay::setNumChannels(int numChannels)
{
	numChans = numChannels;

	deleteAllChildren();

	channels.clear();

	for (int i = 0; i < numChans; i++)
	{

		//std::cout << "Adding new channel display." << std::endl;

		LfpChannelDisplay* lfpChan = new LfpChannelDisplay(canvas, i);

		addAndMakeVisible(lfpChan);

		channels.add(lfpChan);

		totalHeight += channelHeight;

	}

}

void LfpDisplay::resized()
{

	int totalHeight = 0;

	int overlap = 50;

	for (int i = 0; i < numChans; i++)
	{

		getChildComponent(i)->setBounds(0,totalHeight-overlap/2,getWidth(),channelHeight+overlap);

		totalHeight += channelHeight;

	}

}

void LfpDisplay::paint(Graphics& g)
{


	int topBorder = viewport->getViewPositionY();
	int bottomBorder = viewport->getViewHeight() + topBorder;

	// ensure that only visible channels are redrawn
	// for (int i = 0; i < numChans; i++)
	// {

	// 	int componentTop = getChildComponent(i)->getY();
	// 	int componentBottom = getChildComponent(i)->getHeight() + componentTop;

	// 	if ( (topBorder <= componentBottom && bottomBorder >= componentTop) )
	// 	{
	// 		getChildComponent(i)->repaint(canvas->lastScreenBufferIndex,
	// 			 						  0,
	// 			 						  canvas->screenBufferIndex,
	// 			 						  getChildComponent(i)->getHeight());

	// 		//std::cout << i << std::endl;
	// 	}

	// }

}

void LfpDisplay::mouseDown(const MouseEvent& event)
{
	int x = event.getMouseDownX();
	int y = event.getMouseDownY();

	std::cout << "Mouse down at " << x << ", " << y << std::endl;


	for (int n = 0; n < numChans; n++)
	{
		channels[n]->deselect();
	}

	LfpChannelDisplay* lcd = (LfpChannelDisplay*) event.eventComponent;

	lcd->select();

	repaint();

}

// ------------------------------------------------------------------

LfpChannelDisplay::LfpChannelDisplay(LfpDisplayCanvas* c, int channelNumber) : 
	canvas(c), isSelected(false), chan(channelNumber)
{

}

LfpChannelDisplay::~LfpChannelDisplay()
{

}

void LfpChannelDisplay::paint(Graphics& g)
{

	if (isSelected)
		g.setColour(Colours::lightgrey);
	else
		g.setColour(Colours::black);

	g.drawLine(0, getHeight()/2, getWidth(), getHeight()/2);

	int stepSize = 1;

	for (int i = 0; i < getWidth()-1; i += stepSize)
	{

		g.drawLine(i,
				 (canvas->getYCoord(chan, i)+0.5f)*getHeight(),
				 i+stepSize,
				 (canvas->getYCoord(chan, i+stepSize)+0.5f)*getHeight());
	}

}

void LfpChannelDisplay::select()
{
	isSelected = true;
}

void LfpChannelDisplay::deselect()
{
	isSelected = false;
}
