//
//  ofxTLBeatJump.h
//  part of AscoGraph : graphical editor for Antescofo musical scores.
//
//  Created by Thomas Coffy on 06/12/12.
//  Licensed under the Apache License : http://www.apache.org/licenses/LICENSE-2.0
//
#pragma once

#include <iostream>
#include <list>
#include "ofxTLTrack.h"


#ifndef ASCOGRAPH_IOS
class ofxAntescofog;
#else
#include "iOSAscoGraph.h"
class iOSAscoGraph;
#endif


class antescofoJump {
public:
	antescofoJump(float beat_, float destBeat_, string destLabel_, ofColor& color_) 
		: beat(beat_), destBeat(destBeat_), destLabel(destLabel_), color(color_) {}
	float beat;
	float destBeat;
	ofColor color;
	string destLabel;
};

class ofxTLBeatJump : public ofxTLTrack {
public:
	friend class ofxTLAntescofoNote;
#ifndef ASCOGRAPH_IOS
	ofxTLBeatJump(ofxAntescofog* Antescofog);
    ofxAntescofog* mAntescofog;
#else
	ofxTLBeatJump(iOSAscoGraph* Antescofog);
    iOSAscoGraph* mAntescofog;
#endif
	~ofxTLBeatJump() { clear_jumps(); }
	virtual void draw();
	virtual void update();
	virtual void setup();

	virtual void mousePressed(ofMouseEventArgs& args);
	virtual void mouseMoved(ofMouseEventArgs& args);
	virtual void mouseDragged(ofMouseEventArgs& args);
	virtual void mouseReleased(ofMouseEventArgs& args);
	void refreshArrows();
	void clear_jumps();
	void add_jump(float beat_, float destBeat_, string destLabel_);

	vector<antescofoJump*> jumpList;
	bool isSetup;
	int oldBoundsH;
};
