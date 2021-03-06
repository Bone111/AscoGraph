/**
 * ofxTimeline
 * openFrameworks graphical timeline addon
 *
 * Copyright (c) 2011-2012 James George
 * Development Supported by YCAM InterLab http://interlab.ycam.jp/en/
 * http://jamesgeorge.org + http://flightphase.com
 * http://github.com/obviousjim + http://github.com/flightphase
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ofxTLAudioTrack.h"
#include "ofxTimeline.h"

float oldpos = 0;

class AlignMarker {
public:
	AlignMarker(long ms_) : ms(ms_), selected(false) {}
	long ms;
	ofRectangle rect;
	bool selected;
};
vector<AlignMarker> markers;

ofxTLAudioTrack::ofxTLAudioTrack(){
	shouldRecomputePreview = false;
    soundLoaded = false;
	lastFFTPosition = 0;
	defaultFFTBins = 256;
	maxBinReceived = 0;
}

ofxTLAudioTrack::~ofxTLAudioTrack(){

}

bool ofxTLAudioTrack::loadSoundfile(string filepath){
	soundLoaded = false;
	if(player.loadSound(filepath, false)){
    	soundLoaded = true;
		soundFilePath = filepath;
		shouldRecomputePreview = true;
    }
	return soundLoaded;
}
 
string ofxTLAudioTrack::getSoundfilePath(){
	return soundFilePath;
}

bool ofxTLAudioTrack::isSoundLoaded(){
	return soundLoaded;
}

float ofxTLAudioTrack::getDuration(){
	return player.getDuration();
}

void ofxTLAudioTrack::update()
{
	//cout << "ofxTLAudioTrack:: update" <<  player.getPosition() << endl;
	float pos = player.getPosition();
	if (!zoomBounds.contains(pos) || oldpos != pos) {
		recomputePreview();
		oldpos = pos;
	}
}
void ofxTLAudioTrack::update(ofEventArgs& args){
	if(player.getPosition() < lastPercent){
		ofxTLPlaybackEventArgs args = timeline->createPlaybackEvent();
		ofNotifyEvent(events().playbackLooped, args);
	}
	lastPercent = player.getPosition();
	/*
	
    //currently only supports timelines with duration == duration of player
	if(lastPercent < timeline->getInOutRange().min){
		player.setPosition(timeline->getInOutRange().min);
	}
	else if(lastPercent > timeline->getInOutRange().max){
		if(timeline->getLoopType() == OF_LOOP_NONE){
			player.setPosition(timeline->getInOutRange().max);
			cout << "ofxTLAudioTrack::update stop()" << endl;
			stop();
		}
		else{
			player.setPosition(timeline->getInOutRange().min);
		}
	}
	*/
	
    //timeline->setCurrentTimeSeconds(player.getPosition() * player.getDuration());
}
 
void ofxTLAudioTrack::draw(){
	
	if(!soundLoaded || player.getBuffer().size() == 0){
		ofPushStyle();
		ofSetColor(timeline->getColors().disabledColor);
		ofRectangle(bounds);
		ofPopStyle();
		return;
	}
		
	if(shouldRecomputePreview || viewIsDirty){
		recomputePreview();
	}

	//cout << "ofxTLAudioTrack::draw" << endl;

    ofSetColor(255, 255, 255, 205);
    ofFill();
    ofRect(bounds);
    ofPushStyle();
    //ofSetColor(timeline->getColors().keyColor);
    ofSetColor(0, 0 , 0, 255);
    ofNoFill();
    
    for(int i = 0; i < previews.size(); i++){
        ofPushMatrix();
        ofTranslate( normalizedXtoScreenX(computedZoomBounds.min, zoomBounds)  - normalizedXtoScreenX(zoomBounds.min, zoomBounds), 0, 0);
        ofScale(computedZoomBounds.span()/zoomBounds.span(), 1, 1);
        previews[i].draw();
        ofPopMatrix();
    }
    ofPopStyle();
	
	if(getIsPlaying() || timeline->getIsPlaying()){
		ofPushStyle();
		
		//will refresh fft bins for other calls too
		vector<float>& bins = getFFTSpectrum(defaultFFTBins);
		float binWidth = bounds.width / bins.size();
		//find max
		float averagebin = 0 ;
		for(int i = 0; i < bins.size(); i++){
			maxBinReceived = MAX(maxBinReceived, bins[i]);
			averagebin += bins[i];
		}
		averagebin /= bins.size();
		
		ofFill();
		ofSetColor(timeline->getColors().disabledColor, 120);
		for(int i = 0; i < bins.size(); i++){
			float height = bounds.height * bins[i]/maxBinReceived;
			float y = bounds.y + bounds.height - height;
			ofRect(bounds.x + i*binWidth, y, binWidth, height);
		}
		
		ofPopStyle();
	}

	// playhead 
	ofPushStyle();
	ofSetColor(0, 0, 0, 255);
	float x = normalizedXtoScreenX( oldpos, zoomBounds);
	ofLine(x, bounds.y, x, bounds.y + bounds.height);
	ofSetLineWidth(3);
	ofPopStyle();

	/*
	float pos = player.getPosition();
	if (pos) {
		ofxTLZoomer2D *zoom = (ofxTLZoomer2D*)timeline->getZoomer();
		ofRange z = zoom->getViewRange();
		ofRange oldz = z;
		float c = z.center(); 
		float d = pos - c;

		z.min = ofClamp(z.min + d, 0, 1); z.max = ofClamp(z.max + d, 0, 1);
		if (z.min == .0 && z.span() < oldz.span())
			z.max = oldz.max - oldz.min;
		if (z.max == 1. && z.span() < oldz.span())
			z.min = z.max - oldz.max + oldz.min;
	}*/




	// draw markers:
	for (vector<AlignMarker>::iterator m = markers.begin(); m != markers.end(); m++) {
		float xn = screenXtoNormalizedX(millisToScreenX(m->ms));
		if (zoomBounds.contains(xn)) {
			float x = timeline->normalizedXtoScreenX(xn, zoomBounds);
			if (m->selected)
				ofSetColor(255, 0, 0, 255);
			else 
				ofSetColor(0, 0, 0, 255);
			ofLine(x, bounds.y, x, bounds.y+bounds.height);
			m->rect = ofRectangle(x - 5, bounds.y + bounds.height - 12, 10, 10);
			ofSetColor(10, 0, 200, 100);
			ofFill();
			ofRect(m->rect);

			//cout << "m:" << m->ms << endl;
			ofSetColor(0, 0, 0, 255);
			timeline->getFont().drawString(ofToString(m->ms), x+1, bounds.y + 30);
		}
	}
	
}

void ofxTLAudioTrack::setMarkers(vector<float>& map_index, vector<float>& map_markers) {
	for (vector<float>:: iterator i = map_markers.begin(); i != map_markers.end(); i++) {
		markers.push_back(*i * 1000);
	}
}

void ofxTLAudioTrack::recomputePreview(){
	
	previews.clear();
	
	//cout << "recomputing view with zoom bounds of " << zoomBounds << endl;
	
	float normalizationRatio = timeline->getDurationInSeconds() / player.getDuration(); //need to figure this out for framebased...but for now we are doing time based
	float trackHeight = bounds.height/(1+player.getNumChannels());
	int numSamples = player.getBuffer().size() / player.getNumChannels();
	int pixelsPerSample = numSamples / bounds.width;
	for(int c = 0; c < player.getNumChannels(); c++){
		ofPolyline preview;
		int lastFrameIndex = 0;
		for(float i = bounds.x; i < bounds.x+bounds.width; i++){
			float pointInTrack = screenXtoNormalizedX( i, zoomBounds ) * normalizationRatio; //will scale the screenX into wave's 0-1.0
			float trackCenter = bounds.y + trackHeight * (c+1);
			if(pointInTrack <= 1.0){
				//draw sample at pointInTrack * waveDuration;
				int frameIndex = pointInTrack * numSamples;					
				float losample = 0;
				float hisample = 0;
				for(int f = lastFrameIndex; f < frameIndex; f++){
					int sampleIndex = f * player.getNumChannels() + c;
					float subpixelSample = player.getBuffer()[sampleIndex]/32565.0;
					if(subpixelSample < losample) {
						losample = subpixelSample;
					}
					if(subpixelSample > hisample) {
						hisample = subpixelSample;
					}
				}
				if(losample == 0 && hisample == 0){
					preview.addVertex(i, trackCenter);
				}
				else {
					if(losample != 0){
						preview.addVertex(i, trackCenter - losample * trackHeight);
					}
					if(hisample != 0){
						//ofVertex(i, trackCenter - hisample * trackHeight);
						preview.addVertex(i, trackCenter - hisample * trackHeight);
					}
				}
				lastFrameIndex = frameIndex;
			}
			else{
				preview.addVertex(i,trackCenter);
			}
		}
		preview.simplify();
		previews.push_back(preview);
	}
	computedZoomBounds = zoomBounds;
	shouldRecomputePreview = false;
}

int ofxTLAudioTrack::getDefaultBinCount(){
//	cout << defaultFFTBins << endl;
	return defaultFFTBins;
}

bool ofxTLAudioTrack::mousePressed(ofMouseEventArgs& args, long millis){
	if (!bounds.inside(args.x, args.y)) return false;
	
	for (vector<AlignMarker>::iterator m = markers.begin(); m != markers.end(); m++) {
		if (m->rect.inside(args.x, args.y)) {
			cout << "ms selected:" << m->ms << endl;
			m->selected = true;
			return true;
		}
	}
	// change playhead position
	timeline->setPercentComplete(screenXtoNormalizedX(args.x, zoomBounds));

	return false;
}

void ofxTLAudioTrack::mouseMoved(ofMouseEventArgs& args, long millis){
	if (!bounds.inside(args.x, args.y)) return;

	for (vector<AlignMarker>::iterator m = markers.begin(); m != markers.end(); m++) {
		if (m->selected) {
			cout << "ms selected changin from " << m->ms << endl;
			m->ms = screenXToMillis(normalizedXtoScreenX(screenXtoNormalizedX(args.x, zoomBounds)));
			cout << " to: " << m->ms << endl;
		}
	}
}

void ofxTLAudioTrack::mouseDragged(ofMouseEventArgs& args, long millis){
}

void ofxTLAudioTrack::mouseReleased(ofMouseEventArgs& args, long millis){
	if (!bounds.inside(args.x, args.y)) return;

	for (vector<AlignMarker>::iterator m = markers.begin(); m != markers.end(); m++) {
		if (m->selected) {
			cout << "ms selected changin from " << m->ms << endl;
			m->ms = screenXToMillis(normalizedXtoScreenX(screenXtoNormalizedX(args.x, zoomBounds)));
			cout << " to: " << m->ms << endl;
			m->selected = false;
		}
	}
}

void ofxTLAudioTrack::keyPressed(ofKeyEventArgs& args){
	if (args.key == OF_KEY_RETURN) {
		long ms = timeline->getCurrentTimeMillis();
		cout << "Marker taken:"<< ms << "ms" << endl;
		markers.push_back(AlignMarker(ms));
	} else if (args.key == OF_KEY_DEL || args.key == OF_KEY_BACKSPACE) {
		markers.clear();
	}
}

void ofxTLAudioTrack::zoomStarted(ofxTLZoomEventArgs& args){
	//ofxTLTrack::zoomStarted(args);
//	shouldRecomputePreview = true;
}

void ofxTLAudioTrack::zoomDragged(ofxTLZoomEventArgs& args){
	//ofxTLTrack::zoomDragged(args);
	//shouldRecomputePreview = true;
}

void ofxTLAudioTrack::zoomEnded(ofxTLZoomEventArgs& args){
	ofxTLTrack::zoomEnded(args);
	shouldRecomputePreview = true;
}

void ofxTLAudioTrack::boundsChanged(ofEventArgs& args){
	cout << "ofxTLAudioTrack::boundsChanged" << endl;
	shouldRecomputePreview = true;
}

void ofxTLAudioTrack::play(){
	cout << "ofxTLAudioTrack:: play " << endl;
	if(!player.getIsPlaying()){
		
//		lastPercent = MIN(timeline->getPercentComplete() * timeline->getDurationInSeconds() / player.getDuration(), 1.0);
		//player.setLoop(timeline->getLoopType() == OF_LOOP_NORMAL);
		player.setLoop(timeline->getLoopType() == OF_LOOP_NONE);
		cout << "ofxTLAudioTrack:: calling play on track " << endl;
		player.setPosition(0);//XXX timeline->getPercentComplete());
		player.play();
		ofAddListener(ofEvents().update, this, &ofxTLAudioTrack::update);
		/* XXX
		ofxTLPlaybackEventArgs args = timeline->createPlaybackEvent();
		ofNotifyEvent(events().playbackStarted, args);		
		*/
	} else {
		//cout << "ofxTLAudioTrack:: calling NOT play on track " << endl;
		player.stop();
		play();
	}
}

void ofxTLAudioTrack::stop(){
	if(player.getIsPlaying()){

//		cout << "calling stop on track " << endl;
		player.setPaused(true);
		ofRemoveListener(ofEvents().update, this, &ofxTLAudioTrack::update);
		
		ofxTLPlaybackEventArgs args = timeline->createPlaybackEvent();
		ofNotifyEvent(events().playbackEnded, args);
	}
}

bool ofxTLAudioTrack::togglePlay(){
	cout << "ofxTLAudioTrack::togglePlay" << endl;
	if(getIsPlaying()){
		stop();
	}
	else {
		play();
	}
	return getIsPlaying();
}

bool ofxTLAudioTrack::getIsPlaying(){
    return player.getIsPlaying();
}

void ofxTLAudioTrack::setSpeed(float speed){
    player.setSpeed(speed);
}

float ofxTLAudioTrack::getSpeed(){
    return player.getSpeed();
}

void ofxTLAudioTrack::setVolume(float volume){
    player.setVolume(volume);
}

void ofxTLAudioTrack::setPan(float pan){
    player.setPan(pan);
}

vector<float>& ofxTLAudioTrack::getFFTSpectrum(int numBins){
	float fftPosition = player.getPosition();
	if(isSoundLoaded() && lastFFTPosition != fftPosition){
		if(defaultFFTBins != numBins){
			maxBinReceived = 0;
			defaultFFTBins = numBins;
		}
		lastFFTPosition = fftPosition;
		fftBins = player.getSpectrum(defaultFFTBins);
	}
	return fftBins;
}

string ofxTLAudioTrack::getTrackType(){
    return "Audio";    
}

   
