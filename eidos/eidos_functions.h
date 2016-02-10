//
//  eidos_functions.h
//  Eidos
//
//  Created by Ben Haller on 4/6/15.
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

/*
 
 This file contains most of the code for processing function calls in the Eidos interpreter.
 
 */

#ifndef __Eidos__eidos_functions__
#define __Eidos__eidos_functions__

#include "eidos_interpreter.h"
#include "eidos_value.h"

#include <iostream>


// Utility functions usable by everybody
EidosValue_SP ConcatenateEidosValues(const EidosValue_SP *const p_arguments, int p_argument_count, bool p_allow_null);


// A numeric identifier for a function once its name has been looked up; just for efficiency, to allow switch()
enum class EidosFunctionIdentifier {
	kNoFunction = 0,
	kDelegatedFunction,		// implemented through a delegate, typically defined by the Context
	
	// math functions
	absFunction,
	acosFunction,
	asinFunction,
	atanFunction,
	atan2Function,
	ceilFunction,
	cosFunction,
	cumProductFunction,
	cumSumFunction,
	expFunction,
	floorFunction,
	integerDivFunction,
	integerModFunction,
	isFiniteFunction,
	isInfiniteFunction,
	isNaNFunction,
	logFunction,
	log10Function,
	log2Function,
	productFunction,
	roundFunction,
	sinFunction,
	sqrtFunction,
	sumFunction,
	tanFunction,
	truncFunction,
	
	// summary statistics functions
	maxFunction,
	meanFunction,
	minFunction,
	pmaxFunction,
	pminFunction,
	rangeFunction,
	sdFunction,
	
	// distribution drawing and density functions
	dnormFunction,
	rbinomFunction,
	rexpFunction,
	rgammaFunction,
	rlnormFunction,
	rnormFunction,
	rpoisFunction,
	runifFunction,
	rweibullFunction,
	
	// vector construction functions
	cFunction,
	floatFunction,
	integerFunction,
	logicalFunction,
	objectFunction,
	repFunction,
	repEachFunction,
	sampleFunction,
	seqFunction,
	seqAlongFunction,
	stringFunction,
	
	// value inspection/manipulation functions
	allFunction,
	anyFunction,
	catFunction,
	identicalFunction,
	ifelseFunction,
	matchFunction,
	ncharFunction,
	pasteFunction,
	printFunction,
	revFunction,
	sizeFunction,
	sortFunction,
	sortByFunction,
	strFunction,
	strsplitFunction,
	substrFunction,
	uniqueFunction,
	whichFunction,
	whichMaxFunction,
	whichMinFunction,
	
	// value type testing/coercion functions
	asFloatFunction,
	asIntegerFunction,
	asLogicalFunction,
	asStringFunction,
	elementTypeFunction,
	isFloatFunction,
	isIntegerFunction,
	isLogicalFunction,
	isNULLFunction,
	isObjectFunction,
	isStringFunction,
	typeFunction,
	
	// filesystem access functions
	filesAtPathFunction,
	readFileFunction,
	writeFileFunction,
	
	// miscellaneous functions
	applyFunction,
	dateFunction,
	doCallFunction,
	executeLambdaFunction,
	functionFunction,
	lsFunction,
	licenseFunction,
	rmFunction,
	setSeedFunction,
	getSeedFunction,
	stopFunction,
	timeFunction,
	versionFunction,
	
	// object instantiation
	_TestFunction
};

// Functions that are built into Eidos are handled internally.  However, it is also possible for external agents to register their own
// functions with Eidos; this is how the Context can get its functions into Eidos.  This registration is done with a delegate object and
// a delegate function pointer (NOT a pointer to method).  This is the prototype for a function implementation delegate.  The function
// pointed to will be called, and passed the delegate object along with other parameters for the function call.
typedef EidosValue_SP (*EidosDelegateFunctionPtr)(void *p_delegate, const std::string &p_function_name, const EidosValue_SP *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter);


#endif /* defined(__Eidos__eidos_functions__) */






























































