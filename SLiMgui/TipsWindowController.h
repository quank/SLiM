//
//  TipsWindowController.h
//  SLiM
//
//  Created by Ben Haller on 12/2/16.
//  Copyright (c) 2015-2016 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/slim/
//

//	This file is part of SLiM.
//
//	SLiM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	SLiM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with SLiM.  If not, see <http://www.gnu.org/licenses/>.


#import <Cocoa/Cocoa.h>


extern NSString *SLiMDefaultsShowTipsPanelKey;
extern NSString *SLiMDefaultsTipsIndexKey;


@interface TipsWindowController : NSObject <NSWindowDelegate>
{
	NSArray *tipsFilenames;
	NSInteger lastTipShown;
}

@property (nonatomic, retain) IBOutlet NSPanel *tipsWindow;
@property (nonatomic, retain) IBOutlet NSTextView *tipsTextView;
@property (nonatomic, retain) IBOutlet NSTextField *tipsNumberTextField;
@property (nonatomic, retain) IBOutlet NSButton *suppressPanelCheckbox;
@property (nonatomic, retain) IBOutlet NSButton *nextButton;

+ (void)showTipsWindowOnLaunch;


//
//	Actions
//

- (IBAction)rewindButtonClicked:(id)sender;
- (IBAction)nextButtonClicked:(id)sender;
- (IBAction)doneButtonClicked:(id)sender;

@end























