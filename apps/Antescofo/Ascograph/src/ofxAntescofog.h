#pragma once

#include "ofMain.h"
#ifdef TARGET_OSX
# include "ofxNSWindowApp.h"
# include "ofxCocoaDelegate.h"
#endif
#include "ofxTimeline.h"
#include "ofxTLZoomer2D.h"
#include "ofxTLAccompAudioTrack.h"
#include "ofxTLBeatTicker.h"
#include "ofxTLAntescofoNote.h"
#include "ofxTLAntescofoAction.h"
#ifdef TARGET_OSX
# include "ofxTLAntescofoSim.h"
#endif
#include "ofxColorPicker.h"
#include "ofxUI.h"
#include "ofxOsc.h"
#include "ofxConsole.h"
#include "ofxTLBeatJump.h"


#define INT_CONSTANT_BUTTON_RELOAD      0
#define INT_CONSTANT_BUTTON_LOAD        1
#define INT_CONSTANT_BUTTON_SAVE        2
#define INT_CONSTANT_BUTTON_COLORSETUP  3
#define INT_CONSTANT_BUTTON_OSCSETUP    4
#define INT_CONSTANT_BUTTON_TOGGLEVIEW  5
#define INT_CONSTANT_BUTTON_TOGGLEEDIT  6    
#define INT_CONSTANT_BUTTON_QUIT        7
#define INT_CONSTANT_BUTTON_SELECTALL   8
#define INT_CONSTANT_BUTTON_AUTOSCROLL  9
#define INT_CONSTANT_BUTTON_SNAP        10
#define INT_CONSTANT_BUTTON_PLAY        11
#define INT_CONSTANT_BUTTON_START       12
#define INT_CONSTANT_BUTTON_STOP        13
#define INT_CONSTANT_BUTTON_SAVE_AS     14
#define	INT_CONSTANT_BUTTON_LINEWRAP    15
#define	INT_CONSTANT_BUTTON_FIND        16

#define INT_CONSTANT_BUTTON_PLAYSTRING		24
#define INT_CONSTANT_BUTTON_NEW			25
#define INT_CONSTANT_BUTTON_SIMULATE		26
#define INT_CONSTANT_BUTTON_EDIT		27
#define INT_CONSTANT_BUTTON_PREVEVENT		28
#define INT_CONSTANT_BUTTON_NEXTEVENT		29
#define INT_CONSTANT_BUTTON_ZOOM_IN		30
#define INT_CONSTANT_BUTTON_ZOOM_OUT		31
#define INT_CONSTANT_BUTTON_OPEN_ALL_CURVES	32
#define INT_CONSTANT_BUTTON_OPEN_ALL_GROUPS	33
#define INT_CONSTANT_BUTTON_CLOSE_ALL_CURVES	34
#define INT_CONSTANT_BUTTON_CLOSE_ALL_GROUPS	35
#define INT_CONSTANT_BUTTON_SHOWHIDE_ACTION  	36
#define INT_CONSTANT_BUTTON_HIDE		37
#define INT_CONSTANT_BUTTON_UNDO		38
#define INT_CONSTANT_BUTTON_REDO		39
#define INT_CONSTANT_BUTTON_GET_PATCH_RECEIVERS 40
#define INT_CONSTANT_BUTTON_AUTOCOMPLETE	41
#define INT_CONSTANT_BUTTON_TOGGLE_FULL_EDITOR	42
#define INT_CONSTANT_BUTTON_LOCK		43
#define INT_CONSTANT_BUTTON_ACTIONVIEWDEEPMODE	44
#define INT_CONSTANT_BUTTON_CANCEL_SAVE		45
#define INT_CONSTANT_BUTTON_DONT_SAVE		46
#define INT_CONSTANT_BUTTON_OK_SAVE		47
#define INT_CONSTANT_BUTTON_OPENRECENT		200
#define INT_CONSTANT_BUTTON_CUES_INDEX  	300
#define INT_CONSTANT_BUTTON_CREATE_ACTIONGROUP		600
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_GROUP		601
#define INT_CONSTANT_BUTTON_CREATE_ACTION_LOOP		602
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_WHENEVER	603
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_CURVE		604
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_CURVES	605
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_MACRO		606
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_PROCESS	607
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_FUNCTION	608
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_TABINIT	609
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_MAPINIT	610
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_PATTERN	611
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_OSCSEND	612
#define	INT_CONSTANT_BUTTON_CREATE_ACTION_OSCRECV	613
#define INT_CONSTANT_BUTTON_CREATE_EVENT		650
#define INT_CONSTANT_BUTTON_CREATE_EVENT_NOTE		651
#define INT_CONSTANT_BUTTON_CREATE_EVENT_CHORD		652
#define INT_CONSTANT_BUTTON_CREATE_EVENT_TRILL		653
#define INT_CONSTANT_BUTTON_CREATE_EVENT_MULTI		654
#define INT_CONSTANT_BUTTON_CREATE_EVENT_SILENCE	655

#define TEXT_CONSTANT_TITLE                     "Ascograph: score editor"
#define TEXT_CONSTANT_BUTTON_LOAD               "Open score"
#define TEXT_CONSTANT_BUTTON_RELOAD             "Re-open score"
#define TEXT_CONSTANT_BUTTON_OPENRECENT         "Open Recent"
#define TEXT_CONSTANT_BUTTON_SAVE               "Save score"
#define TEXT_CONSTANT_BUTTON_SAVE_AS            "Save score as"
#define TEXT_CONSTANT_BUTTON_COLOR_SETUP        "Color preferences"
#define TEXT_CONSTANT_BUTTON_OSC_SETUP          "OSC Setup"
#define TEXT_CONSTANT_BUTTON_COLOR_SETUP_EXIT   "Exit color preferences"
#define TEXT_CONSTANT_BUTTON_OSC_SETUP_EXIT     "Exit OSC Setup"
#define TEXT_CONSTANT_BUTTON_TOGGLE_VIEW        "Toggle view"
#define TEXT_CONSTANT_BUTTON_TOGGLE_EDITOR      "Toggle Editor"
#define TEXT_CONSTANT_BUTTON_BEAT               "Position in score (in beats): "
#define TEXT_CONSTANT_BUTTON_PITCH              "Detected Pitch: "
#define TEXT_CONSTANT_BUTTON_BPM              	"Detected BPM: "
#define TEXT_CONSTANT_BUTTON_SPEED              "Accompaniment speed : "
#define TEXT_CONSTANT_BUTTON_SNAP               "Snap to grid"
#define TEXT_CONSTANT_BUTTON_AUTOSCROLL         "Auto Scroll"
#define TEXT_CONSTANT_BUTTON_LINEWRAP		"Toggle line wrapping"
#define TEXT_CONSTANT_BUTTON_PLAY               "Play"
#define TEXT_CONSTANT_BUTTON_SIMULATE           "Simulate"
#define TEXT_CONSTANT_BUTTON_EDIT		"Edit"
#define TEXT_CONSTANT_BUTTON_PLAYSTRING         "Play string"
#define TEXT_CONSTANT_BUTTON_START              "Start"
#define TEXT_CONSTANT_BUTTON_STOP               "Stop"
#define TEXT_CONSTANT_BUTTON_NEXT_EVENT         "next event"
#define TEXT_CONSTANT_BUTTON_PREV_EVENT         "prev event"
#define TEXT_CONSTANT_BUTTON_SAVE_COLOR         "Save color"
#define TEXT_CONSTANT_BUTTON_CUEPOINTS		"cue points"
#define TEXT_CONSTANT_PARSE_ERROR               " /!\\ Antescofo Score parsing error /!\\  "
#define TEXT_CONSTANT_SIMULATION_ERROR          " /!\\ Antescofo performance simulation error /!\\  "
#define TEXT_CONSTANT_BUTTON_CANCEL             "Cancel"
#define TEXT_CONSTANT_BUTTON_BACK               "Back"
#define TEXT_CONSTANT_BUTTON_FIND               "Find"
#define TEXT_CONSTANT_BUTTON_REPLACE       	"Replace"
#define TEXT_CONSTANT_BUTTON_TEXT               "Text"
#define TEXT_CONSTANT_BUTTON_REPLACE_TEXT	"Replaced Text"
#define TEXT_CONSTANT_BUTTON_REPLACE_NB		"Number of replacement:"
#define TEXT_CONSTANT_TEMP_FILENAME             "/tmp/tmpfile-ascograph.txt"
#define TEXT_CONSTANT_TITLE_LOAD_SCORE          "Select a score : MusicXML2 or Antescofo format"
#define TEXT_CONSTANT_TITLE_SAVE_AS_SCORE       "Save score in Antescofo format"
#define TEXT_CONSTANT_TEMP_ACTION_FILENAME      "/tmp/ascograph_tmp.asco.txt"
#define TEXT_CONSTANT_LOCAL_SETTINGS		"Ascograph_settings.xml"


class ofxTLBeatTicker;
class rational;

#ifdef TARGET_OSX
@class ofxCodeEditor;
class ofxCocoaWindow;

@interface ofxCocoaDelegate (ofxAntescofogAdditions)
- (void)menu_item_hit:(id)sender;
- (void) receiveNotification:(NSNotification *) notification;
@end

// simulation structures
class curveval {
public:
	curveval(double now_, double rnow_, double val_)
		: now(now_), rnow(now_), val(val_)
	{}
	double now, rnow, val;
};
class curve_trace {
public:
	curve_trace(string name_, string fathername_, string varname_, double now_, double rnow_, double val_)
		: name(name_), fathername(fathername_), now(now_), rnow(rnow_), varname(varname_)
	{
		values.push_back(new curveval(now, rnow, val_));
		min = val_; 
		max = val_;
	}
	string name;
	string varname;
	string fathername;
	vector<curveval*> values;
	double now, rnow; // time of first element
	ofRectangle rect;
	double min, max;
	ofPolyline line;
};

class action_trace {
  public:
	action_trace(string name_, string fathername_, double now_, double rnow_, string s_)
		: name(name_), fathername(fathername_), now(now_), rnow(rnow_), s(s_), nbcurves(0)
	{}
	ofRectangle rect;
	string name;
	string fathername;
	double now, rnow;
	string s;
	int nbcurves;
};
#endif


class AntescofoTimeline : public ofxTimeline
{
	public: 
	AntescofoTimeline() : ofxTimeline() {}
	virtual ~AntescofoTimeline(){}

	void setZoomer(ofxTLZoomer *z);

	void keypressed(ofKeyEventArgs& args) {}
};

#ifdef TARGET_OSX
@interface NSFindWindow : NSWindow {
	ofxAntescofog* fog;
}
@property (nonatomic, assign) ofxAntescofog* fog;
-(void)findNextText_pressed;
-(void)findPrevText_pressed;
-(void)keyDown:(NSEvent *)theEvent;
@end
@interface NSFindTextField : NSTextField {
}
-(void)keyDown:(NSEvent *)theEvent;
@end
#endif

#ifdef TARGET_OSX
class ofxAntescofog : public ofxNSWindowApp
#else
class ofxAntescofog : public ofBaseApp
#endif
{
	public:
#ifdef TARGET_OSX
		ofxAntescofog(int argc, char* argv[], ofxCocoaWindow* window);
#else
		ofxAntescofog(int argc, char* argv[]);
#endif

		AntescofoTimeline timeline, timelineSim;

		void setup();
		void setupTimeline();
#ifdef TARGET_OSX
		void setupTimelineSim();
#endif
		void setupUI();
		void setupOSC();
		void update();
		void draw();
		void load();
		void save();
		void load_appsupport(string filename);
		void save_appsupport(string filename);
		void menu_item_hit(int n);

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved( int x, int y);
		void mouseDragged( int x, int y, int button);
		void mousePressed( int x, int y, int button);
		void mouseReleased( int x, int y, int button);
		void windowResized(ofResizeEventArgs& resizeEventArgs);//int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void addTrackCurve(string trackname);

		string mScore_filename;
		void parse_AntescofoScore(const string filename);
		void setGotoPos(float pos);
		void shouldRedraw();
		void display_error();
		void showJumpTrack();
		void setMouseCursorGoto(bool bState);
#ifdef TARGET_OSX
		void setEditorMode(bool state, float beatn, bool fullTextEditor=false);
		ofxCodeEditor* editor;
		void editorShowLine(int linea, int lineb, int cola, int colb);
		void editorDoubleclicked(int line);
		void editorTextDidChange();
		void replaceEditorScore(int linebegin, int lineend, int cola, int colb, string actstr);
		void createCodeTemplate(int which);
		void findNextText_pressed();
		void findPrevText_pressed();
		void replaceText_pressed();
		void replaceAllText_pressed();
		int askToSaveScore();
		int draw_asksave_window();
		void get_identifiers_for_completion();
		//bool compare_stringptr_content(const string* lhs, const string* rhs);
		NSFindWindow* mFindWindow;
#endif
		ofxTLAntescofoNote* ofxAntescofoNote;
		ofxTLBeatTicker *ofxAntescofoBeatTicker;
		ofxTLZoomer2D *ofxAntescofoZoom;
		ofxTLAccompAudioTrack* audioTrack;
		ofxTLBeatJump* ofxJumpTrack;

#ifdef TARGET_OSX
		void set_mouse_cursor(int x, int y);
		// timeline tracks for simulation
		ofxTLAntescofoNote* ofxAntescofoNoteSim;
		ofxTLBeatTicker *ofxAntescofoBeatTickerSim;
		ofxTLZoomer2D *ofxAntescofoZoomSim;
		ofxTLAntescofoSim* ofxAntescofoSim;
#endif

	protected:
		// display properties
		int score_x, score_y, score_w, score_h, mUIbottom_y;
		float score_bg_color, score_fg_color;
		int score_line_x, score_line_y, score_line_w, score_line_h;
		int score_line_space;

		int bpm;
		bool bSnapToGrid, bAutoScroll, bSetupDone, bLineWrapMode, bScoreFromCommandLine, bShowActions;
		string tmpfilename; // file to store converted MusicXML file to Antescofo score

		vector<float> accomp_map_index, accomp_map_markers;
		vector<string> patch_receivers;

#ifdef TARGET_OSX
		ofxCocoaWindow*	cocoaWindow;

		// UI
		id mCuesMenuItem, mCuesMenu, mShowhideActiontrackMenuItem, mSnapMenuItem, mAutoscrollMenuItem, mLineWrapModeMenuItem, mLockMenuItem, mFileMenu, mActionsViewDeepLevelModeMenuItem;
		ofxUICanvas *guiTop, *guiBottom, *guiSetup_OSC;
		ofxUICanvas *guiSetup_Colors, *guiFind;
		ofxUIScrollableCanvas *guiError;
		ofxUISlider *mSliderBPM;
		ofxUILabel  *mLabelBeat, *mLabelBPM, *mLabelPitch, *mLabelAccompSpeed, *mFindReplaceOccur;
		ofxUILabelButton *mSaveColorButton;
		ofxUILabelToggle *mEditButton;
#endif
		void exit();
		void guiEvent(ofxUIEventArgs &e);
		ofxUIDropDownList *mUImenu;
		ofFbo	drawCache;
		bool bShouldRedraw, bLockAscoGraph;
		ofImage mLogoInria, mLogoIrcam;
		vector<string> cuepoints;
		string mPlayLabel;
		ofxUIDropDownList* mCuepointsDdl;
		float* mBPMbuffer;
		void push_tempo_value();
		map<int, string> mCuesIndexToString;
		int mCuesMaxIndex;
		void cues_clear_menu();
		void cues_add_menu(string s);
#ifdef TARGET_OSX
		void newWindow();
		ofxCocoaWindow* subWindow;
#endif
		float mGotoPos;
		string getApplicationSupportSettingFile();
		vector<string> mRecentFiles;
		void populateOpenRecentMenu();
		bool bMouseCursorSet;

		// OpenSoundControl communication with MAX/MSP or PureData
		ofxOscReceiver  mOSCreceiver;
		ofxOscSender    mOSCsender, mOSCsender_www;
		bool            mHasReadMessages;
		string          mOsc_host, mOsc_port, mOsc_port_MAX;
		float           mOsc_beat, mOsc_tempo, mOsc_pitch, mOsc_rnow, mOsc_accomp_speed;
		float           fAntescofoTimeSeconds;

		// color chooser
		ofxColorPicker	mColorPicker;
		map<string, ofColor*> colorString2var;
		bool bShowColorSetup, bColorSetupInitDone;
		void draw_ColorSetup();
		void draw_ColorPicker(string name);
		void save_ColorPicker(string name);
		void draw_ColorAsset(string name, ofColor *color);
		string mColorChanged;

		// OSC setup
		void draw_OSCSetup();
		bool bShowOSCSetup, bOSCSetupInitDone;
		map<string, string *> oscString2var;
		unsigned long long mLastOSCmsgDate;
		ofxUITextInput* mTextOscPort, *mTextOscHost, *mTextOscPortRemote;

		// error handling
		void draw_error();
		bool bShowError, bErrorInitDone;

		// open/save file
		int loadScore(string filename, bool reloadEditor, bool sendOsc = true);
		void saveScore(bool stopSimu = true);
		void saveAsScore();
		void newScore();
		bool edited();

#ifdef TARGET_OSX
		// MIDI file conversion
		void setup_Midi(string& midifile, bool do_actions);
		string convertMidiFileToActions(string& midifile);
		string convertMidiFileToNotes(string& midifile);

		// simulation
		bool bIsSimulating;
		void simulate();
		void stop_simulate_and_goedit();
		void draw_simulate();

		bool bEditorShow, bFullTextEditorShow;
#endif

		struct timeval last_draw_time;

#ifdef TARGET_OSX
		// find text
		NSFindTextField* mFindTextField, *mReplaceTextField;
		bool bFindTextInitDone, bShowFind;
		void create_Find_and_Replace_window();
		void draw_FindText();
		NSButton* mBtnFindMatchCase, *mBtnFindWrapMode;
#endif
};

