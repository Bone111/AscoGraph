//
//  ofxAntescofoAction.cpp
//  part of AscoGraph : graphical editor for Antescofo musical scores.
//
//  Created by Thomas Coffy on 06/12/12.
//  Licensed under the Apache License : http://www.apache.org/licenses/LICENSE-2.0
//
#include <sstream>
#include <algorithm>
#include <string>
#ifdef ASCOGRAPH_IOS
# include "iOSAscoGraph.h"
#else
# include <ofxAntescofog.h>
# include "ofxModifierKeys.h"
#endif
#include "ofxTLAntescofoNote.h"
#include "ofxTLAntescofoAction.h"
#include "Score.h"
#include "Values.h"
#include "Action.h"
#include "Track.h"
#include <location.hh>
#include <position.hh>
#include "ofxHotKeys.h"
#ifndef ASCOGRAPH_IOS
# ifndef TARGET_LINUX
#  include "ofxCodeEditor.h"
# endif
#endif
#include "BeatCurve.h"

ofxTimeline *_timeline;

bool debug_edit_curve = false;
bool debug_actiongroup = false;
template<class T>
int inline findAndReplace(T& source, const T& find, const T& replace)
{
	int num=0;
	int fLen = find.size();
	int rLen = replace.size();
	for (int pos=0; (pos=source.find(find, pos))!=T::npos; pos+=rLen)
	{
		num++;
		source.replace(pos, fLen, replace);
	}
	return num;
}

static ActionGroup* create_ActionGroup(float beatnum, Action* g, Event* e);

#ifdef ASCOGRAPH_IOS
ofxTLAntescofoAction::ofxTLAntescofoAction(iOSAscoGraph *Antescofog)
#else
ofxTLAntescofoAction::ofxTLAntescofoAction(ofxAntescofog *Antescofog)
#endif
{
	mAntescofog = Antescofog;
	bEditorShow = false;
	movingAction = false;
	movingActionRect = ofRectangle(0, 0, 40, 20);
	shouldDrawModalContent = false;
	mTrackStates.clear();
	bElevatorShowMore = mFilterActions = false;

	actualBounds = bounds;
	mTrackBtnHeight = 13;
	mTrackBtnWidth = 10;
	mTrackBtnSpace = 20;
	mCurveBeingEdited = 0;
	mElevatorClickedY = mElevatorStartY = mMaxHeight = mFirstTrackBtn = 0;
	mPrevTrackBtn.height = mNextTrackBtn.height = mTrackBtnHeight;
	bActionsEditable = bHasToResize = true;
	bViewActionWithDeepLevels = false;

	// store Antescofo tracks names
	if (TrackDefinition::idx2track.size()) {
		for (TrackDefinition::idx2t_t::iterator i = TrackDefinition::idx2track.begin(); i != TrackDefinition::idx2track.end(); i++)
			mTrackStates[i->second->_name.substr(7)] = new TrackState(); // 7 because "track::"
	}

	elevator_disable();
	ofAddListener(ofEvents().windowResized, this, &ofxTLAntescofoAction::windowResized);
}


ofxTLAntescofoAction::~ofxTLAntescofoAction()
{
	clear_actions();
	if (timeline) ofRemoveListener(timeline->timelineEvents.viewWasResized, this, &ofxTLAntescofoAction::viewWasResized);
}

void ofxTLAntescofoAction::setup()
{
	_timeline = getTimeline();

	load();

	//update();
	actualBounds = bounds;
	mCurveArrowImgUp.loadImage(ofFilePath::getCurrentExeDir() + "../Resources/GUI/arrowcurve_up.png");
	mCurveArrowImgDown.loadImage(ofFilePath::getCurrentExeDir() + "../Resources/GUI/arrowcurve_down.png");

	disable();
}

void ofxTLAntescofoAction::viewWasResized(ofEventArgs& args){
	cout << "ofxTLAntescofoAction:view was resized" << endl;
	resize();

	mElevatorBarY = bounds.y;
	mElevatorStartY = 0;
	actualBounds = bounds;
	if (bElevatorEnabled) elevator_enable();
}

void ofxTLAntescofoAction::resize() {
	if (!isEnabled()) return;
	int h = ofGetWindowHeight();
	int y = bounds.y + bounds.height;
	int bh = h - bounds.y;
	//ofxTLTrackHeader* th = timeline->getTrackHeader(this);
	//if (th && bounds != th->getDrawRect()) {
	if (h != bounds.y + bounds.height && timeline) {
		bounds.height = bh;
		ofRectangle r(bounds.x, bounds.y - 18, bounds.width, 18);
		ofxTLTrackHeader* th = timeline->getTrackHeader(this);
		if (th) {
			//cout << "ofxTLAntescofoAction::resize: goin to recalculateFooter"<< endl;
			th->setDrawRect(r);
			th->recalculateFooter();		
			setDrawRect(bounds);
		}
	}
	bHasToResize = false;
}
void ofxTLAntescofoAction::draw()
{
	if (mActionGroups.empty()) {
		//bounds.height = 0;
		disable();
	}
	if (mScore && bounds.height > 1) {
		// draw close cross
		ofSetColor(0, 0, 0, 255);
		ofRect(mRectCross);
		ofLine(mRectCross.x, mRectCross.y, mRectCross.x+mRectCross.width, mRectCross.y+mRectCross.height);
		ofLine(mRectCross.x+mRectCross.width, mRectCross.y, mRectCross.x, mRectCross.y+mRectCross.height);

		// draw antescofo header tracks
		if (TrackDefinition::idx2track.size()) draw_antescofo_tracks_header();

		update_groups();

		if (bElevatorEnabled) {
			ofPushMatrix();
			ofTranslate(0, mElevatorStartY);

			glEnable(GL_SCISSOR_TEST);
			glScissor(bounds.x, bounds.y, bounds.width, bounds.height);
		}

#if 0
		if (mFilterActions) { //tracks lines
			draw_tracks_lines();
		} else 
#endif
		{
			for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
				bool inbounds = zoomBounds.max >= timeline->beatToNormalizedX((*i)->beatnum)
					|| zoomBounds.min <= timeline->beatToNormalizedX((*i)->beatnum + (*i)->duration);
				if (ofxAntescofoNote->mode_guido()) inbounds = true;
				if (inbounds) {
					ofSetColor(0, 0, 0, 125);

					ActionGroup *act = *i;
					act->draw(this);

					if (debug_actiongroup) act->print();

					if (0 && movingAction) {
						ofPushStyle();
						ofFill();
						//ofSetColor(123, 13, 13, 140);
						ofSetColor(200, 200, 200, 255);
						ofRect(movingActionRect);
						ofPopStyle();
					}

				}
			}

			if (foreground_groups.size()) {
				for (int i = 0; i < foreground_groups.size(); i++) {
					foreground_groups[i]->draw(this);
				}
			}
		}
		if (bElevatorEnabled) {
			glDisable(GL_SCISSOR_TEST);
			ofPopMatrix();
			elevator_update();
			draw_elevator();
		}
		if (mCurveBeingEdited) { // if editing a curve
			draw_curve_big();
		}
		// draw modal windows for every curves
		for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
			bool inbounds = zoomBounds.max >= timeline->beatToNormalizedX((*i)->beatnum) || zoomBounds.min <= timeline->beatToNormalizedX((*i)->beatnum + (*i)->duration);
			if (ofxAntescofoNote->mode_guido()) inbounds = true;
			if (inbounds) {
				if (*i && !(*i)->hidden)
					(*i)->drawModalContent(this);
			}
		}

	}
}

void ofxTLAntescofoAction::draw_tracks_lines() {
	//for (map<string, TrackState*>::iterator i = mTrackStates.begin(); i != mTrackStates.end(); i++, curTrackN++) {

}

void ofxTLAntescofoAction::elevator_disable(void) {
	bElevatorEnabled = false;
	mElevatorStartY = 0;
}

void ofxTLAntescofoAction::elevator_enable(void) {
	//cout << "elevator_enable: bounds.y="<<bounds.y << " enabled:" << bElevatorEnabled<< endl;
	bElevatorEnabled = true;
	mElevatorBarY = bounds.y;
	mElevatorStartY = 0;
	actualBounds = bounds;
	elevator_update();
}


void ofxTLAntescofoAction::elevator_update(void) {
	if (mMaxHeight < bounds.height) {
		elevator_disable();
		return;
	}
	mElevatorRect.y = bounds.y;
	mElevatorRect.width = ELEVATOR_WIDTH;
	mElevatorRect.x = bounds.x;// + bounds.width - mElevatorRect.width;
	mElevatorRect.height = bounds.height;

	// x -> boundsH
	// bH -> mH
	// -> x = bH2 / mH
	mElevatorBarHeight = bounds.height * bounds.height / mMaxHeight;
	//cout << "!!!!!!!!!!!!!!!!! mElevatorBarY=" << mElevatorBarY << " mElevatorRect.y=" << mElevatorRect.y << " mMaxHeight=" << mMaxHeight << " bounds.y="<< bounds.y << " bounds.height=" << bounds.height << endl;
	mElevatorStartY = - (mElevatorBarY - mElevatorRect.y) * mMaxHeight / bounds.height;

	actualBounds.y = mElevatorStartY + bounds.y;
	//cout << "Elevator: " << mElevatorRect.x << ", " << mElevatorRect.y << ", " << mElevatorRect.width << " x " << mElevatorRect.height << " bary="<< mElevatorBarY <<" barh=" << mElevatorBarHeight << " mElevatorStartY=" << mElevatorStartY << endl;
}


void ofxTLAntescofoAction::draw_elevator(void) {
	ofPushStyle();
	if (bElevatorShowMore) ofSetColor(0, 0, 0, 90);
	else ofSetColor(0, 0, 0, 70);
	ofFill();
	ofRect(mElevatorRect);

	if (bElevatorShowMore) ofSetColor(0, 0, 0, 215);
	else ofSetColor(0, 0, 0, 165);
	ofRect(mElevatorRect.x, mElevatorBarY, ELEVATOR_WIDTH, mElevatorBarHeight);

	ofPopStyle();
}

	
void ofxTLAntescofoAction::elevator_mousePressed(ofMouseEventArgs& args) {
	bElevatorShowMore = true;
	mElevatorClickedY = args.y; //mElevatorBarY;
	mElevatorBarYClicked = mElevatorBarY;
}
void ofxTLAntescofoAction::elevator_mouseMoved(ofMouseEventArgs& args) {
	bElevatorShowMore = true;
}
void ofxTLAntescofoAction::elevator_mouseDragged(ofMouseEventArgs& args) {
	if (!mElevatorClickedY) return;
	//mElevatorBarY = args.y - mElevatorClickedY + mElevatorRect.y;
	//if (args.y > mElevatorClickedY) // going down
		mElevatorBarY = mElevatorBarYClicked + args.y - mElevatorClickedY;
	//else mElevatorBarY = mElevatorBarClickedY + mElevatorClickedY - args.y;
	//cout << "elevator_mouseDragged: args.y=" << args.y << " mElevatorBarY=" << mElevatorBarY << " mElevatorClickedY=" << mElevatorClickedY << endl;
	mElevatorBarY = ofClamp(mElevatorBarY, mElevatorRect.y, mElevatorRect.y+mElevatorRect.height);
	int maxy = bounds.y+bounds.height;
	if (mElevatorBarY + mElevatorBarHeight > maxy)
		mElevatorBarY = maxy - mElevatorBarHeight;
}
void ofxTLAntescofoAction::elevator_mouseReleased(ofMouseEventArgs& args) {
	mElevatorClickedY = 0;
}

void ofxTLAntescofoAction::draw_antescofo_tracks_header(int x, int y, int sens) {
		ofPushStyle();
		ofFill();
		ofSetColor(0, 0, 0, 205);
		if (sens > 0) { // next
			ofTriangle(mNextTrackBtn.x, mNextTrackBtn.y, 
				   mNextTrackBtn.x, mNextTrackBtn.y + mNextTrackBtn.height, 
				   mNextTrackBtn.x + mNextTrackBtn.width, mNextTrackBtn.y + mNextTrackBtn.height/2); 
		} else { // prev
			ofTriangle(mPrevTrackBtn.x, mPrevTrackBtn.y + mPrevTrackBtn.height/2,
				   mPrevTrackBtn.x + mPrevTrackBtn.width, mPrevTrackBtn.y, 
				   mPrevTrackBtn.x + mPrevTrackBtn.width, mPrevTrackBtn.y + mPrevTrackBtn.height); 
		}
		ofPopStyle();
}

void ofxTLAntescofoAction::setEditable(bool state) {
	bActionsEditable = state;
}


void ofxTLAntescofoAction::draw_antescofo_tracks_header() {
	bool selected = false;
	int x = bounds.x + 100;
	int y = bounds.y;

	int x_prevbtn = x;
	int x_nextbtn = bounds.x + bounds.width - mTrackBtnWidth;

	mFont.drawString("Filter :", x, y - 6);

	x += 50 + mTrackBtnSpace;
	int curTrackN = 0;

	if (mFirstTrackBtn > 0)
		draw_antescofo_tracks_header(x_prevbtn, y, -1); 

	for (map<string, TrackState*>::iterator i = mTrackStates.begin(); i != mTrackStates.end(); i++, curTrackN++) {
		if (curTrackN >= mFirstTrackBtn) {
			if (x + i->first.size()*sizec > bounds.x + bounds.width - 20) {
				draw_antescofo_tracks_header(x_nextbtn, y, 1);
				break;
			}
			ofPushStyle();
			int len = (i->first.size() + 1) * sizec;
			if (i->second->selected) { ofFill(); ofSetColor(0, 0, 0, 100); }
			else { ofNoFill(); ofSetColor(0, 0, 0, 255); }

			i->second->rect.x = x - 1;
			i->second->rect.y = y - 15;
			i->second->rect.width = len + 2;
			i->second->rect.height = mTrackBtnHeight;

			ofRect(i->second->rect);
			ofSetColor(0, 0, 0, 255);
			mFont.drawString(i->first, i->second->rect.x + 3, y - 5);
			ofPopStyle();
			x += len + mTrackBtnSpace;
		}
	}
}

//--------------------------------------------------
void ofxTLAntescofoAction::drawBitmapStringHighlight(string text, int x, int y, const ofColor& background, const ofColor& foreground) {
}

void ofxTLAntescofoAction::add_action_curves(float beatnum, ActionGroup *ar, Curve *c) 
{ 
}

int ofxTLAntescofoAction::get_x(float beat) {
	if (ofxAntescofoNote->mode_pianoroll()) // pianoroll
		return normalizedXtoScreenX( timeline->beatToNormalizedX(beat), zoomBounds);
	else // guido
		return ofxAntescofoNote->beat2guidoX(beat);
}


ofColor ofxTLAntescofoAction::get_random_color() {
	static ofColor color(205, 1, 1, 200);

	//color += ofColor(10, 210, 155, 200);
	color.r = (color.r + 10) % 255;
	color.g = (color.g + 210) % 255;
	color.b = (color.b + 155) % 255;
	color.a = 200; //GROUP_COLOR_ALPHA;
	return color;
}

void ofxTLAntescofoAction::attribute_header_colors(vector<ActionGroup*> groups) {
	for (vector<ActionGroup*>::const_iterator i = groups.begin(); i != groups.end(); i++)
	{
		if (!*i) abort();
		(*i)->headerColor = get_random_color();
		ActionGroup *a = *i;
		attribute_header_colors((*i)->sons);
		/*
		if (a->sons.size()) {
			for (list<ActionGroup*>::const_iterator j = a->sons.begin(); j != a->sons.end(); j++) {
				if (*j)
					(*j)->headerColor = get_random_color();
			}
		}
		*/
	}

	ofAddListener(timeline->timelineEvents.viewWasResized, this, &ofxTLAntescofoAction::viewWasResized);
}

void ofxTLAntescofoAction::add_action(float beatnum, string action, Event *e)
{
	actualBounds = bounds;
	// clean tabs
	string tab("\t");
	string doublespace("  ");
	findAndReplace(action, tab, doublespace);

	ofRectangle rect(bounds.x, bounds.y, 0, 0);
	// extract data
	if (e->gfwd) {
		ActionGroup *ag = create_ActionGroup(beatnum, e->gfwd, e);
		if (ag)
			mActionGroups.push_back(ag);
	}
	if (mActionGroups.size()) {
		enable();
		attribute_header_colors(mActionGroups);
	} else
		disable();
}


void ofxTLAntescofoAction::clear_actions()
{
	while(!mActionGroups.empty()) delete mActionGroups.back(), mActionGroups.pop_back();
}



void ofxTLAntescofoAction::save()
{}

void ofxTLAntescofoAction::load()
{
	//string fontfile = "DroidSansMono.ttf";
	string fontfile = "GUI/NewMedia Fett.ttf";
	mFont.loadFont (fontfile, 9);

	sizec = mFont.stringWidth(string("_"));
	//mFont.loadFont ("menlo.ttf", 10);
}



void ofxTLAntescofoAction::update()
{
	mRectCross.x = bounds.x + bounds.width - 20;
	mRectCross.y = bounds.y - 15;
	mRectCross.width = 14;
	mRectCross.height = 14;

	if (bHasToResize) resize();

	// tracks
	int fromx = bounds.x + 200;
	int w = 0;
	for (map<string, TrackState*>::iterator i = mTrackStates.begin(); i != mTrackStates.end(); i++, w += mTrackBtnSpace) {
		w += i->first.size() * sizec;
		if (w > bounds.width - 200) {
			mPrevTrackBtn.x = bounds.x + 150;
			mNextTrackBtn.x = bounds.x + bounds.width - 40;
			mPrevTrackBtn.y = mNextTrackBtn.y = bounds.y - 15;
			mPrevTrackBtn.width = mNextTrackBtn.width = mTrackBtnWidth;
			break;
		}
	}
	if (bElevatorEnabled) elevator_update();
	//cout << "track bounds: x:" << bounds.x << " y:" << bounds.y << " w:" << bounds.width << " h:" << bounds.height << endl;
}


int ofxTLAntescofoAction::get_max_note_beat()
{
	update_groups();

	int maxbeat = 0;
	if (!mActionGroups.empty()) {
		vector<ActionGroup*>::const_iterator i = mActionGroups.end();
		i--;
		maxbeat = (*i)->beatnum + (*i)->delay;
	}
	return maxbeat;
}

#if 0 // OLD SHIT
int ofxTLAntescofoAction::update_sub_height_curve(ActionMultiCurves* c, int& cury, int& curh)
{
	c->rect.y = cury;
	// update curves rectangle bounds
	int boxy = c->rect.y + c->HEADER_HEIGHT;
	//int boxh = (bounds.height - c->HEADER_HEIGHT - curh - 5) / c->curves.size();
	int boxh = 120 / c->curves.size();
	boxh *= c->resize_factor;
	int boxw = c->getWidth();
	int boxx = c->rect.x;
	if (c->delay) boxx = get_x(c->beatnum /*c->header->delay*/ + c->delay);
	//cout << "boxx: " << boxx << " delay:" << c->delay << endl;
	ofRectangle cbounds(boxx, boxy, boxw, boxh);
	for (int k = 0; k < c->curves.size(); k++) {
		ActionCurve *ac = c->curves[k];
		if (k) cbounds.y += boxh;
		for (int iac = 0; iac < ac->beatcurves.size(); iac++) {
			ac->beatcurves[iac]->setBounds(cbounds);
			ofRange zr(getZoomBounds());
			ac->beatcurves[iac]->setZoomBounds(zr);
			ofRectangle r(getBounds());
			ac->beatcurves[iac]->setTLBounds(r);
			ac->beatcurves[iac]->viewIsDirty = true;
		}
	}
	curh = c->rect.height = c->getHeight();
	
	return 0;
}

int ofxTLAntescofoAction::update_sub_height_message(ActionMessage* m, int& cury, int& curh)
{
	if (mFilterActions && !m->in_selected_track) {
		curh = 0;
		m->rect.height = 0;
		return 0;
	}
	m->rect.x = get_x(m->beatnum + m->delay);
	m->rect.y = cury;
	m->rect.height = m->HEADER_HEIGHT;
	curh = m->HEADER_HEIGHT;

	return 0;
}


int ofxTLAntescofoAction::update_sub_height_switch(ActionSwitch* s, int& cury, int& curh)
{
	if (mFilterActions && !s->in_selected_track) {
		curh = 0;
		s->rect.height = 0;
		return 0;
	}
	s->rect.x = get_x(s->beatnum + s->delay);
	s->rect.y = cury;
	s->rect.height = s->HEADER_HEIGHT;
	curh = s->HEADER_HEIGHT;
	for (int i = 0; i < s->cases.size(); i++)
		s->actions_cases[i]->update();
	return s->getHeight();
}
#endif



// for updating subgroups
// return height of ActionGroup in px
int ofxTLAntescofoAction::update_sub_height(ActionGroup *ag)
{
	int debugsub = 0;
	if (debugsub) cout << endl << "<<<< update_sub_height : " << ag->realtitle << endl;
	int toth = 0, curh = 0; // find max h
	int cury = ag->rect.y;

	//if ((mFilterActions && !ag->in_selected_track)) return 0;

	if (ag->hidden) {
		return ag->HEADER_HEIGHT;
	} else if (ag->sons.size()) {
		cury += ag->HEADER_HEIGHT;
		toth += ag->HEADER_HEIGHT;
	}
	ag->rect.height = toth;

#if 0
	ActionMessage* m = 0;
	ActionMultiCurves* c = 0;
	ActionSwitch* s = 0;
	
	if ((c = dynamic_cast<ActionMultiCurves*>(ag))) {
		//OLDSHIT update_sub_height_curve(c, cury, curh);
		cury += curh;
		toth += curh;
	} else if ((m = dynamic_cast<ActionMessage*>(ag))) {
		//OLDSHIT update_sub_height_message(m, cury, curh);
		m->update_sub_height(cury, curh);
		cury += curh;
		toth += curh;
	} else if ((s = dynamic_cast<ActionSwitch*>(ag))) {
		//OLDSHIT update_sub_height_switch(s, cury, curh);
		s->update_sub_height_switch(cury, curh);
		cury += curh;
		toth += curh;
	}
#else
	ag->update_sub_height(this, cury, curh);
	cury += curh;
	toth += curh;

	ActionMessage* m = 0;
	ActionMultiCurves* c = 0;
	ActionSwitch* s = 0;
#endif

	// go deep
	vector<ActionGroup*>::iterator g;
	for (g = ag->sons.begin(); g != ag->sons.end(); g++) {
#if 0
		if ((m = dynamic_cast<ActionMessage*>(*g)))
			update_sub_height_message(m, cury, curh);
		else if ((c = dynamic_cast<ActionMultiCurves*>(*g)))
			update_sub_height_curve(c, cury, curh);
		else if ((s = dynamic_cast<ActionSwitch*>(ag)))
			update_sub_height_switch(s, cury, curh);
		else { (*g)->rect.height = curh = (*g)->HEADER_HEIGHT; }
#else
		(*g)->update_sub_height(this, cury, curh);
#endif

		if (!(*g)->top_level_group) {
			(*g)->rect.x = ag->rect.x;
			if ((*g)->delay)
				(*g)->rect.x = get_x((*g)->beatnum + (*g)->delay);
			(*g)->rect.y = cury;
			if (debugsub) cout << "update_sub_height: x:" << (*g)->rect.x << " y:"<< (*g)->rect.y << endl;
		}

		curh = update_sub_height(*g);

		if (debugsub) cout << " curh from update_sub_height:" << curh <<" ag->rect.height =" << ag->rect.height  << endl;
		ag->rect.height += curh;
		cury += curh;
		toth += curh;
	}
	if (debugsub) cout << "update_sub_height: returning toth=" << toth << endl;
	if (toth > bounds.height) {
		if (!bElevatorEnabled) elevator_enable();
		if (mMaxHeight < toth) mMaxHeight = toth;
	}
	return toth;
}

// for updating subgroups durations
// return width in beats
float ofxTLAntescofoAction::update_sub_duration(ActionGroup *ag)
{
	int maxw = 0, curw = 0;
	ag->duration = 0;

	ActionMessage *m;
	if ((m = dynamic_cast<ActionMessage*>(ag))) {
		if (m->delay) 
			return m->delay;
	} else {
		vector<ActionGroup*>::const_iterator g;
		for (g = ag->sons.begin(); g != ag->sons.end(); g++) {
			if ((*g)->delay)
				ag->duration += (*g)->delay;
			curw = update_sub_duration(*g);
			//if (curw > maxw)
				maxw += curw;
		}
	}
	return maxw;
}

void ofxTLAntescofoAction::tracks_rec_mark_groups_as_not_displayed(ActionGroup *ag) {
	ag->in_selected_track = false;
	for (vector<ActionGroup*>::iterator g = ag->sons.begin(); g != ag->sons.end(); g++)
		tracks_rec_mark_groups_as_not_displayed(*g);
}


void ofxTLAntescofoAction::tracks_mark_group_as_displayed(ActionGroup *ag) {
	cout << "Marking group " << ag->realtitle << " as displayed." << endl;
	ag->in_selected_track = true;

	// go up and mark each parent true
#if 0
	ActionGroup* g = ag->header->parent;
	while (g) {
		g->in_selected_track = true;
		g = g->parent;
	}
#endif
}


void ofxTLAntescofoAction::tracks_rec_test_if_groups_are_displayed(ActionGroup *ag) {

	for (map<string, TrackState*>::iterator i = mTrackStates.begin(); i != mTrackStates.end(); i++)
		if (i->second->selected) {
			string trackname = "track::" + i->first;
			if (ag->action && ag->action->is_in_track(trackname)) // don't display when not in track
				tracks_mark_group_as_displayed(ag);
		}

	// rec
	vector<ActionGroup*>::const_iterator g;
	for (g = ag->sons.begin(); g != ag->sons.end(); g++)
		tracks_rec_test_if_groups_are_displayed(*g);
}


// for updating subgroups width
// return width in px
int ofxTLAntescofoAction::update_sub_width(ActionGroup *ag)
{
	int maxw = 0, curw = 0;
	int del = 0; float beatdel = 0.;
	ag->duration = 0;

	bool debugsub = false;
	if (debugsub) cout << "update_width: " << ag->title << endl;
	ActionMessage* m;
	ActionMultiCurves* c;
	ActionSwitch* s;

	if (!(mFilterActions && !ag->in_selected_track)) {
		if ((m = dynamic_cast<ActionMessage*>(ag))) {
			int len = sizec * (m->actionstr.size() - 1);
			if (m->delay)
				//len += get_x(m->beatnum + m->delay) - get_x(m->beatnum);
				;
			maxw = len;
			m->rect.width = len;
			if (debugsub) cout << "\tupdate_width: msg maxw:" << maxw << endl;

		} else if ((c = dynamic_cast<ActionMultiCurves*>(ag))) {
			int len = sizec * (ag->title.size());
			if (c->delay)
				//len += abs(get_x(c->beatnum + c->delay)) - abs(get_x(c->beatnum));
				len += get_x(c->beatnum + c->delay) - get_x(c->beatnum);
			maxw = len;
			if (c->isValid) {
				if (c->rect.x < 0)
					maxw = c->getWidth() - abs(get_x(c->beatnum));
				else maxw = c->getWidth();
			}
			//cout << "c->delay=" << c->delay << " isValid=" << c->isValid << " len="<< len <<" getWidth= " << c->getWidth() << endl;
			if (debugsub) cout << "\tupdate_width: curves msg maxw:" << maxw << endl;
		} else if ((s = dynamic_cast<ActionSwitch*>(ag))) {
			if (s->rect.x < 0)
				maxw = s->getWidth() - abs(get_x(s->beatnum));
			else maxw = s->getWidth();
			for (int i = 0; i < s->sons.size(); i++) // update sub cases
				//update_sub_width(s->actions_cases[i]);//->update_sub_height(tlAction, cury, curh);
				update_sub_width(s->sons[i]);
		}
	}
	vector<ActionGroup*>::const_iterator g;
	for (g = ag->sons.begin(); g != ag->sons.end(); g++) {
		curw = update_sub_width(*g);
		if ((*g)->delay) {
			beatdel = (*g)->delay;
			del = get_x((*g)->beatnum + beatdel) - get_x((*g)->beatnum); // get_x((*g)->header->delay);
		}
		if (curw + del > maxw)
			maxw = curw + del;
	}
	if (debugsub) cout << "\tupdate_width: maxw:" << maxw << endl;
	ag->rect.width = maxw;
	//maxw = del + maxw;

	return maxw;
}


// avoid x overlapping
void ofxTLAntescofoAction::update_avoid_overlap_rec(ActionGroup* g, int w, int h)
{
	if (bViewActionWithDeepLevels) return;
	if (g->rect.width + g->rect.x > w) {
		//XXX if (g->rect.y + g->rect.height < h) 
			g->rect.width = w - g->rect.x;
		
		ActionMultiCurves* c = 0;
		if ((c = dynamic_cast<ActionMultiCurves*>(g))) {
			//c->setWidth(g->rect.width);
		}
	}
	for (vector<ActionGroup*>::const_iterator i = g->sons.begin(); i != g->sons.end(); i++) {
		ActionMessage *m;
		
		update_avoid_overlap_rec(*i, w, h);
	}
}

// avoid x overlapping
void ofxTLAntescofoAction::update_avoid_overlap()
{
	//bounds.height = 0;
	for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {

		vector<ActionGroup*>::const_iterator j = i;
		if (++j != mActionGroups.end() && (*i)->is_in_bounds(this) ) {
			if ( ((*i)->rect.x + (*i)->rect.width) > (*j)->rect.x) {
				update_avoid_overlap_rec((*i), (*j)->rect.x - 1, (*j)->rect.height + (*j)->rect.y);
			}
		}
		//cout << "Action hearder height:"  << (*i)->rect.height <<" x:" << bounds.x << endl;
	}
}

void ofxTLAntescofoAction::update_groups()
{
	if (mFilterActions) {
		for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++)
			tracks_rec_mark_groups_as_not_displayed(*i);
		for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++)
			tracks_rec_test_if_groups_are_displayed(*i);
	}

	// update actions' rect
	for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
		if (debug_actiongroup) cout << "--------------------------------------------------------------" << endl;
		//if ((*i)->group && !(*i)->group->is_in_bounds(this)) continue;

		(*i)->rect.x = get_x((*i)->beatnum);
		(*i)->rect.y = actualBounds.y + 1;
		update_sub_duration(*i);
		(*i)->rect.width = update_sub_width(*i);
		
		if (!(*i)->hidden) {
			(*i)->rect.height = update_sub_height(*i);
		} else (*i)->rect.height = (*i)->HEADER_HEIGHT;
	}

	update_avoid_overlap();
}

//--------------------------------------------------------------
void ofxTLAntescofoAction::windowResized(ofResizeEventArgs& resizeEventArgs){
	bHasToResize = true;
}

bool ofxTLAntescofoAction::mousePressed_in_header(ofMouseEventArgs& args, ActionGroup* group, bool recurs) {
	//cout << "mousePressed_in_header: group "<< group->title << " hidden="<<group->hidden << endl; 
	bool res = false;
	if (!group) return res;
	ActionMultiCurves* c = 0;
	//cout << "mousePressed_in_header: group "<< group->realtitle << " hidden="<<group->hidden <<" before" << endl; 
	if (group->rect.inside(args.x, args.y - mElevatorStartY)) {
		if (!group->hidden && (c = dynamic_cast<ActionMultiCurves*>(group))) {
			if (c->mCurveArrowImgRect.inside(args.x, args.y - mElevatorStartY)) {
				cout  << "mousePressed_in_header: group "<< group->realtitle << " INNA DI ARROW " << endl;
				if (mCurveBeingEdited) close_down_curve_editor(c);
				else open_up_curve_editor(c);
				return true;
			}
			cout << "mousePressed_in_header: group "<< group->realtitle << " hidden="<<group->hidden << " return" << endl; 
			return group->is_in_header(args.x, args.y - mElevatorStartY);
		}
		if (group->hidden) {
			//if (!ofGetModifierSelection())
				group->hidden = false;
			cout << "mousePressed: setting action '"<< group->title <<"' hidden:"<< group->hidden << endl;
			if (group) {
				Event *e = group->event;
				//mAntescofog->editorShowLine(e->scloc->begin.line, e->scloc->end.line);
				res = true;
				if ((c = dynamic_cast<ActionMultiCurves*>(group)) && !ofGetModifierSelection())
					c->hidden = false;
			}
			return res;
		} else if (group->is_in_header(args.x, args.y - mElevatorStartY)) {
			if (!ofGetModifierSelection())
				group->hidden = true;
			res = true;

			// hide curve tracks WHY WTF?
			/*ActionMultiCurves* c = 0;
			if ((c = dynamic_cast<ActionMultiCurves*>(group)))
				c->hidden = true;*/
			group->hidden = true;
		} else if (recurs) {
			if (group) { // not hidden and click in group
				if (group->sons.size()) { // rec in subgroups
					ActionGroup *a = group;
					for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); j != a->sons.end(); j++) {
						if (*j && *j != group && mousePressed_in_header(args, *j)) {
							return true;
						}
					}
				}
			}
		}
	}
	return res;
}

ActionGroup* ofxTLAntescofoAction::groupFromScreenPoint_rec(ActionGroup* group, int x, int y) {
	ActionGroup* rg = 0;
	for (vector<ActionGroup*>::const_iterator i = group->sons.begin(); i != group->sons.end(); i++) {
		if ((*i)->rect.inside(x, y) && *i != group) {
			if (!(rg = groupFromScreenPoint_rec(*i, x, y)))
				return *i;
			else return rg;
		}
	}
	if (group && group->rect.inside(x, y)) {
		return group;
	}
	return 0;
}

ActionGroup* ofxTLAntescofoAction::groupFromScreenPoint(int x, int y, vector<ActionGroup*>& groups) {
	y -= mElevatorStartY;
	for (vector<ActionGroup*>::const_iterator i = groups.begin(); i != groups.end(); i++) {
		ActionGroup *gr = 0;
		if ((*i)->rect.inside(x, y) && (gr = groupFromScreenPoint_rec(*i, x, y))) { 
			// check if it was clicked recently
			if (find(clickedGroups.begin(), clickedGroups.end(), gr) != clickedGroups.end()) { // already clicked
				continue;
			} else { // first time clicked
				clickedGroups.push_back(gr);
				return gr;
			}
		}
	}
	return 0;
}

void ofxTLAntescofoAction::regionSelected(ofLongRange timeRange, ofRange valueRange) {
	cout << "regionSelected:" << timeRange.min << " " << timeRange.max << " y: " << valueRange.min << " " << valueRange.max << endl;
}

// mousePress curves
bool ofxTLAntescofoAction::mousePressed_curve_rec(ActionGroup* a, ofMouseEventArgs& args, long millis)
{
	if (a->hidden) return false;
	ActionMultiCurves* ac = (ActionMultiCurves*)a;
	bool res = false;
	for (vector<ActionCurve*>::iterator i = ac->curves.begin(); !res && i != ac->curves.end(); i++) {
		ActionCurve *c = (*i);
		for (int iac = 0; !res && iac < c->beatcurves.size(); iac++) {
#if 0
			if (ofGetModifierSelection()) { // resize curve box
				int xc = c->beatcurves[iac]->bounds.x;
				int xw = c->beatcurves[iac]->bounds.width;
				int yc = c->beatcurves[iac]->bounds.y + c->beatcurves[iac]->bounds.height;
				if ((xc - 2 <= args.x || args.x <= xc + xw + 2) && (yc - 5 <= args.y || args.y <= yc + 5)) {
					cout << "Resizing curve box !" << endl;
				}
			}
#endif
			//cout << "mousePressed_curve_rec: c->beatcurves[iac]->bounds " << c->beatcurves[iac]->bounds.x << ", " << c->beatcurves[iac]->bounds.y << " " << c->beatcurves[iac]->bounds.width << " x " << c->beatcurves[iac]->bounds.height << endl;
			int y = args.y; if (!mCurveBeingEdited || a != mCurveBeingEdited) y -= mElevatorStartY;
			if (c->beatcurves[iac]->bounds.inside(args.x, y) || c->beatcurves[iac]->drawingEasingWindow) {
				//cout << "mousePressed_curve_rec: splitrect= " << c->mSplitBtnRect.x << ", " << c->mSplitBtnRect.y << " " << c->mSplitBtnRect.width << " x " << c->mSplitBtnRect.height << endl;
				if (c->beatcurves.size() || c->mSplitBtnRect.inside(args.x, y)) {
					ofRange zr = getZoomBounds();
					c->beatcurves[iac]->setZoomBounds(zr);
					ofMouseEventArgs args2 = args; if (!mCurveBeingEdited || a != mCurveBeingEdited) args2.y -= mElevatorStartY;
					c->beatcurves[iac]->mousePressed(args2, millis);
					if (c->beatcurves[iac]->drawingEasingWindow)
						shouldDrawModalContent = true;
					cout << "mousePressed_curve_rec: adding" << endl;
					clickedCurves.push_back(ac);
					res = true;
				}
				break;
			}
		}
	}

	for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); !res && j != a->sons.end(); j++) {
		res = mousePressed_curve_rec(*j, args, millis);
	}
	return res;
}

// mousePress recurses through group searching for a multicurve
bool ofxTLAntescofoAction::mousePressed_search_curve_rec(ActionGroup* a, ofMouseEventArgs& args, long millis) {
	bool res = false;
	ActionMultiCurves* ac = 0;
	if ((ac = dynamic_cast<ActionMultiCurves* >(a)))
		res = mousePressed_curve_rec(a, args, millis);
	for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); !res && j != a->sons.end(); j++) {
		if ((ac = dynamic_cast<ActionMultiCurves* >(*j)))
			res = mousePressed_curve_rec(*j, args, millis);
		if (!res)
			res = mousePressed_search_curve_rec(*j, args, millis);
		else 
			break;
	}
	return res;
}

bool ofxTLAntescofoAction::mousePressed(ofMouseEventArgs& args, long millis)
{
	cout << "ofxTLAntescofoAction::mousePressed: x:"<< args.x << " y:" << args.y << endl;
	bool res = false;
#ifndef ASCOGRAPH_IOS
	if (mElevatorRect.inside(args.x, args.y)) {
		elevator_mousePressed(args);
		return true;
	}
	if (mCurveBeingEdited) {
		if (mCurveBeingEdited->rect.inside(args.x, args.y)) // in big curve
			res = mCurveBeingEdited->mousePressed(args, millis);
		else if (mCurveBeingEdited->mCurveArrowImgRect.inside(args.x, args.y)) {
			close_down_curve_editor(mCurveBeingEdited);
			res = true;
		}
		if (res) return res;
	}
	// selection
	ActionGroup* clickedGroup = 0;
	if (foreground_groups.size()) clickedGroup = groupFromScreenPoint(args.x, args.y, foreground_groups);
	if (!clickedGroup) clickedGroup = groupFromScreenPoint(args.x, args.y, mActionGroups);
	if (!clickedGroup && clickedGroups.size()) { // cycle to first clickedgroup
		clickedGroup = clickedGroups.front();
		clickedGroups.clear();
	}
	if (clickedGroup) {
#ifndef TARGET_LINUX
		cout << "clickedGroup: " << clickedGroup->realtitle << endl;
		if (clickedGroup) {
			cout << "show selection: lineNum_begin:" << clickedGroup->lineNum_begin << " lineNum_end:" << clickedGroup->lineNum_end
			     << "\tcolNum_begin:" << clickedGroup->colNum_begin << " colNum_end:" << clickedGroup->colNum_end << endl;
			mAntescofog->editorShowLine(clickedGroup->lineNum_begin, clickedGroup->lineNum_end, clickedGroup->colNum_begin, clickedGroup->colNum_end);
		}
#endif
		res = mousePressed_in_header(args, clickedGroup, false);
		// bring it to foreground level : if CMD pressed : bring background
		if (clickedGroup->is_in_header(args.x, args.y) && ofGetModifierSelection()) {
			cout << "Bringing to front group : " << clickedGroup->realtitle << endl;
			clickedGroup->bringFront();
			foreground_groups.push_back(clickedGroup);
			return true;
		} else { 
			//clickedGroup->bringBack();
			foreground_groups.clear();
		}
		if (!res && !clickedGroup->hidden && bActionsEditable) {
			// handle curve click
			ofMouseEventArgs args2 = args; args2 -= mElevatorStartY;
			mousePressed_search_curve_rec(clickedGroup, args2, millis);
		}
	}
	if (res) { 
		return res;
		//cout << "Not Leaving.... mousepressed: SHIT: clickedCurves:"<< clickedCurves.size() << endl;
	}
	for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
		if (!res && *i != clickedGroup && mousePressed_in_header(args, *i)) {
#ifndef TARGET_LINUX
			if (!clickedGroup) mAntescofog->editorShowLine((*i)->lineNum_begin, (*i)->lineNum_end, (*i)->colNum_begin, (*i)->colNum_end);
#endif
			return true;
		} else if (*i) { // look for subgroups
			//res = mousePressed_search_curve_rec(*i, args, millis);
			ActionGroup* a = *i;
			if (!(*i)->hidden && bActionsEditable) {
				// handle curve click
				res = mousePressed_search_curve_rec(*i, args, millis);
			}
			if (a->sons.size()) {
				for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); j != a->sons.end(); j++) {
					// handle arrow click
					if (*j != clickedGroup && *j && mousePressed_in_header(args, *j)) {
#ifndef TARGET_LINUX
						if (!clickedGroup) mAntescofog->editorShowLine((*i)->lineNum_begin, (*i)->lineNum_end, (*i)->colNum_begin, (*i)->colNum_end);
#endif
						res = true;
					} else if (!(*i)->hidden && bActionsEditable) {
						// handle curve click
						ofMouseEventArgs args2 = args; args2 -= mElevatorStartY;
						res = mousePressed_search_curve_rec(*j, args2, millis);
					}
					//if (res) return res;
				}
			}
		}
	}
#endif

	//cout << "Leaving mousepressed: SHIT: clickedCurves:"<< clickedCurves.size() << endl;
	return res;
}

void ofxTLAntescofoAction::mouseMoved(ofMouseEventArgs& args, long millis)
{
	if (mElevatorRect.inside(args.x, args.y)) {
		elevator_mouseMoved(args);
		return;
	} else bElevatorShowMore = false;
	if (mCurveBeingEdited && mCurveBeingEdited->rect.inside(args.x, args.y))
		mCurveBeingEdited->mouseMoved(args, millis);
	for (vector<ActionMultiCurves*>::iterator j = clickedCurves.begin(); j != clickedCurves.end(); j++) {
		for (vector<ActionCurve*>::iterator i = (*j)->curves.begin(); i != (*j)->curves.end(); i++) {
			ActionCurve *c = (*i);
			for (int iac = 0; iac < c->beatcurves.size(); iac++) {
				if (c->beatcurves[iac]->bounds.inside(args.x, args.y - mElevatorStartY)) {
					//cout << "mouseMoved dyncast ok" << endl;
					if (c->beatcurves.size() == 1) {
						c->beatcurves[iac]->mouseMoved(args, millis);
					}
				}
			}
		}
	}
}


void ofxTLAntescofoAction::mouseDragged(ofMouseEventArgs& args, long millis)
{
	if (mElevatorClickedY) {
		elevator_mouseDragged(args);
		return;
	}
	if (!bActionsEditable) return;
	if (mCurveBeingEdited && mCurveBeingEdited->rect.inside(args.x, args.y))
		mCurveBeingEdited->mouseDragged(args, millis);
	for (vector<ActionMultiCurves*>::iterator j = clickedCurves.begin(); j != clickedCurves.end(); j++) {
		for (vector<ActionCurve*>::iterator i = (*j)->curves.begin(); i != (*j)->curves.end(); i++) {
			ActionCurve *c = (*i);
			for (int iac = 0; iac < c->beatcurves.size(); iac++) {
				if (ofGetModifierSelection()) { // is cmd pressed for resizing curve...
					int xc = c->beatcurves[iac]->bounds.x;
					int xw = c->beatcurves[iac]->bounds.width;
					int yc = c->beatcurves[iac]->bounds.y + c->beatcurves[iac]->bounds.height;

					if ((xc - 2 <= args.x || args.x <= xc + xw + 2)) {
						if (yc - 5 <= args.y) {
							//cout << "Resizing curve box !" << endl;
							c->parentCurve->resize_factor += 0.01;
						} else if (args.y <= yc + 5)
							c->parentCurve->resize_factor -= 0.01;

						if (c->parentCurve->resize_factor == 1.) {
							// TODO c->beatcurves[iac]->bounds.height -= selectionRangeAnchor.y;
						}
					}
				}

				//if (c->beatcurves[iac]->bounds.inside(args.x, args.y) {
					//if (c->beatcurves.size() == 1) {
						int w = c->beatcurves[iac]->bounds.width;
						ofMouseEventArgs args2; args2 = args; args2.y -= mElevatorStartY;
						//cout << "mouseDragged dyncast ok: calling beatcurve mouseDragged: howmany:"<< (*j)->howmany << " varname:"<< c->varname << endl;
						c->beatcurves[iac]->mouseDragged(args2, millis);

						// extend curve box width on mouseDrag
						if (c->beatcurves[iac]->bounds.width != w) {
							cout << "mouseDragged: bounds width:" << c->beatcurves[iac]->bounds.width << " was:" << w << endl;
							c->setWidth(c->beatcurves[iac]->bounds.width);
						}
						//return;

					//}
				//}
			}
		}
	}

	if (movingAction) {
		movingActionRect.x = args.x;
		movingActionRect.y = args.y;
	}

	if(draggingSelectionRange){
		//dragSelection.min = MIN(args.x, selectionRangeAnchor);
		//dragSelection.max = MAX(args.x, selectionRangeAnchor);
		dragSelection.x = MIN(args.x, selectionRangeAnchor.x);
		dragSelection.y = MIN(args.y, selectionRangeAnchor.y);
	}
}

bool ofxTLAntescofoAction::mouseReleased_tracks_header(ofMouseEventArgs& args, long millis)
{
	// antescofo track arrows
	if (mNextTrackBtn.inside(args.x, args.y)) {
		if (mFirstTrackBtn < mTrackStates.size())
			mFirstTrackBtn++;
		return true;
	} else if (mPrevTrackBtn.inside(args.x, args.y)) {
		if (mFirstTrackBtn > 0)
			mFirstTrackBtn--;
		return true;
	}
	for (map<string, TrackState*>::iterator i = mTrackStates.begin(); i != mTrackStates.end(); i++) {
		if (i->second->rect.inside(args.x, args.y)) {
			i->second->selected = !i->second->selected;
			break;
		}
	}
	mFilterActions = false;
	for (map<string, TrackState*>::iterator i = mTrackStates.begin(); i != mTrackStates.end(); i++) {
		if (i->second->selected) {
			mFilterActions = true;
			return true;
		}
	}
	return false;
}



void ofxTLAntescofoAction::mouseReleased(ofMouseEventArgs& args, long millis)
{
	elevator_mouseReleased(args);
	if (mouseReleased_tracks_header(args, millis)) {
		return;
	}
	if (mRectCross.inside(args.x, args.y)) {
		cout << "ofxTLAntescofoAction::mouseReleased: should close track" << endl;
		disable();
		ofxAntescofoNote->deleteActionTrack();
		//timeline->removeTrack(this);
		return;
	}
	//if (!bounds.inside(args.x, args.y)) return;
	if (movingAction) {
		//move_action();
	}
	movingAction = false;

	if (mCurveBeingEdited && mCurveBeingEdited->rect.inside(args.x, args.y))
		mCurveBeingEdited->mouseReleased(args, millis);

	if (!bActionsEditable) return;
	std::unique (clickedCurves.begin(), clickedCurves.end());
	bool done = false;
	for (vector<ActionMultiCurves*>::iterator j = clickedCurves.begin(); !done && j != clickedCurves.end(); j++) {
		for (vector<ActionCurve*>::iterator i = (*j)->curves.begin(); !done && i != (*j)->curves.end(); i++) {
			ActionCurve *c = (*i);
			//cout << "mouseReleased: splitbtn: x:" << c->mSplitBtnRect.x << " y:"<< c->mSplitBtnRect.y << " " << c->mSplitBtnRect.width << "x" << c->mSplitBtnRect.height << endl;
			// split btn
			if (c->mSplitBtnRect.inside(args.x, args.y - mElevatorStartY)) {
				cout << "mouseReleased: split" << endl;
				if (c->beatcurves.size() > 1)
					c->split();
				done = true;
			}
			for (int iac = 0; iac < c->beatcurves.size() && !done; iac++) {
				if (c->beatcurves.size()) { 
					//if (c->beatcurves.size() == 1) {
						if (shouldDrawModalContent && c->beatcurves[iac]->drawingEasingWindow == false)
							continue;
						if (!shouldDrawModalContent && !c->beatcurves[iac]->bounds.inside(args.x, args.y - mElevatorStartY))
							c->beatcurves[iac]->unselectAll();
						cout << "mouseReleased: calling beatcurve varname:" << c->varname << endl;
						ofMouseEventArgs args2 = args; args2.y -= mElevatorStartY;
						c->beatcurves[iac]->mouseReleased(args2, millis);
						//done = true;
					//}
				}
			}
		}
	}
	//_timeline->unselectAll();
	/*
	if(draggingSelectionRange){
		for (list<ActionGroupHeader*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
			//normalizedXtoScreenX( timeline->beatToNormalizedX((*i)->beatnum), zoomBounds)) 
			if((*i)->group && (dragSelection.contains( (*i)->rect.x) || dragSelection.contains( (*i)->rect.x + (*i)->rect.width)))
				(*i)->group->selected = true;
		}
		draggingSelectionRange = false;
	}
	*/
	clickedCurves.clear();
}


// move action from action->selected beatnum 
// to moveActionRect. x y
void ofxTLAntescofoAction::move_action() {
	vector<ActionGroup*>::iterator from;
	/*for (from = mActionGroups.begin(); from != mActionGroups.end(); from++) {
		if (*from && (*from)->selected) {
			(*from)->selected = false;
			break;
		}
	}*/
	if (from == mActionGroups.end() || !(*from))
		return;
	bool doneMoving = false;
	ActionGroup* dest = groupFromScreenPoint(movingActionRect.x, movingActionRect.y, mActionGroups);
	// if dest ActionGroup exists : simply add it
	if (dest) {
		cout << "move_action: dest found" << endl;
		dest->sons.push_back(*from);
		doneMoving = true;
	} else { // else find the event and plug it to it
		cout << "move_action: dest not found" << endl;
		for (vector<ActionGroup*>::iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
			if ((*i)->rect.x <= movingActionRect.x && (*i)->rect.x + (*i)->rect.width >= movingActionRect.x) {
				cout << "move_action: dest finally found" << endl;
				if (!(*i))
					*i = *from;
				else (*i)->sons.push_back(*from);
				//(*from)->group->header = *i;
				dest = *i;
				doneMoving = true;
				break;
			}
		}
	}

	if (doneMoving) {// remove from from
		// assign dest header to from subgroups
		for (vector<ActionGroup*>::iterator j = (*from)->sons.begin(); j != (*from)->sons.end(); j++) {
			*j = dest;
		}
		//(*from)->group->header = dest->header;
		mActionGroups.erase(from);
		delete *from;
		//update_groups();
	}
	for (vector<ActionGroup*>::iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
		//(*i)->print();
	}
}


void ofxTLAntescofoAction::keyPressed_curve_rec(ActionGroup* a, ofKeyEventArgs& key)
{
	ActionMultiCurves* ac = (ActionMultiCurves*)a;
	bool res = false;
	for (vector<ActionCurve*>::iterator i = ac->curves.begin(); !res && i != ac->curves.end(); i++) {
		ActionCurve *c = (*i);
		for (int iac = 0; iac < c->beatcurves.size(); iac++) {
			//if (c->beatcurves[iac]->bounds.inside(args.x, args.y)) {
				cout << "keyPressed dyncast ok" << endl;
				if (c->beatcurves.size() == 1) {
					c->beatcurves[iac]->keyPressed(key);
				}
				break;
			//}
		}
	}

	for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); !res && j != a->sons.end(); j++) {
		keyPressed_curve_rec(*j, key);
	}
}

void ofxTLAntescofoAction::keyPressed(ofKeyEventArgs& args) {
	// transmit DEL key for deleting curve points
	for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
		ActionGroup *a = *i;
		if (a->sons.size()) {
			for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); j != a->sons.end(); j++) {
				// handle arrow click
				if (!(*i)->hidden) { 
					// handle curve click
					ActionMultiCurves* ac = 0;
					if ((ac = dynamic_cast<ActionMultiCurves* >(*j)))
						keyPressed_curve_rec(*j, args);
				}
			}
		}
	}
}

void ofxTLAntescofoAction::setScore(Score* score)
{
	mScore = score;
}

void ofxTLAntescofoAction::show_rec(ActionGroup* a, string label) {
	cout << "ShowRec: label:" << label << endl;
	ActionMultiCurves* ac = 0;
	if ((ac = dynamic_cast<ActionMultiCurves* >(a))) {
		for (vector<ActionCurve*>::iterator i = ac->curves.begin(); i != ac->curves.end(); i++) {
			ActionCurve *c = (*i);
			cout << "ShowRec: comparing with label:" << c->label << endl;
			if (label == c->label) {
				cout << "Show: GOT IT !" << endl;
				a->hidden = false;
				return;
			}
		}
	}
	for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); j != a->sons.end(); j++) {
		show_rec(*j, label);
	}
}

void ofxTLAntescofoAction::show(string label) {
	cout << "Show: label:" << label << endl;
	for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
		 if (*i) {
			ActionGroup* a = *i;
			if (a->sons.size()) {
				for (vector<ActionGroup*>::const_iterator j = a->sons.begin(); j != a->sons.end(); j++) {
					show_rec(*j, label);
				}
			}
		 }
	}
}

string ofxTLAntescofoAction::cut_str(int w, string in)
{
	//cout << "ActionGroup: cut_str: w:"<< w << " in:" << in << " nc=" << in.size() * sizec  << endl;
	if (w < sizec)
		return string("");
	if (w < in.size() * sizec) {
		int nc = w / sizec - 1;
		//cout << "ActionGroup: cut_str: cutting action msg :nc = " << nc << endl;
		return in.substr(0, nc);
	}
	return in;
}


ofRectangle ofxTLAntescofoAction::getBounds()
{
	return bounds;
}


ofRectangle ofxTLAntescofoAction::getBoundedRect(ofRectangle& r)
{
	ofRectangle ret = r;
	if (ret.x < actualBounds.x && ret.x + ret.width > actualBounds.x) {
		ret.x = actualBounds.x;
		ret.width = r.width - ret.x - abs(r.x);
	}
	if (ret.x + ret.width > actualBounds.x + actualBounds.width)
		ret.width = actualBounds.width - ret.x + actualBounds.x;// + abs(r.x);

	//ret.y = ofClamp(ret.y, bounds.y, bounds.y+bounds.height);

	return ret;
}

void ofxTLAntescofoAction::replaceEditorScore(ActionCurve* actioncurve) {
#ifndef ASCOGRAPH_IOS
#ifndef TARGET_LINUX
	vector<Action*>::const_iterator i;
	float d = 0;
	string newscore;
	ostringstream oss;
	string actstr;

	if (actioncurve) {
		actioncurve->parentCurve->antescofo_curve->show(oss);
		actstr = oss.str();
		mAntescofog->replaceEditorScore(actioncurve->parentCurve->lineNum_begin, actioncurve->parentCurve->lineNum_end,
				actioncurve->parentCurve->colNum_begin, actioncurve->parentCurve->colNum_end, actstr);
	}
#endif
#endif
}

void ofxTLAntescofoAction::show_all_curves()
{
	show_all_groups(true);
}

void ofxTLAntescofoAction::show_all_groups_rec(bool bJustCurves, ActionGroup* gf)
{
	vector<ActionGroup*>::const_iterator i;
	for (i = gf->sons.begin(); i != gf->sons.end(); i++) {
		ActionGroup* g = *i;
		if (!bJustCurves || dynamic_cast<ActionMultiCurves* >(g)) {
			g->hidden = false;
			//continue;
		}
		show_all_groups_rec(bJustCurves, g);
	}

}

void ofxTLAntescofoAction::show_all_groups(bool bJustCurves)
{
	for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
		ActionGroup* g = *i;
		if (!bJustCurves || dynamic_cast<ActionMultiCurves* >(g))
			(*i)->hidden = false;
		show_all_groups_rec(bJustCurves, g);
	}
}

void ofxTLAntescofoAction::hide_all_curves()
{
	hide_all_groups(true);
}

void ofxTLAntescofoAction::hide_all_groups_rec(bool bJustCurves, ActionGroup* gf)
{
	vector<ActionGroup*>::const_iterator i;
	for (i = gf->sons.begin(); i != gf->sons.end(); i++) {
		ActionGroup* g = *i;
		if (!bJustCurves || dynamic_cast<ActionMultiCurves* >(g)) {
			g->hidden = true;
			//continue;
		}
		hide_all_groups_rec(bJustCurves, g);
	}

}

void ofxTLAntescofoAction::hide_all_groups(bool bJustCurves)
{
	for (vector<ActionGroup*>::const_iterator i = mActionGroups.begin(); i != mActionGroups.end(); i++) {
		ActionGroup* g = *i;
		if (!bJustCurves || dynamic_cast<ActionMultiCurves* >(g))
			(*i)->hidden = true;
		hide_all_groups_rec(bJustCurves, g);
	}
}

////////////////////////////////////////////////////
void ActionGroup::createActionGroup_fill(Action* action)
{
	if (action) {
		string lab = action->label();
		if (strncmp(lab.c_str(), "__anonymous", 9) != 0 && strncmp(lab.c_str(), "eventgroup", 10) != 0)
			realtitle = lab;
		//if (lab.size() && strncmp(lab.c_str(), "eventgroup", 10) == 0) {
		if (event && event->gfwd == action) {
			top_level_group = true;
			hidden = false;
		}
		lineNum_begin = action->locate()->begin.line;
		lineNum_end = action->locate()->end.line;
		colNum_begin = action->locate()->begin.column;
		colNum_end = action->locate()->end.column;
		Curve* c = dynamic_cast<Curve*>(action);
		Gfwd *g = dynamic_cast<Gfwd*>(action);
		SwitchAction *s = dynamic_cast<SwitchAction*>(action);
		ConditionalAction* ca = dynamic_cast<ConditionalAction*>(action);
		if (c) {
			title = "Curve " + realtitle + "   ";
			rect.height = HEADER_HEIGHT;
			if (debug_actiongroup) cout << "ActionGroup: createActionGroup_fill -----create curve from beatnum=" << beatnum << " ----" << endl;
		} else if (g) {
			title = "Group " + realtitle;
			if (debug_actiongroup) cout << "ActionGroup: createActionGroup_fill -----create group "<< title<< " from beatnum=" << beatnum << " ----" << endl;
			rect.height = HEADER_HEIGHT;
		} else if (s) {
			string sel;
			if (s->selector) {
				ostringstream oss;
				oss << *(s->selector);
				sel = oss.str();
			}
			title = "switch " + sel;//realtitle;
			if (debug_actiongroup) cout << "ActionGroup: createActionGroup_fill -----create switch "<< title<< " from beatnum=" << beatnum << " ----" << endl;
			rect.height = HEADER_HEIGHT;
		} else if (ca) {
			string sel;
			if (ca->cond) {
				ostringstream oss;
				oss << *(ca->cond);
				sel = oss.str();
			}
			title = "if " + sel;//realtitle;
			if (debug_actiongroup) cout << "ActionGroup: createActionGroup_fill -----create if "<< title<< " from beatnum=" << beatnum << " ----" << endl;
			rect.height = HEADER_HEIGHT;
		}
	}
}

void ActionGroup::createActionGroup(Action* tmpa, Event* e, float d) {
	/*
	double delay = 0;
	if (e->gfwd && e->gfwd->delay() && e->gfwd->delay()->value() && e->gfwd->delay()->value()->is_value())
		delay = (double)e->gfwd->delay()->eval();
	*/
	ActionGroup *ag = 0;
	// can be group
	Gfwd *g = dynamic_cast<Gfwd*>(tmpa);
	Message *m = dynamic_cast<Message*>(tmpa);
	AssignmentAction *asa = dynamic_cast<AssignmentAction*>(tmpa);
	Curve* c = dynamic_cast<Curve*>(tmpa);
	Lfwd* l = dynamic_cast<Lfwd*>(tmpa);
	KillAction* k = dynamic_cast<KillAction*>(tmpa);
	ProcCall* p = dynamic_cast<ProcCall*>(tmpa);
	SwitchAction* s = dynamic_cast<SwitchAction*>(tmpa);
	ConditionalAction* ca = dynamic_cast<ConditionalAction*>(tmpa);

	if (g) {
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [group] ("<<action->label()<<")" << endl;
		ag = new ActionGroup(beatnum, d, g, e);
	} else if (m) { // can be message
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [message] ("<<action->label()<<")" << endl;
		ag = new ActionMessage(beatnum, d, m, e); 
	} else if (asa) { // can be an assignment action
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [assignment] ("<<action->label()<<")" << endl;
		ag = new ActionMessage(beatnum, d, asa, e);
	} else if (k) { // can be a kill/abort
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [kill] ("<<action->label()<<")" << endl;
		ag = new ActionMessage(beatnum, d, k, e);
	} else if (p) { // can be a proc call
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [proc call] ("<<action->label()<<")" << endl;
		ag = new ActionMessage(beatnum, d, p, e);
	} else if (c) { // can be a curve
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [curve] ("<<action->label()<<")" << endl;
		ag = new ActionMultiCurves(beatnum, d, c, e);
	} else if (l && l->_group) { // can be a loop
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [loop] ("<<action->label()<<")" << endl;
		ag = new ActionGroup(beatnum, d, l->_group, e);
		if (l->_period.value() && l->_period.value()->is_value())
			ag->period = l->_period.eval();
		ag->realtitle = ag->title = l->label();
	} else if (s) {
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [switch] ("<<action->label()<<")" << endl;
		ag = new ActionSwitch(beatnum, d, s, e);
	} else if (ca) {
		if (debug_actiongroup) cout << "ActionGroup::createActionGroup [if] ("<<action->label()<<")" << endl;
		ag = new ActionSwitch(beatnum, d, ca, e);
	}
	if (ag)
		sons.push_back(ag);
}


ActionGroup::ActionGroup(float beatnum_, float delay_, Action* a, Event *e) 
			: beatnum(beatnum_), delay(delay_), action(a), event(e),
			  period(0.), duration(0.), hidden(true), top_level_group(false),
			  HEADER_HEIGHT(16), ARROW_LEN(15), LINE_SPACE(12), deep_level(0.15)
{
	if (debug_actiongroup) cout << "ActionGroup::ActionGroup("<<action->label()<<")" << endl;
	createActionGroup_fill(action);

	float d = delay;
	Gfwd *g = dynamic_cast<Gfwd*>(action);
	if (g) {
		vector<Action*>::const_iterator i;
		for (i = g->actions().begin(); i != g->actions().end(); i++) {
			Action *tmpa = *i;

			d += get_delay(tmpa);

			if (debug_actiongroup) cout << "ActionGroup::ActionGroup(): son createActionGroup: " << tmpa->label()<< endl;
			createActionGroup(tmpa, event, d);

			duration += get_delay(tmpa);
		}
	}
}

ActionGroup::~ActionGroup()
{
	if (trackName.size()) {
		cout << "ActionGroup: removing track " << trackName << endl;
		_timeline->removeTrack(trackName);
	}

	while(!sons.empty()) delete sons.back(), sons.pop_back();
}

int ActionGroup::getHeight()
{
	return 0;
}


string ActionGroup::dump()
{
	/*
	string groupdump;
	ostringstream oss;

	if (event) {
		oss << *event;
		groupdump += oss.str();
	}

	if (action) {
		string actstr;
		vector<Action*>::const_iterator i;
		for (i = gfwd->actions().begin(); i != gfwd->actions().end(); i++) {
			Action *tmpa = *i;

			oss.str(""); oss.clear();
			//list<Action*> l = ->bloc();
			//for (list<Action*>::iterator a = l.begin(); a != l.end(); a++) {
				if (tmpa)
					oss << *tmpa; // *a;
			//}
			actstr = oss.str();
			groupdump += actstr;
			actstr.clear();
		}

	}
	if (header && !header->top_level_group) {
		list<ActionGroup*>::const_iterator s;
		for (s = sons.begin(); s != sons.end(); s++) {
			groupdump += (*s)->dump();
		}
	}
	return groupdump;
	*/
	return string("");
}


double ActionGroup::get_delay(Action* tmpa) 
{
	double d = 0.;
	if (tmpa && tmpa->delay().is_constant()) {
		d = tmpa->delay().eval();
	}
	return d;
}

void ActionGroup::print() {
	cout << "***** ActionGroup: " << title << " (" << realtitle << ") "<< " line:" << lineNum_begin << ":" << colNum_begin << " -> "<< lineNum_end<< ":" << colNum_end << " beat:" << beatnum 
	     << " delay:"<< delay <<" dur:" << duration << " hidden:"<< hidden << " top_level:" << top_level_group
	     << " x:" << rect.x << " y:" << rect.y << " w:" << rect.width << " h:" << rect.height << endl;

	if (period) cout << "ActionGroup loop period: " << period << endl;

	for (vector<ActionGroup*>::iterator i = sons.begin(); i != sons.end(); i++)
		(*i)->print();
}

bool ActionGroup::is_in_bounds(ofxTLAntescofoAction *tlAction) {
	bool res = false;
	ofRange r(_timeline->screenXtoNormalizedX(rect.x, tlAction->getZoomBounds()), _timeline->screenXtoNormalizedX(rect.x + rect.width, tlAction->getZoomBounds()));
	res = tlAction->getZoomBounds().intersects(r);

	return res;
}

bool ActionGroup::is_in_bounds_y(ofxTLAntescofoAction *tlAction) {
	bool res = false;
	if (rect.y + rect.height + tlAction->mElevatorStartY <= tlAction->getBounds().y + tlAction->getBounds().height + 100
	    || rect.y + tlAction->mElevatorStartY >= tlAction->ofxAntescofoNote->getBounds().y) // tlAction->getBounds().y)
		res = true;
	return res;
}


void ActionGroup::draw_header(ofxTLAntescofoAction* tlAction, bool draw_rect)
{
	if (!is_in_bounds_y(tlAction)) return;
	if (tlAction->mFilterActions && !in_selected_track) return;
	ofFill(); // rect color filled
	ofSetColor(headerColor);
	ofRectangle r(rect);
	r.height = HEADER_HEIGHT;
	ofRect(tlAction->getBoundedRect(r)); // header filled rect
	ofNoFill();
	ofSetColor(0, 0, 0, 255);
	if (rect.width > ARROW_LEN + 4) drawArrow(); // arrow
	string name = title;
	if (period > 0.) {name = "Loop " + realtitle; name += " period:"; name += get_period(); }
	tlAction->mFont.drawString(tlAction->cut_str(rect.width, name), rect.x + 1, rect.y + HEADER_HEIGHT - 5);
	ofNoFill();
	//ofRect(tlAction->getBoundedRect(r)); // black border rect
	//if (draw_rect) ofRect(tlAction->getBoundedRect(rect)); // black border rect
}

void ActionGroup::draw(ofxTLAntescofoAction *tlAction)
{
	ofSetLineWidth(1);
	/*cout << "ActionGroup::draw: label:"<< realtitle << " toplevel:" << top_level_group << " inbounds:" << is_in_bounds(tlAction) <<  " x:"<<rect.x << " y:" << rect.y
	     << " " << rect.width <<  "x"<< rect.height << " sons size: " << sons.size() << endl;
	     */
	bool inbounds = is_in_bounds(tlAction);
	if (!hidden) {
		if (inbounds) {
			if (!(tlAction->mFilterActions && in_selected_track)) { // group not filtered out: display
				ofFill();
				if (debug_actiongroup) cout << "deep level=" << deep_level << endl;
				ofSetColor(200, 200, 200, deep_level*255);
				ofRectangle inrect = rect;
				inrect.y += HEADER_HEIGHT; inrect.height -= HEADER_HEIGHT;

				if (inrect.y + tlAction->mElevatorStartY <= tlAction->getBounds().y) // don't draw outside of bounds during vertical scrolling
					inrect.y = tlAction->getBounds().y;

				ofRect(tlAction->getBoundedRect(inrect)); // draw background
			}
			for ( vector<ActionGroup*>::const_iterator i = sons.begin(); i != sons.end(); i++) {
				if ((*i)->is_in_bounds(tlAction))
					(*i)->draw(tlAction);
			}
		}
	}
	if (inbounds) draw_header(tlAction);
}


void ActionGroup::drawModalContent(ofxTLAntescofoAction *tlAction)
{
	if (is_in_bounds(tlAction))
		for (vector<ActionGroup*>::iterator i = sons.begin(); i != sons.end(); i++)
			(*i)->drawModalContent(tlAction);
}

/* 
   Curve
 */
ActionMultiCurves::ActionMultiCurves(float beatnum_, float delay_, Curve* c, Event* e)
	: resize_factor(0.6), deep_level(0.15)
{
	HEADER_HEIGHT = 16;
	ARROW_LEN = 12;
	beatnum = beatnum_;
	delay = delay_;
	action = c;
	event = e;

	antescofo_curve = c;
	period = 0.;
	isValid = true;
	top_level_group = false;
	if (antescofo_curve) {
		if (debug_edit_curve) cout << "got MultiCurve: " << antescofo_curve->label() << endl; 
		title = string("Curve ") + antescofo_curve->label();

		lineNum_begin = antescofo_curve->locate()->begin.line;
		lineNum_end = antescofo_curve->locate()->end.line;
		colNum_begin = antescofo_curve->locate()->begin.column;
		colNum_end = antescofo_curve->locate()->end.column;

		nbvects = antescofo_curve->seq_vect.size();

		if (debug_edit_curve) cout << "ActionMultiCurves: contains " << nbvects << " vectors." << endl;
		for (uint i = 0; i < nbvects; i++)
		{
			howmany = antescofo_curve->seq_vect[i].var_list->size();
			if (debug_edit_curve) cout << "ActionMultiCurves: contains vect:" << nbvects << " howmany:"<< howmany << endl;
			SeqContFunction& s = antescofo_curve->seq_vect[i];
			list<Var*>::iterator it_var = s.var_list->begin();
			//for (uint j = 0; /*j < s->s_vect[0][0].size() &&*/ it_var != s->var_list->end(); j++, it_var++)
			//{
				string var = (*it_var)->name();
				if (debug_edit_curve) cout << "ActionMultiCurves: adding sub curve:" << endl;

				ActionCurve* newc = new ActionCurve(*(s.var_list), &s, &s.dur_vect, 0, e, this);
				newc->label = antescofo_curve->label();
				curves.push_back(newc);
			//}
		}
		if (e->gfwd == (Action*)c) {
			top_level_group = true;
			hidden = false;
		} else hidden = true;

		bool res = true;
		vector<ActionCurve*>::iterator c;
		for (c = curves.begin(); res && c != curves.end(); c++) {
			(*c)->simple_vect = &(*c)->seq->s_vect[0];
			res &= (*c)->create_from_parser_objects((*c)->vars, (*c)->dur_vect, this);
		}
		if (!res)
			isValid = false;
	}
}

void ActionMultiCurves::draw_header(ofxTLAntescofoAction* tlAction, bool draw_rect)
{
	ActionGroup::draw_header(tlAction, draw_rect);

	if (!is_in_bounds_y(tlAction)) return;
	if (tlAction->mFilterActions && !in_selected_track) return;

	if (!hidden) {
		ofNoFill();
		ofSetColor(0, 0, 0, 255);
		//cout << "draw_header curve: " << realtitle << " " << rect.x+rect.width-20 << " " << rect.y+5  << endl;
		mCurveArrowImgRect.x = rect.x+rect.width-40;
		mCurveArrowImgRect.y = rect.y+1;
		mCurveArrowImgRect.width = 16;
		mCurveArrowImgRect.height = 16;

		if (rect.x+16 <= mCurveArrowImgRect.x) {
			if (!tlAction->mCurveBeingEdited) {
				tlAction->mCurveArrowImgUp.draw(mCurveArrowImgRect);
			}
		}
	}
}

int ActionMultiCurves::update_sub_height(ofxTLAntescofoAction* tlAction, int& cury, int& curh)
{
	rect.y = cury;
	// update curves rectangle bounds
	int boxy = rect.y + HEADER_HEIGHT;
	//int boxh = (bounds.height - c->HEADER_HEIGHT - curh - 5) / c->curves.size();
	int boxh = 120 / curves.size();
	boxh *= resize_factor;
	int boxw = getWidth();
	int boxx = rect.x;
	if (delay) boxx = tlAction->get_x(beatnum /*c->header->delay*/ + delay);
	//cout << "boxx: " << boxx << " delay:" << c->delay << endl;
	ofRectangle cbounds(boxx, boxy, boxw, boxh);
	for (int k = 0; k < curves.size(); k++) {
		ActionCurve *ac = curves[k];
		if (k) cbounds.y += boxh;
		for (int iac = 0; iac < ac->beatcurves.size(); iac++) {
			ac->beatcurves[iac]->setBounds(cbounds);
			ofRange zr(tlAction->getZoomBounds());
			ac->beatcurves[iac]->setZoomBounds(zr);
			ofRectangle r(tlAction->getBounds());
			ac->beatcurves[iac]->setTLBounds(r);
			ac->beatcurves[iac]->viewIsDirty = true;
		}
	}
	curh = rect.height = getHeight();
	return 0;
}



/*
 * Split a multi curve into tracks
 * that means copy for each member of seq_vect
 *      for each var: create a curve
 */
void ActionCurve::split()
{
#ifndef ASCOGRAPH_IOS
#ifndef TARGET_LINUX
	if (debug_edit_curve) cout << "ActionMultiCurves:: split " << parentCurve->antescofo_curve->label() << endl;
	if (parentCurve->antescofo_curve) {
		if (debug_edit_curve) cout << "ActionMultiCurves:: split " << parentCurve->antescofo_curve->label() << endl; 
		label = parentCurve->antescofo_curve->label();
		int nbvects = parentCurve->antescofo_curve->seq_vect.size();

		vector<SeqContFunction> listseq;
		ofxTLAntescofoAction* actionTrack = (ofxTLAntescofoAction *)_timeline->getTrack("Actions");
		if (debug_edit_curve) cout << "ActionMultiCurves: contains " << nbvects << " vectors." << endl;
		int nbtracks = 0;
		for (uint i = 0; i < nbvects; i++)
		{
			parentCurve->howmany = parentCurve->antescofo_curve->seq_vect[i].var_list->size();
			if (debug_edit_curve) cout << "ActionMultiCurves: contains vect:" << nbvects << " howmany:"<< parentCurve->howmany << endl;
			SeqContFunction& s = parentCurve->antescofo_curve->seq_vect[i];
			list<Var*>::iterator it_var = s.var_list->begin();
			// accumulate types
			vector<Expression*> listinterp;

			for (int j = 0; j < s.s_vect[0].size(); j++) {
				listinterp.push_back(s.s_vect[0][j].type);
			}
			for (uint j = 0; /*j < s->s_vect[0][0].size() &&*/ it_var != s.var_list->end(); j++, it_var++)
			{
				nbtracks++;
				vector<Expression*> listexp; 
				vector<SimpleContFunction>* simple_vect = &parentCurve->antescofo_curve->seq_vect[i].s_vect[j]; //&(seq->s_vect[j]);
				// get values
				vector<double> hvalues;
				for (uint j = 0; j != parentCurve->antescofo_curve->seq_vect[i].dur_vect.size() - 1; j++) {
					listexp.push_back((*simple_vect)[j].y0);
				}
				// get last y1
				listexp.push_back((*simple_vect)[simple_vect->size()-1].y1);
				
				antescofo_ascograph_offline *ao = (actionTrack)->mAntescofog->ofxAntescofoNote->mAntescofo;
				//SeqContFunction* n = new SeqContFunction(ao, *it_var, s.dur_vect, listexp, listinterp, s.grain, parentCurve->antescofo_curve);
				//listseq.push_back(*n);
				listseq.push_back(SeqContFunction(ao, *it_var, s.dur_vect, listexp, listinterp, s.grain, parentCurve->antescofo_curve));
			}
		}
		// assign new seq_vect
		vector<SeqContFunction> oldseq = parentCurve->antescofo_curve->seq_vect;
		parentCurve->antescofo_curve->seq_vect = listseq;
		parentCurve->antescofo_curve->show(cout);

		actionTrack->replaceEditorScore(this);
		string newscore;
		[actionTrack->mAntescofog->editor getEditorContent:newscore];
		cout << "split: newscore = " << newscore << endl;
		cout << "split: label size= " << label.size() << endl;
		cout << "split: label = " << label << endl;
		ofxAntescofog* fog = actionTrack->mAntescofog;
		if (((ofxTLAntescofoNote *)_timeline->getTrack("Notes"))->loadscoreAntescofo_fromString(newscore, fog->mScore_filename))
			actionTrack->show(label);
	}
#endif
#endif
}

ActionMultiCurves::~ActionMultiCurves()
{
	while(!curves.empty()) delete curves.back(), curves.pop_back();
}

void ActionMultiCurves::draw(ofxTLAntescofoAction *tlAction) {
	if (tlAction->mFilterActions && !in_selected_track) return;
	if (!is_in_bounds_y(tlAction)) return;
	//cout << "ActionMultiCurves::draw: x:"<< rect.x << " y:" << rect.y << endl;
	bool inbounds = is_in_bounds(tlAction);
	if (!hidden && inbounds) {
		vector<ActionCurve*>::iterator j;
		for (j = curves.begin(); j != curves.end(); j++) {
			(*j)->draw(tlAction);
		}
	}
	if (inbounds) draw_header(tlAction, false);
}

void ActionMultiCurves::drawModalContent(ofxTLAntescofoAction *tlAction) {
	vector<ActionCurve*>::iterator j;
	if (!hidden) {
		for (j = curves.begin(); j != curves.end(); j++) {
			(*j)->drawModalContent(tlAction);
		}
	}
}


void ActionMultiCurves::print() {
	cout << "\t***** ActionMultiCurves: got " << howmany << " curves:" << " hidden: " << hidden << " top_level_group:" << top_level_group
	     << " x:" << rect.x << " y:" << rect.y << " w:" << rect.width << " h:" << rect.height << endl;
	//ActionGroup::print();
	for (int i = 0; i < curves.size(); i++)
		curves[i]->print();
}


string ActionMultiCurves::dump()
{
	string groupdump;
	if (antescofo_curve) {
		ostringstream oss;
		string actstr;

		oss.str(""); oss.clear();
		antescofo_curve->show(oss);
		groupdump = oss.str();
	}

	return groupdump;
}


int ActionMultiCurves::getWidth() {
	int maxw = 0;
	vector<ActionCurve*>::iterator j;
	for (j = curves.begin(); j != curves.end(); j++) {
		int w = (*j)->getWidth();
		if (maxw < w)
			maxw = w;
	}
	return maxw;
}

int ActionMultiCurves::getHeight() {
	if (hidden)
		return HEADER_HEIGHT;

	int h = 0;
	vector<ActionCurve*>::iterator j;
	for (j = curves.begin(); j != curves.end(); j++) {
		h += (*j)->getHeight();
	}
	return h + HEADER_HEIGHT;//* resize_factor;
}

ActionCurve::ActionCurve(list<Var*> &var, SeqContFunction* seq_, vector<AnteDuration*>* dur_vect_, float delay_, Event *e, ActionMultiCurves* parentCurve_)
	: parentCurve(parentCurve_), vars(var), seq(seq_) //, delay(delay_)
{
	vars = var;
	for (list<Var*>::iterator i = var.begin(); i != var.end(); i++) {
		varname += (*i)->name();
		if (i != var.end()) varname += " ";
	}
	simple_vect = &seq->s_vect[0];
	dur_vect = dur_vect_;
}


bool ActionCurve::create_from_parser_objects(list<Var*> &var, vector<AnteDuration*>* dur_vect_, /*float delay_, Event *e, */ ActionMultiCurves* parentCurve_) 
{
	dur_vect = dur_vect_;
	parentCurve = parentCurve_;

	if (debug_edit_curve) cout << "ofxTLAntescofoAction::adding ActionCurve: for var: "<< varname << " : " << dur_vect->size() << " delays" << endl;// << seq->label() << endl; 
	if (values.empty() && delays.empty()) {
		for (int i = 0; i < var.size(); i++) {
			vector<SimpleContFunction>* simple_vect = &(seq->s_vect[i]);
			IntValue* in;
			// get values
			vector<double> hvalues;
			for (uint j = 0; j != dur_vect->size() - 1; j++) {
				FloatValue* f = dynamic_cast<FloatValue*>((*simple_vect)[j].y0);
				if (f) {
					double dou = f->get_double();
					if (debug_edit_curve) cout << "ofxTLAntescofoAction::add_action: got values:" << dou << endl;
					hvalues.push_back(dou);
				} else if ((in = dynamic_cast<IntValue*>((*simple_vect)[j].y0))) {
					int ii = in->get_int();
					if (debug_edit_curve) cout << "ofxTLAntescofoAction::add_action: got values:" << ii << endl;
					hvalues.push_back(ii);
				} else return false;
				//cout << "ActionCurve : got y0:" << s_vect[j].y0->get_double() << " y1:" << s_vect[j].y1->get_double() << " type:"<< s_vect[j].type << endl;
			} 
			// get last y1
			FloatValue* f = dynamic_cast<FloatValue*>((*simple_vect)[simple_vect->size()-1].y1);
			if (f) {
				double dou = f->get_double();
				if (debug_edit_curve) cout << "ofxTLAntescofoAction::add_action: got values:" << dou << endl;
				hvalues.push_back(dou);
			} else if ((in = dynamic_cast<IntValue*>((*simple_vect)[simple_vect->size()-1].y1))) {
				int ii = in->get_int();
				if (debug_edit_curve) cout << "ofxTLAntescofoAction::add_action: got values:" << ii << endl;
				hvalues.push_back(ii);
			} else return false;
			values.push_back(hvalues);
		}
		// get delays
		double groupDelay = 0;//parentCurve->delay;
		for (std::vector<AnteDuration*>::const_iterator j = dur_vect->begin(); j != dur_vect->end(); j++) {
			if ((*j)->is_constant()) {
				double dou = (*j)->eval();
				if (j == dur_vect->begin())
					dou += groupDelay;
				if (debug_edit_curve) cout << "ofxTLAntescofoAction::add_action: got delay:" << dou << endl;
				delays.push_back(dou);
			} else return false;
		}
	}
	//cout << " ============ values:"<< values.size() << " delays:"<<  delays.size() << " ========= " << endl;
	// add object
	if (values.size() && delays.size() && (*(values.begin())).size()) {
		// search value min/max

		double min = 0, max = 0;
		for (int i = 0; i < parentCurve->howmany && i < values.size(); i++) {
			// set values ranges
			int j = 0;
			for (vector<double>::iterator ii = values[i].begin(); ii != values[i].end(); ii++) {
				if (min > (*ii)) min = (*ii);
				if (max < (*ii)) max = (*ii);
			}
		}	
		ofColor color(250, 0, 0, 255);
		for (int i = 0; i < parentCurve->howmany && i < values.size(); i++) {
			BeatCurve* curve = new BeatCurve();
			curve->setTimeline(_timeline);

			curve->keyColor = color;
			color.r += 255; color.r %= 255;
			color.g += 115; color.g %= 255;
			color.b += 15; color.b %= 255;

			curve->tlAction = (ofxTLAntescofoAction *)_timeline->getTrack("Actions");
			curve->setTLTrack(curve->tlAction);
			parentCurve->tlAction = curve->tlAction;

			int boxh = (curve->tlAction->getBounds().height - parentCurve->HEADER_HEIGHT - 5) / parentCurve->curves.size();
			int boxy = parentCurve->rect.y + parentCurve->HEADER_HEIGHT + i * boxh;
			int boxw = 0;//getWidth();// + 30;
			int boxx;
			if (parentCurve->delay) boxx = curve->tlAction->get_x((parentCurve->beatnum + parentCurve->delay));
			else boxx = parentCurve->rect.x;
			ofRectangle bounds(boxx, boxy, boxw, boxh);
			curve->setBounds(bounds);
			ofRange zr = curve->tlAction->getZoomBounds();
			curve->setZoomBounds(zr);
			ofRectangle r(curve->tlAction->getBounds());
			curve->setTLBounds(r);
			curve->viewIsDirty = true;
			curve->ref = this;

			curve->setValueRangeMax(max);
			curve->setValueRangeMin(min);
			// set keyframes
			//   calculate absolute delay for keyframes
			//ActionGroup *parentgrp = parentCurve->parentGroup;
			double dcumul = parentCurve->delay;
			/*while (parentgrp) {
				dcumul += parentgrp->delay;
				parentgrp = parentgrp->parentGroup;
			}*/
			if (debug_edit_curve) cout << "====================== dcumul=" << dcumul << endl;
			//cout << "====================== parentdelay= " << parentCurve->parentGroup->delay << endl;
			vector<SimpleContFunction>::iterator s = simple_vect->begin();
			for (int k = 0; k < delays.size(); k++, s++) {
				dcumul += delays[k];
				float b = parentCurve->beatnum + dcumul;
				if (debug_edit_curve) cout << "ofxTLAntescofoAction::add_action: CURVE add keyframe[" << k << "] beat=" << b << " msec=" << _timeline->beatToMillisec(b)
					<< " val=" <<  values[i][k] << endl;
				string easetype;
				if (s != simple_vect->end() && s->type && s->type->is_value()) {
					Value* v = (Value*)s->type->is_value();
					//*v = StringValue(type);
					ostringstream oss; 
					oss << *v;
					easetype = oss.str();
				}
				curve->addKeyframeAtBeat(values[i][k], parentCurve->beatnum + dcumul);
				if (easetype.size() && easetype != "linear" && easetype != "\"linear\"")
					curve->changeKeyframeEasing(parentCurve->beatnum + dcumul, easetype);
			}
			beatcurves.push_back(curve);
		}
	}
	return true;
}


ActionCurve::~ActionCurve()
{
	while(!beatcurves.empty()) delete beatcurves.back(), beatcurves.pop_back();
	// XXX should delete parser objects,
	// but for some incredible reason now it crashes (worked til 28/01/14)
	//if (dur_vect) while(!dur_vect->empty()) delete dur_vect->back(), dur_vect->pop_back();
}


bool ActionCurve::set_dur_val(double d, AnteDuration* a)
{
	if (debug_edit_curve) cout << "ActionCurve::set_dur_val: " << d << endl;
	//assert(d);
	if (!d) return false;
	Value* v;
	if (a->value() && (v = (Value*)a->value()->is_value())) {
		*v = FloatValue(d);
		return true;
	} else {
		cerr << "ActionCurve: ERROR : set_dur_val: can not convert to FloatValue" << a->value() << endl; 
		return false;
	}
}

bool ActionCurve::set_y(Expression* y, double val) {
	Value* v = (Value*)y->is_value();
	if (v) {
		*v = FloatValue(val);
		return true;
	} else {
		cerr << "ActionCurve: ERROR : set_y: can not convert to FloatValue" << endl; 
		return false;
	}
}


void ActionCurve::print()
{
	cout << "\tActionCurve::print: "<< endl; 
	vector<SimpleContFunction>::iterator s = simple_vect->begin();
	for (vector<AnteDuration*>::iterator k = dur_vect->begin(); k != dur_vect->end(); k++, s++) {
		if (*k) cout << "\t\tdelay: " << (*k)->eval();
		if (s != simple_vect->end())
			cout << "  y0: " << s->y0 << " y1: " << s->y1 << endl;
	}
	cout << endl;
}

FloatValue* ActionCurve::get_new_y(Expression* y) {
	if (y && y->is_value() && y->is_value()->is_numeric()) {
		if (debug_edit_curve) cout << "get_new_y: " << eval_double(y->is_value()) << endl;
		return new FloatValue(eval_double(y->is_value()));
	} else return NULL;
}

void ActionCurve::deleteKeyframeAtBeat(float beat_) {
	double beat = beat_;
	if (beat == 0) {
		//set_dur_val(val, simple_vect->begin());
		//abort();
		return;
	}

	double dcumul = parentCurve->beatnum;
	int i = 0;
	bool done = false;

	if (dur_vect->size() == 2) {
		cerr << "Can not delete point because curves need at least 2 points" << endl;
		return;
	}

	if (debug_edit_curve) { cout << "Entering deleteKeyframeAtBeat: " << beat << " dur_vect size:" << dur_vect->size() << endl; parentCurve->antescofo_curve->show(cout); print(); }
	vector<SimpleContFunction>::iterator s = simple_vect->begin();
	for (vector<AnteDuration*>::iterator k = dur_vect->begin(); k != dur_vect->end() /* && s != simple_vect->end()*/; k++, i++, s++) {
		dcumul += (*k)->eval();
		if (debug_edit_curve) cout << "ofxTLAntescofoAction:: del keyframe  looping : " << i << " curdur:" << (*k)->eval() <<  " dcumul:" << dcumul << " beat:" << beat<< endl;

		//if dcumul < beat
		if (fabs(dcumul - beat) > 0.001) {
			//cout << "------------------ continue" << endl;
			continue;
		}
		else {
			//cout << "------------------ Entering else" << endl;
			vector<AnteDuration*>::iterator p = k;
			p--;
			// delete k, but change prev duration += k duration
			if (s == simple_vect->end()) { //k == dur_vect->end()) {
				//cout << "%%%%%%%%%%%%%%%%%%%%% Entering ==" << endl;
				dur_vect->erase(k);
				simple_vect->erase(s);
			} else {
				vector<AnteDuration*>::iterator n = k;
				//cout << "%%%%%%%%%%%%%%%%%%%%% Entering !=" << endl;
				n++;
				if (set_dur_val((*n)->eval() + (*k)->eval(), *n)) {
					// add new point to simple_vect[]
					if (debug_edit_curve) cout << "Next point duration : "<< (*n)->eval() << endl;
					dur_vect->erase(k);
					vector<SimpleContFunction>::iterator sp = s;
					sp--;
					sp->y1 = get_new_y(s->y1);
					simple_vect->erase(s);
				} else { cout << "Can not convert to Value." << endl; }
			}
			break;
		}
	}
	if (!done && dcumul < beat) { // delete last point
		cout << "ofxTLAntescofoAction:: del keyframe: delete NOT done !!!!!!!!!!!!!!!!" << endl;
		/*
		vector<AnteDuration*>::iterator k = dur_vect->end();
		k--;
		vector<SimpleContFunction>::iterator s = simple_vect->end();
		s--;
		AnteDuration *ad = new AnteDuration(beat - (*k)->eval());
		simple_vect->push_back(SimpleContFunction(s->antesc, new StringValue("linear"), ad, get_new_y(s->y1), new FloatValue(val), s->var));
		dur_vect->push_back(ad);
		//set_dur_val(dcumul - beat, *k)
		*/
	}
	parentCurve->antescofo_curve->show(cout);
	print();
}

// when new breakpoint is created, we should reduce prev breakpoint duration, before adding
bool ActionCurve::addKeyframeAtBeat(float beat, float val)
{
	if (debug_edit_curve) { cout << "ofxTLAntescofoAction:: add keyframe at beat: " << beat << " val: " << val << endl; print(); }
	if (beat == 0) {
		//set_dur_val(val, simple_vect->begin());
		return false;
	}
	cout << "ofxTLAntescofoAction:: add keyframe at beat: " << beat << " howmany:" << parentCurve->howmany << endl;
	if (parentCurve->howmany > 1) // do not support multicurves editing for now
		return false;

	double dcumul = parentCurve->beatnum;// + delay;
	int i = 0;
	bool done = false;

	vector<SimpleContFunction>::iterator s = simple_vect->begin();
	for (vector<AnteDuration*>::iterator k = dur_vect->begin(); k != dur_vect->end() /* && s != simple_vect->end()*/; k++, i++, s++) {
		if (debug_edit_curve) cout << "ofxTLAntescofoAction:: add keyframe at beat: looping : " << i << " curdur:" << (*k)->eval() <<  " dcumul:" << dcumul<< endl;

		dcumul += (*k)->eval();
		//cout << "------ y0 " << (*s).y0 << endl; cout << " y1:" << (*s).y1 << endl;
		if (dcumul < beat)
			continue;
		else {
			vector<AnteDuration*>::iterator p = k;
			p--;
			// insert point between p and k 
			// substract (dcumul - beat) on prev point
			double d = (*k)->eval() - (dcumul - beat);// - (*p)->eval();
			if (set_dur_val(dcumul - beat, *k)) {
				// add new point to simple_vect[]
				if (debug_edit_curve) cout << "New point duration : beat:"<< beat << " k:"<< (*k)->eval() << " duration:" << d << endl;
				if (i <= simple_vect->size()) {
					vector<SimpleContFunction>::iterator sp = s; sp--;
					AnteDuration* ad = new AnteDuration(d);
					FloatValue* ny1 = get_new_y(sp->y1);
					//assert(ny1);
					if (!ny1) cout << "addKeyframeAtBeat:ERROR : not an constant value in curve...." << endl;
					FloatValue* ny0 = new FloatValue(val);
					//s = simple_vect->insert(s, SimpleContFunction(sp->antesc, new StringValue("linear"), ad, ny0, ny1, s->var));
					s = simple_vect->insert(s, SimpleContFunction(sp->antesc, new StringValue("linear"), ad, ny0, ny1, s->var, parentCurve->antescofo_curve));
					dur_vect->insert(k, ad);
					// change prev y1
					s--;
					s->y1 = ny0;
					done = true;
					if (debug_edit_curve) cout << "looping stopped, inserted: " << i << endl;
				} 
			} else { cout << "ERROR: Can not convert to Value." << endl; }

			break;
		} 
	}
	if (!done && dcumul < beat) { // insert beat after last point
		vector<AnteDuration*>::iterator k = dur_vect->end();
		k--;
		vector<SimpleContFunction>::iterator s = simple_vect->end();
		s--;
		AnteDuration *ad = new AnteDuration(beat - parentCurve->beatnum - (*k)->eval());
		simple_vect->push_back(SimpleContFunction(s->antesc, new StringValue("linear"), ad, get_new_y(s->y1), new FloatValue(val), s->var, parentCurve->antescofo_curve));
		dur_vect->push_back(ad);
		//set_dur_val(dcumul - beat, *k)
	}
	parentCurve->antescofo_curve->show(cout);
	print();
	return true;
}


void ActionCurve::changeKeyframeEasing(float beat, string type) {
	//if (debug_edit_curve) 
	cout << "ActionCurve::changeKeyframeEasing: beat:"<< beat << " type:"<< type << endl;
	
	float dcumul = parentCurve->beatnum;
	int i = 0;
	bool done = false;

	vector<SimpleContFunction>::iterator s = simple_vect->begin();
	for (vector<AnteDuration*>::iterator k = dur_vect->begin(); k != dur_vect->end(); k++, i++, s++) {
		if (debug_edit_curve) cout << "ofxTLAntescofoAction:: change keyframe easing at beat: looping : " << i << " curdur:" << (*k)->eval() <<  " dcumul:" << dcumul<< endl;

		dcumul += (*k)->eval();

		if (dcumul < beat)
			continue;
		else {
			if (s != simple_vect->end() && s->type && s->type->is_value()) {
				if (debug_edit_curve) cout << "change: is value:" << s->type->is_value() << endl;
				if (debug_edit_curve) cout << "ofxTLAntescofoAction:: change keyframe" << endl;
				Value* v = (Value*)s->type->is_value();
				//StringValue *sv = dynamic_cast<StringValue*>(s->type);
				std::transform(type.begin(), type.end(), type.begin(), ::tolower);
				*v = StringValue(type);
				done = true;
				break;
			}
		}
	}

	if (done) {
		parentCurve->antescofo_curve->show(cout);
		print();
	} else {
		cout << "!!!!!ERROR!!!! Could not find keyframe at beat: " << beat << " dcumul:" << dcumul << endl;
	}
}


// 
void ActionCurve::moveKeyframeAtBeat(float to_beat, float from_beat, float to_val, float from_val)
{
	if (debug_edit_curve) 
	{
		cout << "ofxTLAntescofoAction:: move keyframe from beat: " << from_beat << " (val:" << from_val << ") to beat: " << to_beat << "(val:" << to_val <<")"<< endl;
		print();
	}
	//if (from_beat == 0) { // TODO set val return; }

	double dcumul = 0.;
	double dfrom_beat = from_beat;
	int i = 0;

	vector<SimpleContFunction>::iterator s = simple_vect->begin();
	for (vector<AnteDuration*>::iterator k = dur_vect->begin(); k != dur_vect->end(); k++, i++, s++) {
		if (debug_edit_curve) cout << "ofxTLAntescofoAction:: move keyframe at beat: looping : " << i << endl;

		dcumul += (double)((*k)->eval());

		if (dcumul < dfrom_beat)
			continue;
		else if ( dcumul > dfrom_beat - 0.01 && dcumul < dfrom_beat + 0.01) { //dcumul == dfrom_beat) 
			if (debug_edit_curve) cout << "ofxTLAntescofoAction:: beat found" << endl;
			// prev dur -= (from_beat - to_beat)
			vector<AnteDuration*>::iterator p = k;
			p--;
			AnteDuration *d = *k;
			if (set_dur_val(d->value()->is_value()->get_double() - (from_beat - to_beat), *k)) {
				// change current delay
				//if (debug_edit_curve) cout << "Changed point duration : " << to_beat - (*p)->eval() << endl;
				// change next
				if (k != dur_vect->end()) {
					double nextdur = 0.;
					vector<AnteDuration*>::iterator n = k; n++;
					AnteDuration *an = *n;
					if (an) {
						if (an->value() && an->value()->is_value()->get_double() + from_beat < to_beat) { // point dragged after next point 
							cout << "!!!!!!!!!!!!!!!!!!!!!!! negative delay TODO" << endl;

						} else {
							nextdur = an->value()->is_value()->get_double() + (from_beat - to_beat);
							set_dur_val(nextdur, *n);
						}
					}
				}
				vector<SimpleContFunction>::iterator sp = s;
				if (s != simple_vect->end())
					set_y(s->y0, to_val);
				s--;
				set_y(s->y1, to_val);
			}
			break;
		} else {
			cout << "An error moveKeyframeAtBeat...: dcumul:" << dcumul << " from_beat:"<< dfrom_beat << endl;
			abort();
		}
	}

	parentCurve->antescofo_curve->show(cout);
	print();
}


void ActionMultiCurves::setWidth(int w) {
	for (int i = 0; i < curves.size(); i++) {
		curves[i]->setWidth(w);
	}
}

void ActionCurve::setWidth(int w) {
	//cout << "--------------------------------setting Width on \'" << parentCurve->title << "\" : " << parentCurve->rect.width << endl;
	parentCurve->rect.width = w;
	for (int i = 0; i < beatcurves.size(); i++) {
		beatcurves[i]->bounds.width = w;
	}	
}


int ActionCurve::getWidth() {
	int maxx = 0, minx = 0;
	for (int i = 0; i < beatcurves.size(); i++) {
		ofRectangle tlbounds = beatcurves[i]->tlAction->getBounds();
		ofRange tlzbounds = beatcurves[i]->tlAction->getZoomBounds();
		beatcurves[i]->setTLBounds(tlbounds);
		beatcurves[i]->setZoomBounds(tlzbounds);
		//beatcurves[i]->recomputePreviews();
		if (beatcurves[i]->keyframes.size() == 0) { cout << "ActionCurve::getWidth ERROR null keyframes, skippin' i=" <<i << endl; continue; }
		int x1 = beatcurves[i]->tlAction->get_x( beatcurves[i]->keyframes[0]->beat );
		int l = beatcurves[i]->keyframes.size() - 1;
		int x2 = beatcurves[i]->tlAction->get_x( beatcurves[i]->keyframes[l]->beat );

		if (!minx || minx > x1)
			minx = x1;
		if (!maxx || maxx < x2) {
			maxx = x2;
		}
	}

	int d = maxx - minx;
	return d;
}

int ActionCurve::getHeight() {
	if (beatcurves.empty())
		return 0;
	return beatcurves[0]->bounds.height;
}

void ActionCurve::draw(ofxTLAntescofoAction* tlAction) {
	if (beatcurves.empty()) return;

	ofColor col = _timeline->getColors().backgroundColor;
	ofSetColor(col.r + 30, col.g + 30, col.b + 20, parentCurve->deep_level*255);

	ofFill();
	ofRect(beatcurves[0]->bounds);

	for (int i = 0; i < beatcurves.size(); i++) {
		beatcurves[i]->draw();
		ofSetColor(0, 0, 0, 255);
		ofDrawBitmapString(varname, beatcurves[i]->bounds.x + 8, beatcurves[i]->bounds.y + 15);
	}

	ofNoFill();
	ofSetColor(0, 0, 0, 255);
	ofRect(beatcurves[0]->bounds);

	if (beatcurves.size() > 1)
		drawSplitBtn(tlAction);
}

void ActionCurve::drawSplitBtn(ofxTLAntescofoAction *tlAction) {
	mSplitBtnRect.width = 50;
	if (parentCurve->rect.width < mSplitBtnRect.width) return;
	ofPushStyle();
	ofFill();
	ofSetColor(_timeline->getColors().backgroundColor);
	mSplitBtnRect.x = beatcurves[0]->bounds.x + beatcurves[0]->bounds.width - 54;
	mSplitBtnRect.y = beatcurves[0]->bounds.y + 4;
	mSplitBtnRect.height = 14;
	ofRect(mSplitBtnRect);
	ofSetColor(0);
	ofNoFill();
	ofRect(mSplitBtnRect);

	ofDrawBitmapString("Split",  mSplitBtnRect.x + 4, mSplitBtnRect.y + 10);

	ofPopStyle();
}




void ActionCurve::drawModalContent(ofxTLAntescofoAction* tlAction) {
	if (beatcurves.empty()) return;
	for (int i = 0; i < beatcurves.size(); i++) {
		beatcurves[i]->drawModalContent();
	}
}

/*
	Message
 */
ActionMessage::ActionMessage(float beatnum_, float delay_, Action* a, Event *e) 
	: is_kill(false), is_proc(false)
{
	period = 0, duration = 0., hidden = true,
	HEADER_HEIGHT = 16, ARROW_LEN = 15, LINE_SPACE = 12;

	top_level_group = false;
	beatnum = beatnum_;
	delay = delay_;
	event = e;
	createActionGroup_fill(a);

	ostringstream oss;
	oss << *a;
	actionstr = oss.str();
	action = a;
	hidden = false;

	if (dynamic_cast<KillAction*>(a))
		is_kill = true;

	if (dynamic_cast<ProcCall*>(a))
		is_proc = true;

	if (debug_actiongroup) 	cout << "Action: adding message with delay: " << delay << " : " << actionstr << endl;
}

void ActionMessage::print() {
	cout << "\t**** Action Message: " <<  action 
	     << " x:" << rect.x << " y:" << rect.y << " w:" << rect.width << " h:" << rect.height << endl;
}

int ActionMessage::getHeight() {
	return HEADER_HEIGHT;
}

int ActionMessage::update_sub_height(ofxTLAntescofoAction* tlAction, int& cury, int& curh)
{
	if (tlAction->mFilterActions && !in_selected_track) {
		curh = 0;
		rect.height = 0;
		return 0;
	}
	rect.x = tlAction->get_x(beatnum + delay);
	rect.y = cury;
	rect.height = HEADER_HEIGHT;
	curh = HEADER_HEIGHT;
	return 0;
}

void ActionMessage::draw(ofxTLAntescofoAction* tlAction) {
	if (tlAction->mFilterActions && !in_selected_track) return;
	if (!is_in_bounds_y(tlAction)) return;

	ofFill();
	ofSetColor(200, 200, 200, 255); //deep_level*255);
	ofRect(tlAction->getBoundedRect(rect));

	ofNoFill();
	ofSetColor(0, 0, 0, deep_level*255);
	int strw = rect.width + rect.x - MAX(rect.x, 0);
	//cout << "==========(((((((((((((((((============== strw=" << strw << " rw=" << rect.width << " rx=" << rect.x << endl;
	if (is_kill )
		ofSetColor(255, 0, 0, 255);
	else if (is_proc)
		ofSetColor(0, 168, 0, 255);
	else
		ofSetColor(0, 0, 0, 255);
	//cout << "Draw msg:" << tlAction->cut_str(strw, actionstr) << " x=" << rect.x+1 << " y=" << rect.y + HEADER_HEIGHT - 3 << endl;
	tlAction->mFont.drawString(tlAction->cut_str(strw, actionstr), rect.x + 1, rect.y + HEADER_HEIGHT - 3);

	if (is_kill )
		ofSetColor(255, 0, 0, 80);
	else
		ofSetColor(0, 0, 0, 80);
	ofRect(tlAction->getBoundedRect(rect));
}



string ActionGroup::get_period()
{
	string ret;
	std::ostringstream oss;
	oss << period;

	ret = oss.str();
	return ret;
}


/*
   2
   1
   3
   */
void ActionGroup::drawArrow() {
	//cout << "ActionRects.draw arrow " << title << " ("<<rect.x<<","<<rect.y<<") : hidden:" << hidden << endl; 
	int xlen = ARROW_LEN;
	int space = 3;
	int halfHeight = HEADER_HEIGHT / 2;

	int x1, y1, x2, y2, x3, y3;
	if (hidden) {
		x1 = rect.x + rect.width - xlen - space;
		y1 = rect.y + halfHeight;
		x2 = rect.x + rect.width - space;
		y2 = rect.y + space;
		x3 = x2;
		y3 = rect.y + HEADER_HEIGHT - space;
	} else {
		x1 = rect.x + rect.width - xlen - space;
		y1 = rect.y + space;
		x2 = rect.x + rect.width - space;
		y2 = y1;
		x3 = rect.x + rect.width - space - xlen/2;
		y3 = rect.y + HEADER_HEIGHT - space;
	}

	arrow.setFillColor(ofColor(0, 0, 0, 200));
	arrow.setFilled(true);
	arrow.lineTo(x1, y1);
	arrow.lineTo(x2, y2);
	arrow.lineTo(x3, y3);
	arrow.lineTo(x1, y1);
	arrow.close();
	arrow.draw();
	arrow.clear();
}


bool ActionGroup::is_in_header(int x, int y)
{
	/* was: is_in_arrow
	bool res = false;
	if (x > rect.x + rect.width - ARROW_LEN && y < rect.y + HEADER_HEIGHT)
		res = true;
	return res;
	*/
	//cout << "is in arrow: " << x << " " << y << " return " << res << endl;
	ofRectangle r(rect);
	r.height = HEADER_HEIGHT;
	return r.inside(x, y);
}

void ofxTLAntescofoAction::close_down_curve_editor(ActionMultiCurves* c) {
	mCurveBeingEdited = NULL;
}


void ofxTLAntescofoAction::open_up_curve_editor(ActionMultiCurves* c) {
	mCurveBeingEdited = c;
	c->hidden = false;
}

void ofxTLAntescofoAction::draw_curve_big() {
	if (mCurveBeingEdited) {
		mCurveBeingEdited->draw_big(this);
	}
}


void ActionMultiCurves::draw_big(ofxTLAntescofoAction* tlAction) {
	ofRectangle notebounds = tlAction->ofxAntescofoNote->getBounds();
	ofPushStyle();
	ofFill();
	ofSetColor(0, 0, 0, 74);
	ofRect(notebounds);
	ofPopStyle();

	// update
	int border_height = 15; // y distance between note track top and curve rect
	notebounds.y += border_height;
	notebounds.height -= border_height*2;
	int cury = notebounds.y;
	rect.y = cury;
	int boxy = rect.y;// + HEADER_HEIGHT;
	//int boxh = (bounds.height - c->HEADER_HEIGHT - curh - 5) / c->curves.size();
	int boxh = notebounds.height / curves.size();
	//boxh *= resize_factor;
	int boxw = getWidth();
	int boxx = rect.x;
	if (delay) boxx = tlAction->get_x(beatnum /*c->header->delay*/ + delay);
	//cout << "ActionMultiCurves::draw_big: notebounds.height:" << notebounds.height << " boxx: " << boxx << " delay:" << delay << " y:" << boxy << " boxw=" << boxw << " boxh=" << boxh << endl;
	ofSetColor(255, 255, 255, 145);
	ofFill();
	ofRect(boxx, boxy, boxw, notebounds.height);
	ofSetColor(0, 0, 0, 255);
	ofNoFill();
	ofRect(boxx, boxy, boxw, notebounds.height);
	ofRectangle cbounds(boxx, boxy, boxw, boxh);
	for (int k = 0; k < curves.size(); k++) {
		ActionCurve *ac = curves[k];
		if (k) cbounds.y += boxh;
		for (int iac = 0; iac < ac->beatcurves.size(); iac++) {
			ac->beatcurves[iac]->setBounds(cbounds);
			ofRange zr(tlAction->getZoomBounds());
			ac->beatcurves[iac]->setZoomBounds(zr);
			ac->beatcurves[iac]->setTLBounds(notebounds);
			ac->beatcurves[iac]->viewIsDirty = true;
		}
	}
	int curh = rect.height = getHeight();

	// draw
	if (tlAction->mFilterActions && !in_selected_track) return;
	vector<ActionCurve*>::iterator j;
	for (j = curves.begin(); j != curves.end(); j++) {
		(*j)->draw(tlAction);
	}

	// draw grid bar
	ofFill(); ofSetColor(255, 255, 255, 155); // white rect
	int bottomy = notebounds.y + notebounds.height;
	ofRect(rect.x, bottomy, rect.width, border_height);
	ofSetColor(0, 0, 0, 255);
	int maxb = beatnum+delay + _timeline->normalizedXToBeat( _timeline->screenXtoNormalizedX(getWidth(), tlAction->getZoomBounds()) );
	float step = 1.; // TODO
	for (int b = beatnum+delay; b <= maxb; b += step) { // draw beat ticks + string num
		int x = tlAction->get_x(b);
		ofRect(x, bottomy, 1, border_height/3); // draw tick
		string num = ofToString(b+1);
		tlAction->mFont.drawString(num, x, bottomy+border_height/2 + 7);
	}
	for (float b = beatnum+delay; b <= maxb; b += step/10) { // draw small beat ticks
		float x = tlAction->get_x(b);
		int h = border_height/3;
		if (int(floor(b*10)) % 5 == 0) h += 6;
		ofRect(x, bottomy, 1, h); // draw tick
	}

	if (tlAction->mCurveBeingEdited == this) {
		mCurveArrowImgRect.x = rect.x - 20;
		mCurveArrowImgRect.y = notebounds.y + 20;
		mCurveArrowImgRect.width = 16;
		mCurveArrowImgRect.height = 16;
		tlAction->mCurveArrowImgDown.draw(mCurveArrowImgRect);
	}
}

bool ActionMultiCurves::mousePressed(ofMouseEventArgs& args, long millis) {
	bool res = false;
	cout << "Calling ActionMultiCurves::mousePressed" << endl;
	for (vector<ActionCurve*>::iterator i = curves.begin(); !res && i != curves.end(); i++) {
		ActionCurve *c = (*i);
		for (int iac = 0; !res && iac < c->beatcurves.size(); iac++) {
			if (c->beatcurves[iac]->bounds.inside(args.x, args.y) || c->beatcurves[iac]->drawingEasingWindow) {
				cout << "Calling ActionMultiCurves::mousePressed beatcurve" << endl;
				res = c->beatcurves[iac]->mousePressed(args, millis);
			}
		}
	}
	return res;
}

bool ActionMultiCurves::mouseMoved(ofMouseEventArgs& args, long millis) {
	bool res = false;
	for (vector<ActionCurve*>::iterator i = curves.begin(); !res && i != curves.end(); i++) {
		ActionCurve *c = (*i);
		for (int iac = 0; !res && iac < c->beatcurves.size(); iac++) {
			if (c->beatcurves[iac]->bounds.inside(args.x, args.y) || c->beatcurves[iac]->drawingEasingWindow) {
				c->beatcurves[iac]->mouseMoved(args, millis);
				res = true;
			}
		}
	}
	return res;
}

bool ActionMultiCurves::mouseDragged(ofMouseEventArgs& args, long millis) {
	bool res = false;
	for (vector<ActionCurve*>::iterator i = curves.begin(); !res && i != curves.end(); i++) {
		ActionCurve *c = (*i);
		for (int iac = 0; !res && iac < c->beatcurves.size(); iac++) {
			if (c->beatcurves[iac]->bounds.inside(args.x, args.y) || c->beatcurves[iac]->drawingEasingWindow) {
				c->beatcurves[iac]->mouseDragged(args, millis);
				res = true;
			}
		}
	}
	return res;
}

bool ActionMultiCurves::mouseReleased(ofMouseEventArgs& args, long millis) {
	bool res = false;
	for (vector<ActionCurve*>::iterator i = curves.begin(); !res && i != curves.end(); i++) {
		ActionCurve *c = (*i);
		for (int iac = 0; !res && iac < c->beatcurves.size(); iac++) {
			if (c->beatcurves[iac]->bounds.inside(args.x, args.y) || c->beatcurves[iac]->drawingEasingWindow) {
				c->beatcurves[iac]->mouseReleased(args, millis);
				res = true;
			}
		}
	}
	return res;
}


#ifdef ACTION_SHOW_SWITCH
ActionSwitch::ActionSwitch(float beatnum_, float delay_, Action* a, Event* e) {
	period = 0, duration = 0., hidden = true,
	HEADER_HEIGHT = 16, ARROW_LEN = 15, LINE_SPACE = 12;

	top_level_group = false;
	beatnum = beatnum_;
	delay = delay_;
	event = e;
	action = a;
	createActionGroup_fill(a);

	SwitchAction* sw = dynamic_cast<SwitchAction*>(a);
	ConditionalAction* ca = dynamic_cast<ConditionalAction*>(a);
	if (sw) {
		is_switch = true;
		ostringstream oss;
		string str;
		for (int i = 0; i < sw->cases.size(); i++) {
			// get cases exprs strings
			oss << * (sw->cases[i].first);
			str = oss.str();
			cases.push_back(str);
			oss.clear(); oss.str("");
			// get actions 
			ActionGroup* ag = create_ActionGroup(beatnum, sw->cases[i].second, e);
			//actions_cases.push_back(ag);
			sons.push_back(ag);
		}
	} else if (ca) {
		is_switch = false;
		cases.push_back("");
		ActionGroup* ag = create_ActionGroup(beatnum, ca->if_true, e);
		//actions_cases.push_back(ag);
		sons.push_back(ag);
		cases.push_back("else");
		ag = create_ActionGroup(beatnum, ca->if_false, e);
		//actions_cases.push_back(ag);
		sons.push_back(ag);
	}
	hidden = false;
}

void ActionSwitch::draw(ofxTLAntescofoAction* tlAction) {
	if (tlAction->mFilterActions && !in_selected_track) return;
	if (!is_in_bounds_y(tlAction)) return;

	ofFill();
	ofSetColor(200, 200, 200, 255);
	ofRect(tlAction->getBoundedRect(rect));

	ofNoFill();
	ofSetColor(0, 0, 0, deep_level*255);
	int strw = rect.width + rect.x - MAX(rect.x, 0);
	ofSetColor(0, 0, 0, 255);
	//cout << "Draw msg:" << tlAction->cut_str(strw, actionstr) << " x=" << rect.x+1 << " y=" << rect.y + HEADER_HEIGHT - 3 << endl;
	//tlAction->mFont.drawString(tlAction->cut_str(strw, actionstr), rect.x + 1, rect.y + HEADER_HEIGHT - 3);
	for (int i = 0; i < sons.size(); i++) {
		//TODO DRAW IF OR ELSE tlAction->mFont.drawString(tlAction->cut_str(strw, cases[i]), rect.x + 1, rect.y + HEADER_HEIGHT -3);
		ActionGroup* g = sons[i];//actions_cases[i];
		if (g)
			g->draw(tlAction);
	}
	ofSetColor(0, 0, 0, 80);
	ofRect(tlAction->getBoundedRect(rect));
	draw_header(tlAction);
}

void ActionSwitch::print() {
	cout << "\t**** Action Switch: " <<  title << " x:" << rect.x << " y:" << rect.y << " w:" << rect.width << " h:" << rect.height << endl;
	for (int i = 0; i < sons.size(); i++) {
		cout << "--> sub case #" << i <<endl;
		//actions_cases[i]->print();
		sons[i]->print();
	}
}

int ActionSwitch::getHeight() {
	int h = 0;
	//for (int i = 0; i < actions_cases.size(); i++) {
	for (int i = 0; i < sons.size(); i++) {
		//h += actions_cases[i]->getHeight();
		h += sons[i]->getHeight();
	}
	return HEADER_HEIGHT + h;
}

int ActionSwitch::getWidth() {
	int maxw = 0;
	//for (int i = 0; i < actions_cases.size(); i++) {
	for (int i = 0; i < sons.size(); i++) {
		//int w = actions_cases[i]->getWidth();
		int w = sons[i]->getWidth();
		if (maxw < w)
			maxw = w;
	}
	return maxw;
}

int ActionSwitch::update_sub_height(ofxTLAntescofoAction* tlAction, int& cury, int& curh)
{
	if (tlAction->mFilterActions && !in_selected_track) {
		curh = 0;
		rect.height = 0;
		return 0;
	}
	rect.x = tlAction->get_x(beatnum + delay);
	rect.y = cury;
	rect.height = HEADER_HEIGHT;
	curh = HEADER_HEIGHT;
	for (int i = 0; i < sons.size(); i++)
		//actions_cases[i]->update_sub_height(tlAction, cury, curh);
		sons[i]->update_sub_height(tlAction, cury, curh);

	// shift cases groups
	for (int i = 0; i < sons.size(); i++) {
		sons[i]->rect.x += 15;
	}
	return 0;
}


#endif


// create an graphical ActionGroup from an Antescofo object group
ActionGroup* create_ActionGroup(float beatnum, Action* g, Event* e) {
	if (!g) return NULL;
	float d = 0;
	if (g && g->delay().value() && g->delay().value()->is_value())
		d = (double)g->delay().eval();

	ActionGroup *ag = 0;
	// can be group
	Gfwd* gr = dynamic_cast<Gfwd*>(g);
	AtomicSequence* as = dynamic_cast<AtomicSequence*>(g);
	Message* m = dynamic_cast<Message*>(g);
	AssignmentAction* asa = dynamic_cast<AssignmentAction*>(g);
	Curve* c = dynamic_cast<Curve*>(g);
	Lfwd* l = dynamic_cast<Lfwd*>(g);
	KillAction* k = dynamic_cast<KillAction*>(g);
	ProcCall* p = dynamic_cast<ProcCall*>(g);
	SwitchAction* sw = dynamic_cast<SwitchAction*>(g);
	ConditionalAction* ca = dynamic_cast<ConditionalAction*>(g);

	if (gr) {
		ag = new ActionGroup(beatnum, d, gr, e);
	} else if (m) { // can be message
		ag = new ActionMessage(beatnum, d, m, e);
	} else if (asa) { // can be an assignment action
		ag = new ActionMessage(beatnum, d, asa, e);
	} else if (k) { // can be kill/abort
		ag = new ActionMessage(beatnum, d, k, e);
	} else if (c) { // can be a curve
		ag = new ActionMultiCurves(beatnum, d, c, e);
	} else if (p) { // can be a proc call
		ag = new ActionMessage(beatnum, d, p, e);
	} else if (l && l->_group) { // can be a loop
		ag = new ActionGroup(beatnum, d, l->_group, e);
		if (l->_period.value() && l->_period.value()->is_value())
			ag->period = l->_period.eval();
		ag->realtitle = ag->title = l->label();
	} else if (sw || ca) // can be a switch or an if
		ag = new ActionSwitch(beatnum, d, g, e);
	return ag;
}

