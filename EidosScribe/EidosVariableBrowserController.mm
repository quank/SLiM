//
//  EidosVariableBrowserController.m
//  EidosScribe
//
//  Created by Ben Haller on 6/13/15.
//  Copyright (c) 2015 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of Eidos.
//
//	Eidos is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	Eidos is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with Eidos.  If not, see <http://www.gnu.org/licenses/>.


#import "EidosVariableBrowserController.h"
#import "EidosValueWrapper.h"

#include <sstream>


@implementation EidosVariableBrowserController

- (instancetype)init
{
	if (self = [super init])
	{
		browserWrappers = [NSMutableArray new];
	}
	
	return self;
}

- (void)dealloc
{
	[browserWrappers release];
	browserWrappers = nil;
	
	[super dealloc];
}

- (void)reloadBrowser
{
	// Reload symbols in outline view
	[browserWrappers removeAllObjects];
	[_browserOutline reloadData];
}

- (void)setDelegate:(NSObject<EidosVariableBrowserDelegate> *)newDelegate
{
	_delegate = newDelegate;
	
	[self reloadBrowser];
}

- (IBAction)toggleBrowserVisibility:(id)sender
{
	if ([_browserWindow isVisible])
		[_browserWindow performClose:nil];
	else
		[_browserWindow makeKeyAndOrderFront:nil];
}


//
//	NSWindow delegate methods
//
#pragma mark NSWindow delegate

- (void)windowWillClose:(NSNotification *)notification
{
	NSWindow *closingWindow = [notification object];
	
	if (closingWindow == _browserWindow)
	{
		[_browserWindowButton setState:NSOffState];
	}
}


//
//	NSOutlineView datasource / delegate methods
//
#pragma mark NSOutlineView delegate

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
	EidosSymbolTable *symbols = [_delegate EidosSymbolTable];
	
	if (symbols)
	{
		if (item == nil)
		{
			std::vector<std::string> readOnlySymbols = symbols->ReadOnlySymbols();
			std::vector<std::string> readWriteSymbols = symbols->ReadWriteSymbols();
			
			return (readOnlySymbols.size() + readWriteSymbols.size());
		}
		else
		{
			EidosValueWrapper *wrapper = (EidosValueWrapper *)item;
			EidosValue_Object *value = (EidosValue_Object *)(wrapper->wrappedValue);
			std::vector<std::string> readOnlySymbols = value->ReadOnlyMembersOfElements();
			std::vector<std::string> readWriteSymbols = value->ReadWriteMembersOfElements();
			
			return (readOnlySymbols.size() + readWriteSymbols.size());
		}
	}
	
	return 0;
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item
{
	EidosSymbolTable *symbols = [_delegate EidosSymbolTable];
	
	if (symbols)
	{
		if (item == nil)
		{
			std::vector<std::string> readOnlySymbols = symbols->ReadOnlySymbols();
			std::vector<std::string> readWriteSymbols = symbols->ReadWriteSymbols();
			
			if (index < readOnlySymbols.size())
			{
				const std::string &symbolName = readOnlySymbols[index];
				EidosValue *symbolValue = symbols->GetValueForSymbol(symbolName);
				NSString *symbolObjcName = [NSString stringWithUTF8String:symbolName.c_str()];
				EidosValueWrapper *wrapper = [EidosValueWrapper wrapperForName:symbolObjcName value:symbolValue];
				
				[browserWrappers addObject:wrapper];
				
				return wrapper;
			}
			else
			{
				const std::string &symbolName = readWriteSymbols[index - readOnlySymbols.size()];
				EidosValue *symbolValue = symbols->GetValueForSymbol(symbolName);
				NSString *symbolObjcName = [NSString stringWithUTF8String:symbolName.c_str()];
				EidosValueWrapper *wrapper = [EidosValueWrapper wrapperForName:symbolObjcName value:symbolValue];
				
				[browserWrappers addObject:wrapper];
				
				return wrapper;
			}
		}
		else
		{
			EidosValueWrapper *wrapper = (EidosValueWrapper *)item;
			EidosValue_Object *value = (EidosValue_Object *)(wrapper->wrappedValue);
			std::vector<std::string> readOnlySymbols = value->ReadOnlyMembersOfElements();
			std::vector<std::string> readWriteSymbols = value->ReadWriteMembersOfElements();
			
			if (index < readOnlySymbols.size())
			{
				const std::string &symbolName = readOnlySymbols[index];
				EidosGlobalStringID symbolID = EidosGlobalStringIDForString(symbolName);
				EidosValue *symbolValue = value->GetValueForMemberOfElements(symbolID);
				NSString *symbolObjcName = [NSString stringWithUTF8String:symbolName.c_str()];
				EidosValueWrapper *childWrapper = [EidosValueWrapper wrapperForName:symbolObjcName value:symbolValue];
				
				[browserWrappers addObject:childWrapper];
				
				return childWrapper;
			}
			else
			{
				const std::string &symbolName = readWriteSymbols[index - readOnlySymbols.size()];
				EidosGlobalStringID symbolID = EidosGlobalStringIDForString(symbolName);
				EidosValue *symbolValue = value->GetValueForMemberOfElements(symbolID);
				NSString *symbolObjcName = [NSString stringWithUTF8String:symbolName.c_str()];
				EidosValueWrapper *childWrapper = [EidosValueWrapper wrapperForName:symbolObjcName value:symbolValue];
				
				[browserWrappers addObject:childWrapper];
				
				return childWrapper;
			}
		}
	}
	
	return nil;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
	if ([item isKindOfClass:[EidosValueWrapper class]])
	{
		EidosValueWrapper *wrapper = (EidosValueWrapper *)item;
		EidosValue *value = wrapper->wrappedValue;
		
		if (value->Type() == EidosValueType::kValueObject)
		{
			return YES;
		}
	}
	
	return NO;
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
	if ([item isKindOfClass:[EidosValueWrapper class]])
	{
		EidosValueWrapper *wrapper = (EidosValueWrapper *)item;
		
		if (tableColumn == _symbolColumn)
		{
			return wrapper->wrappedName;
		}
		else if (tableColumn == _typeColumn)
		{
			EidosValueType type = wrapper->wrappedValue->Type();
			std::string type_string = StringForEidosValueType(type);
			const char *type_cstr = type_string.c_str();
			NSString *typeString = [NSString stringWithUTF8String:type_cstr];
			
			if (type == EidosValueType::kValueObject)
			{
				EidosValue_Object *object_value = (EidosValue_Object *)wrapper->wrappedValue;
				const std::string *element_string = object_value->ElementType();
				const char *element_cstr = element_string->c_str();
				NSString *elementString = [NSString stringWithUTF8String:element_cstr];
				
				typeString = [NSString stringWithFormat:@"%@<%@>", typeString, elementString];
			}
			
			return typeString;
		}
		else if (tableColumn == _sizeColumn)
		{
			int size = wrapper->wrappedValue->Count();
			
			return [NSString stringWithFormat:@"%d", size];
		}
		else if (tableColumn == _valueColumn)
		{
			EidosValue *value = wrapper->wrappedValue;
			int value_count = value->Count();
			std::ostringstream outstream;
			
			// print values as a comma-separated list with strings quoted; halfway between print() and cat()
			for (int value_index = 0; value_index < value_count; ++value_index)
			{
				EidosValue *element_value = value->GetValueAtIndex(value_index);
				
				if (value_index > 0)
					outstream << ", ";
				
				outstream << *element_value;
				
				if (element_value->IsTemporary()) delete element_value;
			}
			
			NSString *outString = [NSString stringWithUTF8String:outstream.str().c_str()];
			
			return outString;
		}
	}
	
	return nil;
}

@end

































































