//
//  iOSAscoGraphMenu.cpp
//  AscoGraph
//
//  Created by Thomas Coffy on 06/04/14.
//  Copyright (c) 2014 IRCAM. All rights reserved.
//

#include "iOSAscoGraphMenu.h"
#include "iOSAscoGraph.h"

@implementation iOSAscoGraphMenu

iOSAscoGraph *myApp;

-(void)viewDidLoad {
	myApp = (iOSAscoGraph*)ofGetAppPtr();
    
}

-(IBAction)autoScrollSwitchHandler:(id)sender {
	UISwitch *switchObj = sender;
	myApp->setAutoscroll([switchObj isOn]);
}

-(IBAction)fastForwardOnOffSwitchHandler:(id)sender {
	UISwitch *switchObj = sender;
	myApp->setFastForwardOnOff([switchObj isOn]);
}

-(IBAction)setSuiviOffNextEventOnOffSwitchHandler:(id)sender {
	UISwitch *switchObj = sender;
	myApp->setSuiviOffNextEvent([switchObj isOn]);
}

-(IBAction)setPrevNextLabelModeOnOffSwitchHandler:(id)sender {
	UISwitch *switchObj = sender;
	myApp->setPrevNextLabelModeOnOff([switchObj isOn]);
}

-(IBAction)setSuiviOnOffSwitchHandler:(id)sender {
	UISwitch *switchObj = sender;
	myApp->setSuiviOnOff([switchObj isOn]);
}
@end