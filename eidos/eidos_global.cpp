//
//  eidos_global.cpp
//  Eidos
//
//  Created by Ben Haller on 6/28/15.
//  Copyright (c) 2015-2016 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/slim/
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


#include "eidos_global.h"
#include "eidos_script.h"
#include "eidos_value.h"
#include "eidos_interpreter.h"
#include "eidos_object_pool.h"
#include "eidos_ast_node.h"

#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <ctype.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <pwd.h>
#include <unistd.h>
#include <algorithm>
#include "string.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <memory>
#include <limits>


bool eidos_do_memory_checks = true;

EidosSymbolTable *gEidosConstantsSymbolTable = nullptr;


bool Eidos_GoodSymbolForDefine(std::string &p_symbol_name);
EidosValue_SP Eidos_ValueForCommandLineExpression(std::string &p_value_expression);


void Eidos_WarmUp(void)
{
	static bool been_here = false;
	
	if (!been_here)
	{
		been_here = true;
		
		// Make the shared EidosValue pool
		size_t maxEidosValueSize = sizeof(EidosValue_NULL);
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Logical));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Logical_const));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_String));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_String_vector));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_String_singleton));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Int));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Int_vector));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Int_singleton));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Float));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Float_vector));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Float_singleton));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Object));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Object_vector));
		maxEidosValueSize = std::max(maxEidosValueSize, sizeof(EidosValue_Object_singleton));
		
		//std::cout << "maxEidosValueSize == " << maxEidosValueSize << std::endl;
		gEidosValuePool = new EidosObjectPool(maxEidosValueSize);
		
		// Make the shared EidosASTNode pool
		gEidosASTNodePool = new EidosObjectPool(sizeof(EidosASTNode));
		
		// Allocate global permanents
		gStaticEidosValueNULL = EidosValue_NULL::Static_EidosValue_NULL();
		gStaticEidosValueNULLInvisible = EidosValue_NULL::Static_EidosValue_NULL_Invisible();
		
		gStaticEidosValue_Logical_ZeroVec = EidosValue_Logical_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Logical());
		gStaticEidosValue_Integer_ZeroVec = EidosValue_Int_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Int_vector());
		gStaticEidosValue_Float_ZeroVec = EidosValue_Float_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_vector());
		gStaticEidosValue_String_ZeroVec = EidosValue_String_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_String_vector());
		gStaticEidosValue_Object_ZeroVec = EidosValue_Object_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Object_vector(gEidos_UndefinedClassObject));
		
		gStaticEidosValue_LogicalT = EidosValue_Logical_const::Static_EidosValue_Logical_T();
		gStaticEidosValue_LogicalF = EidosValue_Logical_const::Static_EidosValue_Logical_F();
		
		gStaticEidosValue_Integer0 = EidosValue_Int_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Int_singleton(0));
		gStaticEidosValue_Integer1 = EidosValue_Int_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Int_singleton(1));
		
		gStaticEidosValue_Float0 = EidosValue_Float_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(0.0));
		gStaticEidosValue_Float0Point5 = EidosValue_Float_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(0.5));
		gStaticEidosValue_Float1 = EidosValue_Float_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(1.0));
		gStaticEidosValue_FloatINF = EidosValue_Float_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(std::numeric_limits<double>::infinity()));
		
		gStaticEidosValue_StringEmpty = EidosValue_String_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_String_singleton(""));
		gStaticEidosValue_StringSpace = EidosValue_String_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_String_singleton(" "));
		gStaticEidosValue_StringAsterisk = EidosValue_String_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_String_singleton("*"));
		gStaticEidosValue_StringDoubleAsterisk = EidosValue_String_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_String_singleton("**"));
		
		// Register global strings and IDs
		Eidos_RegisterGlobalStringsAndIDs();
		
		// Set up the built-in function map, which is immutable
		EidosInterpreter::CacheBuiltInFunctionMap();
		
		// Set up the symbol table for Eidos constants
		gEidosConstantsSymbolTable = new EidosSymbolTable(EidosSymbolTableType::kEidosIntrinsicConstantsTable, nullptr);
	}
}

bool Eidos_GoodSymbolForDefine(std::string &p_symbol_name)
{
	bool good_symbol = true;
	
	// Eidos constants are reserved
	if ((p_symbol_name == "T") || (p_symbol_name == "F") || (p_symbol_name == "NULL") || (p_symbol_name == "PI") || (p_symbol_name == "E") || (p_symbol_name == "INF") || (p_symbol_name == "NAN"))
		good_symbol = false;
	
	// Eidos keywords are reserved (probably won't reach here anyway)
	if ((p_symbol_name == "if") || (p_symbol_name == "else") || (p_symbol_name == "do") || (p_symbol_name == "while") || (p_symbol_name == "for") || (p_symbol_name == "in") || (p_symbol_name == "next") || (p_symbol_name == "break") || (p_symbol_name == "return"))
		good_symbol = false;
	
	// SLiM constants are reserved too; this code belongs in SLiM, but only
	// SLiM uses this facility right now anyway, so I'm not going to sweat it...
	if (p_symbol_name == "sim")
		good_symbol = false;
	
	int len = (int)p_symbol_name.length();
	
	if (len >= 2)
	{
		char first_ch = p_symbol_name[0];
		
		if ((first_ch == 'p') || (first_ch == 'g') || (first_ch == 'm') || (first_ch == 's'))
		{
			int ch_index;
			
			for (ch_index = 1; ch_index < len; ++ch_index)
			{
				char idx_ch = p_symbol_name[ch_index];
				
				if ((idx_ch < '0') || (idx_ch > '9'))
					break;
			}
			
			if (ch_index == len)
				good_symbol = false;
		}
	}
	
	return good_symbol;
}

EidosValue_SP Eidos_ValueForCommandLineExpression(std::string &p_value_expression)
{
	EidosValue_SP value;
	EidosScript script(p_value_expression);
	
	try
	{
		script.SetFinalSemicolonOptional(true);
		script.Tokenize();
		script.ParseInterpreterBlockToAST();
		
		EidosSymbolTable symbol_table(EidosSymbolTableType::kVariablesTable, gEidosConstantsSymbolTable);
		EidosInterpreter interpreter(script, symbol_table, *EidosInterpreter::BuiltInFunctionMap(), nullptr);
		
		value = interpreter.EvaluateInterpreterBlock(false);
	}
	catch (...)
	{
	}
	
	return value;
}

void Eidos_DefineConstantsFromCommandLine(std::vector<std::string> p_constants)
{
	// We want to throw exceptions, even in SLiM, so that we can catch them here
	bool save_throws = gEidosTerminateThrows;
	
	gEidosTerminateThrows = true;
	
	for (std::string &constant : p_constants)
	{
		// Each constant must be in the form x=y, where x is a valid identifier and y is a valid Eidos expression.
		// We parse the assignment using EidosScript, and work with the resulting AST, for generality.
		EidosScript script(constant);
		bool malformed = false;
		
		try
		{
			script.SetFinalSemicolonOptional(true);
			script.Tokenize();
			script.ParseInterpreterBlockToAST();
		}
		catch (...)
		{
			malformed = true;
		}
		
		if (!malformed)
		{
			//script.PrintTokens(std::cout);
			//script.PrintAST(std::cout);
			
			const EidosASTNode *AST = script.AST();
			
			if (AST && (AST->token_->token_type_ == EidosTokenType::kTokenInterpreterBlock) && (AST->children_.size() == 1))
			{
				const EidosASTNode *top_node = AST->children_[0];
				
				if (top_node && (top_node->token_->token_type_ == EidosTokenType::kTokenAssign) && (top_node->children_.size() == 2))
				{
					const EidosASTNode *left_node = top_node->children_[0];
					
					if (left_node && (left_node->token_->token_type_ == EidosTokenType::kTokenIdentifier) && (left_node->children_.size() == 0))
					{
						std::string symbol_name = left_node->token_->token_string_;
						
						// OK, if the symbol name is acceptable, keep digging
						if (Eidos_GoodSymbolForDefine(symbol_name))
						{
							const EidosASTNode *right_node = top_node->children_[1];
							
							if (right_node)
							{
								// Rather than try to make a new script with right_node as its root, we simply take the substring
								// to the right of the = operator and make a new script object from that, and evaluate that.
								// Note that the expression also parsed in the context of "value = <expr>", so this limits the
								// syntax allowed; the value cannot be a compound statement, for example.
								int32_t assign_end = top_node->token_->token_end_;
								std::string value_expression = constant.substr(assign_end + 1);
								EidosValue_SP x_value_sp = Eidos_ValueForCommandLineExpression(value_expression);
								
								if (x_value_sp)
								{
									//std::cout << "define " << symbol_name << " = " << value_expression << std::endl;
									
									// Permanently alter the global Eidos symbol table; don't do this at home!
									EidosGlobalStringID symbol_id = EidosGlobalStringIDForString(symbol_name);
									EidosSymbolTableEntry table_entry(symbol_id, x_value_sp);
									
									gEidosConstantsSymbolTable->InitializeConstantSymbolEntry(table_entry);
									
									continue;
								}
							}
						}
						else
						{
							gEidosTerminateThrows = save_throws;
							
							EIDOS_TERMINATION << "ERROR (Eidos_DefineConstantsFromCommandLine): illegal defined constant name \"" << symbol_name << "\"." << eidos_terminate(nullptr);
						}
					}
				}
			}
		}
		
		gEidosTerminateThrows = save_throws;
		
		// Terminate without putting out a script line/character diagnostic; that looks weird
		EIDOS_TERMINATION << "ERROR (Eidos_DefineConstantsFromCommandLine): malformed command-line constant definition: " << constant;
		
		if (gEidosTerminateThrows)
		{
			EIDOS_TERMINATION << eidos_terminate(nullptr);
		}
		else
		{
			// This is from operator<<(std::ostream& p_out, const eidos_terminate &p_terminator)
			EIDOS_TERMINATION << std::endl;
			EIDOS_TERMINATION.flush();
			exit(EXIT_FAILURE);
		}
	}
	
	gEidosTerminateThrows = save_throws;
}


// Information on the Context within which Eidos is running (if any).
std::string gEidosContextVersion;
std::string gEidosContextLicense;
std::string gEidosContextCitation;


// the part of the input file that caused an error; used to highlight the token or text that caused the error
int gEidosCharacterStartOfError = -1, gEidosCharacterEndOfError = -1;
int gEidosCharacterStartOfErrorUTF16 = -1, gEidosCharacterEndOfErrorUTF16 = -1;
EidosScript *gEidosCurrentScript = nullptr;
bool gEidosExecutingRuntimeScript = false;

int gEidosErrorLine = -1, gEidosErrorLineCharacter = -1;


// define string stream used for output when gEidosTerminateThrows == 1; otherwise, terminates call exit()
bool gEidosTerminateThrows = true;
std::ostringstream gEidosTermination;
bool gEidosTerminated;


/** Print a demangled stack backtrace of the caller function to FILE* out. */
void eidos_print_stacktrace(FILE *p_out, unsigned int p_max_frames)
{
	fprintf(p_out, "stack trace:\n");
	
	// storage array for stack trace address data
	void* addrlist[p_max_frames+1];
	
	// retrieve current stack addresses
	int addrlen = backtrace(addrlist, static_cast<int>(sizeof(addrlist) / sizeof(void*)));
	
	if (addrlen == 0)
	{
		fprintf(p_out, "  <empty, possibly corrupt>\n");
		return;
	}
	
	// resolve addresses into strings containing "filename(function+address)",
	// this array must be free()-ed
	char** symbollist = backtrace_symbols(addrlist, addrlen);
	
	// allocate string which will be filled with the demangled function name
	size_t funcnamesize = 256;
	char* funcname = (char*)malloc(funcnamesize);
	
	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for (int i = 1; i < addrlen; i++)
	{
		char *begin_name = 0, *end_name = 0, *begin_offset = 0, *end_offset = 0;
		
		// find parentheses and +address offset surrounding the mangled name:
		// ./module(function+0x15c) [0x8048a6d]
		for (char *p = symbollist[i]; *p; ++p)
		{
			if (*p == '(')
				begin_name = p;
			else if (*p == '+')
				begin_offset = p;
			else if (*p == ')' && begin_offset)
			{
				end_offset = p;
				break;
			}
		}
		
		// BCH 24 Dec 2014: backtrace_symbols() on OS X seems to return strings in a different, non-standard format.
		// Added this code in an attempt to parse that format.  No doubt it could be done more cleanly.  :->
		if (!(begin_name && begin_offset && end_offset
			  && begin_name < begin_offset))
		{
			enum class ParseState {
				kInWhitespace1 = 1,
				kInLineNumber,
				kInWhitespace2,
				kInPackageName,
				kInWhitespace3,
				kInAddress,
				kInWhitespace4,
				kInFunction,
				kInWhitespace5,
				kInPlus,
				kInWhitespace6,
				kInOffset,
				kInOverrun
			};
			ParseState parse_state = ParseState::kInWhitespace1;
			char *p;
			
			for (p = symbollist[i]; *p; ++p)
			{
				switch (parse_state)
				{
					case ParseState::kInWhitespace1:	if (!isspace(*p)) parse_state = ParseState::kInLineNumber;	break;
					case ParseState::kInLineNumber:		if (isspace(*p)) parse_state = ParseState::kInWhitespace2;	break;
					case ParseState::kInWhitespace2:	if (!isspace(*p)) parse_state = ParseState::kInPackageName;	break;
					case ParseState::kInPackageName:	if (isspace(*p)) parse_state = ParseState::kInWhitespace3;	break;
					case ParseState::kInWhitespace3:	if (!isspace(*p)) parse_state = ParseState::kInAddress;		break;
					case ParseState::kInAddress:		if (isspace(*p)) parse_state = ParseState::kInWhitespace4;	break;
					case ParseState::kInWhitespace4:
						if (!isspace(*p))
						{
							parse_state = ParseState::kInFunction;
							begin_name = p - 1;
						}
						break;
					case ParseState::kInFunction:
						if (isspace(*p))
						{
							parse_state = ParseState::kInWhitespace5;
							end_name = p;
						}
						break;
					case ParseState::kInWhitespace5:	if (!isspace(*p)) parse_state = ParseState::kInPlus;		break;
					case ParseState::kInPlus:			if (isspace(*p)) parse_state = ParseState::kInWhitespace6;	break;
					case ParseState::kInWhitespace6:
						if (!isspace(*p))
						{
							parse_state = ParseState::kInOffset;
							begin_offset = p - 1;
						}
						break;
					case ParseState::kInOffset:
						if (isspace(*p))
						{
							parse_state = ParseState::kInOverrun;
							end_offset = p;
						}
						break;
					case ParseState::kInOverrun:
						break;
				}
			}
			
			if (parse_state == ParseState::kInOffset && !end_offset)
				end_offset = p;
		}
		
		if (begin_name && begin_offset && end_offset
			&& begin_name < begin_offset)
		{
			*begin_name++ = '\0';
			if (end_name)
				*end_name = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';
			
			// mangled name is now in [begin_name, begin_offset) and caller
			// offset in [begin_offset, end_offset). now apply __cxa_demangle():
			
			int status;
			char *ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
			
			if (status == 0)
			{
				funcname = ret; // use possibly realloc()-ed string; static analyzer doesn't like this but it is OK I think
				fprintf(p_out, "  %s : %s + %s\n", symbollist[i], funcname, begin_offset);
			}
			else
			{
				// demangling failed. Output function name as a C function with
				// no arguments.
				fprintf(p_out, "  %s : %s() + %s\n", symbollist[i], begin_name, begin_offset);
			}
		}
		else
		{
			// couldn't parse the line? print the whole line.
			fprintf(p_out, "URF:  %s\n", symbollist[i]);
		}
	}
	
	free(funcname);
	free(symbollist);
	
	fflush(p_out);
}

void eidos_script_error_position(int p_start, int p_end, EidosScript *p_script)
{
	gEidosErrorLine = -1;
	gEidosErrorLineCharacter = -1;
	
	if (p_script && (p_start >= 0) && (p_end >= p_start))
	{
		// figure out the script line and position
		const std::string &script_string = p_script->String();
		int length = (int)script_string.length();
		
		if ((length >= p_start) && (length >= p_end))	// == is the EOF position, which we want to allow but have to treat carefully
		{
			int lineStart = (p_start < length) ? p_start : length - 1;
			int lineEnd = (p_end < length) ? p_end : length - 1;
			int lineNumber;
			
			for (; lineStart > 0; --lineStart)
				if ((script_string[lineStart - 1] == '\n') || (script_string[lineStart - 1] == '\r'))
					break;
			for (; lineEnd < length - 1; ++lineEnd)
				if ((script_string[lineEnd + 1] == '\n') || (script_string[lineEnd + 1] == '\r'))
					break;
			
			// Figure out the line number in the script where the error starts
			lineNumber = 1;
			
			for (int i = 0; i < lineStart; ++i)
				if (script_string[i] == '\n')
					lineNumber++;
			
			gEidosErrorLine = lineNumber;
			gEidosErrorLineCharacter = p_start - lineStart;
		}
	}
}

void eidos_log_script_error(std::ostream& p_out, int p_start, int p_end, EidosScript *p_script, bool p_inside_lambda)
{
	if (p_script && (p_start >= 0) && (p_end >= p_start))
	{
		// figure out the script line, print it, show the error position
		const std::string &script_string = p_script->String();
		int length = (int)script_string.length();
		
		if ((length >= p_start) && (length >= p_end))	// == is the EOF position, which we want to allow but have to treat carefully
		{
			int lineStart = (p_start < length) ? p_start : length - 1;
			int lineEnd = (p_end < length) ? p_end : length - 1;
			int lineNumber;
			
			for (; lineStart > 0; --lineStart)
				if ((script_string[lineStart - 1] == '\n') || (script_string[lineStart - 1] == '\r'))
					break;
			for (; lineEnd < length - 1; ++lineEnd)
				if ((script_string[lineEnd + 1] == '\n') || (script_string[lineEnd + 1] == '\r'))
					break;
			
			// Figure out the line number in the script where the error starts
			lineNumber = 1;
			
			for (int i = 0; i < lineStart; ++i)
				if (script_string[i] == '\n')
					lineNumber++;
			
			gEidosErrorLine = lineNumber;
			gEidosErrorLineCharacter = p_start - lineStart;
			
			p_out << std::endl << "Error on script line " << gEidosErrorLine << ", character " << gEidosErrorLineCharacter;
			
			if (p_inside_lambda)
				p_out << " (inside runtime script block)";
			
			p_out << ":" << std::endl << std::endl;
			
			// Emit the script line, converting tabs to three spaces
			for (int i = lineStart; i <= lineEnd; ++i)
			{
				char script_char = script_string[i];
				
				if (script_char == '\t')
					p_out << "   ";
				else if ((script_char == '\n') || (script_char == '\r'))	// don't show more than one line
					break;
				else
					p_out << script_char;
			}
			
			p_out << std::endl;
			
			// Emit the error indicator line, again emitting three spaces where the script had a tab
			for (int i = lineStart; i < p_start; ++i)
			{
				char script_char = script_string[i];
				
				if (script_char == '\t')
					p_out << "   ";
				else if ((script_char == '\n') || (script_char == '\r'))	// don't show more than one line
					break;
				else
					p_out << ' ';
			}
			
			// Emit the error indicator
			for (int i = 0; i < p_end - p_start + 1; ++i)
				p_out << "^";
			
			p_out << std::endl;
		}
	}
}

eidos_terminate::eidos_terminate(const EidosToken *p_error_token)
{
	// This is the end of the line, so we don't need to treat the error position as a stack
	if (p_error_token)
		EidosScript::PushErrorPositionFromToken(p_error_token);
}

eidos_terminate::eidos_terminate(bool p_print_backtrace) : print_backtrace_(p_print_backtrace)
{
}

eidos_terminate::eidos_terminate(const EidosToken *p_error_token, bool p_print_backtrace) : print_backtrace_(p_print_backtrace)
{
	// This is the end of the line, so we don't need to treat the error position as a stack
	if (p_error_token)
		EidosScript::PushErrorPositionFromToken(p_error_token);
}

void operator<<(std::ostream& p_out, const eidos_terminate &p_terminator)
{
	p_out << std::endl;
	
	p_out.flush();
	
	if (p_terminator.print_backtrace_)
		eidos_print_stacktrace(stderr);
	
	if (gEidosTerminateThrows)
	{
		// In this case, eidos_terminate() throws an exception that gets caught by the Context.  That invalidates the simulation object, and
		// causes the Context to display an error message and ends the simulation run, but it does not terminate the app.
		throw std::runtime_error("A runtime error occurred in Eidos");
	}
	else
	{
		// In this case, eidos_terminate() does in fact terminate; this is appropriate when errors are simply fatal and there is no UI.
		// In this case, we want to emit a diagnostic showing the line of script where the error occurred, if we can.
		// This facility uses only the non-UTF16 positions, since it is based on std::string, so those positions can be ignored.
		eidos_log_script_error(p_out, gEidosCharacterStartOfError, gEidosCharacterEndOfError, gEidosCurrentScript, gEidosExecutingRuntimeScript);
		
		exit(EXIT_FAILURE);
	}
}

std::string EidosGetTrimmedRaiseMessage(void)
{
	if (gEidosTerminateThrows)
	{
		std::string terminationMessage = gEidosTermination.str();
		
		gEidosTermination.clear();
		gEidosTermination.str(gEidosStr_empty_string);
		
		// trim off newlines at the end of the raise string
		size_t endpos = terminationMessage.find_last_not_of("\n\r");
		if (std::string::npos != endpos)
			terminationMessage = terminationMessage.substr(0, endpos + 1);
		
		return terminationMessage;
	}
	else
	{
		return gEidosStr_empty_string;
	}
}

std::string EidosGetUntrimmedRaiseMessage(void)
{
	if (gEidosTerminateThrows)
	{
		std::string terminationMessage = gEidosTermination.str();
		
		gEidosTermination.clear();
		gEidosTermination.str(gEidosStr_empty_string);
		
		return terminationMessage;
	}
	else
	{
		return gEidosStr_empty_string;
	}
}


//
//	The code below was obtained from http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use.  It may or may not work.
//	On Windows, it requires linking with Microsoft's psapi.lib.  That is left as an exercise for the reader.  Nadeau says "On other OSes, the default libraries are sufficient."
//

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t EidosGetPeakRSS(void)
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.PeakWorkingSetSize;
	
#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
	/* AIX and Solaris ------------------------------------------ */
	struct psinfo psinfo;
	int fd = -1;
	if ( (fd = open( "/proc/self/psinfo", O_RDONLY )) == -1 )
		return (size_t)0L;		/* Can't open? */
	if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) )
	{
		close( fd );
		return (size_t)0L;		/* Can't read? */
	}
	close( fd );
	return (size_t)(psinfo.pr_rssize * 1024L);
	
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
	/* BSD, Linux, and OSX -------------------------------------- */
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)
	return (size_t)rusage.ru_maxrss;
#else
	return (size_t)(rusage.ru_maxrss * 1024L);
#endif
	
#else
	/* Unknown OS ----------------------------------------------- */
	return (size_t)0L;			/* Unsupported. */
#endif
}


/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t EidosGetCurrentRSS(void)
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.WorkingSetSize;
	
#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
				   (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (size_t)0L;		/* Can't access? */
	return (size_t)info.resident_size;
	
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
		return (size_t)0L;		/* Can't open? */
	if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
	{
		fclose( fp );
		return (size_t)0L;		/* Can't read? */
	}
	fclose( fp );
	return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);
	
#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (size_t)0L;			/* Unsupported. */
#endif
}


// resolve a leading ~ in a filesystem path to the user's home directory
std::string EidosResolvedPath(const std::string p_path)
{
	std::string path = p_path;
	
	// if there is a leading '~', replace it with the user's home directory; not sure if this works on Windows...
	if ((path.length() > 0) && (path[0] == '~'))
	{
		const char *homedir;
		
		if ((homedir = getenv("HOME")) == NULL)
			homedir = getpwuid(getuid())->pw_dir;
		
		if (strlen(homedir))
			path.replace(0, 1, homedir);
	}
	
	return path;
}

// run a Un*x command; thanks to http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
std::string EidosExec(const char* cmd)
{
	char buffer[128];
	std::string result = "";
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (!pipe) throw std::runtime_error("popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	return result;
}

size_t EidosGetMaxRSS(void)
{
	static bool beenHere = false;
	static size_t max_rss = 0;
	
	if (!beenHere)
	{
#if 0
		// Find our RSS limit by launching a subshell to run "ulimit -m"
		std::string limit_string = EidosExec("ulimit -m");
		
		std::string unlimited("unlimited");
		
		if (std::mismatch(unlimited.begin(), unlimited.end(), limit_string.begin()).first == unlimited.end())
		{
			// "unlimited" is a prefix of foobar, so use 0 to represent that
			max_rss = 0;
		}
		else
		{
			errno = 0;
			
			const char *c_str = limit_string.c_str();
			char *last_used_char = nullptr;
			
			max_rss = strtoq(c_str, &last_used_char, 10);
			
			if (errno || (last_used_char == c_str))
			{
				// If an error occurs, assume we are unlimited
				max_rss = 0;
			}
			else
			{
				// This value is in 1024-byte units, so multiply to get a limit in bytes
				max_rss *= 1024L;
			}
		}
#else
		// Find our RSS limit using getrlimit() – easier and safer
		struct rlimit rlim;
		
		if (getrlimit(RLIMIT_RSS, &rlim) == 0)
		{
			// This value is in bytes, no scaling needed
			max_rss = (uint64_t)rlim.rlim_max;
			
			// If the claim is that we have more than 1024 TB at our disposal, then we will consider ourselves unlimited :->
			if (max_rss > 1024L * 1024L * 1024L * 1024L * 1024L)
				max_rss = 0;
		}
		else
		{
			// If an error occurs, assume we are unlimited
			max_rss = 0;
		}
#endif
		
		beenHere = true;
	}
	
	return max_rss;
}

void EidosCheckRSSAgainstMax(std::string p_message1, std::string p_message2)
{
	static bool beenHere = false;
	static size_t max_rss = 0;
	
	if (!beenHere)
	{
		// The first time we are called, we get the memory limit and sanity-check it
		max_rss = EidosGetMaxRSS();
		
#if 0
		// Impose a 20 MB limit, for testing
		max_rss = 20*1024*1024;
#warning Turn this off!
#endif
		
		if (max_rss != 0)
		{
			size_t current_rss = EidosGetCurrentRSS();
			
			// If we are already within 10 MB of overrunning our supposed limit, disable checking; assume that
			// either EidosGetMaxRSS() or EidosGetCurrentRSS() is not telling us the truth.
			if (current_rss + 10L*1024L*1024L > max_rss)
				max_rss = 0;
		}
		
		// Switch off our memory check flag if we are not going to enforce a limit anyway;
		// this allows the caller to skip calling us when possible, for speed
		if (max_rss == 0)
			eidos_do_memory_checks = false;
		
		beenHere = true;
	}
	
	if (eidos_do_memory_checks && (max_rss != 0))
	{
		size_t current_rss = EidosGetCurrentRSS();
		
		// If we are within 10 MB of overrunning our limit, then terminate with a message before
		// the system does it for us.  10 MB gives us a little headroom, so that we detect this
		// condition before the system does.
		if (current_rss + 10L*1024L*1024L > max_rss)
		{
			// We output our warning to std::cerr, because we may get killed by the OS for exceeding our memory limit before other streams would get flushed
			std::cerr << "WARNING (" << p_message1 << "): memory usage of " << (current_rss / (1024.0 * 1024.0)) << " MB is dangerously close to the limit of " << (max_rss / (1024.0 * 1024.0)) << " MB reported by the operating system.  This SLiM process may soon be killed by the operating system for exceeding the memory limit.  You might raise the per-process memory limit, or modify your model to decrease memory usage.  You can turn off this memory check with the '-x' command-line option.  " << p_message2 << std::endl;
			std::cerr.flush();
			
			// We want to issue only one warning, so turn off warnings now
			eidos_do_memory_checks = false;
		}
	}
}


//	Global std::string objects.
const std::string gEidosStr_empty_string = "";
const std::string gEidosStr_space_string = " ";

// mostly function names used in multiple places
const std::string gEidosStr_function = "function";
const std::string gEidosStr_method = "method";
const std::string gEidosStr_apply = "apply";
const std::string gEidosStr_doCall = "doCall";
const std::string gEidosStr_executeLambda = "executeLambda";
const std::string gEidosStr_ls = "ls";
const std::string gEidosStr_rm = "rm";

// mostly language keywords
const std::string gEidosStr_if = "if";
const std::string gEidosStr_else = "else";
const std::string gEidosStr_do = "do";
const std::string gEidosStr_while = "while";
const std::string gEidosStr_for = "for";
const std::string gEidosStr_in = "in";
const std::string gEidosStr_next = "next";
const std::string gEidosStr_break = "break";
const std::string gEidosStr_return = "return";

// mostly Eidos global constants
const std::string gEidosStr_T = "T";
const std::string gEidosStr_F = "F";
const std::string gEidosStr_NULL = "NULL";
const std::string gEidosStr_PI = "PI";
const std::string gEidosStr_E = "E";
const std::string gEidosStr_INF = "INF";
const std::string gEidosStr_MINUS_INF = "-INF";
const std::string gEidosStr_NAN = "NAN";

// mostly Eidos type names
const std::string gEidosStr_void = "void";
const std::string gEidosStr_logical = "logical";
const std::string gEidosStr_string = "string";
const std::string gEidosStr_integer = "integer";
const std::string gEidosStr_float = "float";
const std::string gEidosStr_object = "object";
const std::string gEidosStr_numeric = "numeric";

// Eidos function names, property names, and method names
const std::string gEidosStr_size = "size";
const std::string gEidosStr_property = "property";
const std::string gEidosStr_str = "str";

// other miscellaneous strings
const std::string gEidosStr_GetPropertyOfElements = "GetPropertyOfElements";
const std::string gEidosStr_ExecuteInstanceMethod = "ExecuteInstanceMethod";
const std::string gEidosStr_undefined = "undefined";
const std::string gEidosStr_applyValue = "applyValue";

// strings for Eidos_TestElement
const std::string gEidosStr__TestElement = "_TestElement";
const std::string gEidosStr__yolk = "_yolk";
const std::string gEidosStr__increment = "_increment";
const std::string gEidosStr__cubicYolk = "_cubicYolk";
const std::string gEidosStr__squareTest = "_squareTest";

// strings for parameters, function names, etc., that are needed as explicit registrations in a Context and thus have to be
// explicitly registered by Eidos; see the comment in Eidos_RegisterStringForGlobalID() below
const std::string gEidosStr_weights = "weights";
const std::string gEidosStr_n = "n";
const std::string gEidosStr_x = "x";
const std::string gEidosStr_y = "y";
const std::string gEidosStr_z = "z";
const std::string gEidosStr_color = "color";


static std::unordered_map<std::string, EidosGlobalStringID> gStringToID;
static std::unordered_map<EidosGlobalStringID, const std::string *> gIDToString;
static EidosGlobalStringID gNextUnusedID = gEidosID_LastContextEntry;


void Eidos_RegisterStringForGlobalID(const std::string &p_string, EidosGlobalStringID p_string_id)
{
	// BCH 13 September 2016: So, this is a tricky issue without a good resolution at the moment.  Eidos explicitly registers
	// a few strings, using this method, right below in Eidos_RegisterGlobalStringsAndIDs().  And SLiM explicitly registers
	// a bunch more strings, in SLiM_RegisterGlobalStringsAndIDs().  So far so good.  But Eidos also registers a bunch of
	// strings "in passing", as a side effect of calling EidosGlobalStringIDForString(), because it doesn't care what the
	// IDs of those strings are, it just wants them to be registered for later matching.  This happens to function names and
	// parameter names, in particular.  This is good; we don't want to have to explicitly enumerate and register all of those
	// strings, that would be a tremendous pain.  The problem is that these "in passing" registrations can conflict with
	// registrations done in the Context, unpredictably.  A new parameter named "weights" is added to a new Eidos function,
	// and suddenly the explicit registration of "weights" in SLiM has broken and needs to be removed.  At least you know
	// that that has happened, because you end up here.  When you end up here, don't just comment out the registration call
	// in the Context; you also need to add an explicit registration call in Eidos, and remove the string and ID definitions
	// in the Context, and so forth.  Migrate the whole explicit registration from the Context back into Eidos.  Unfortunate,
	// but I don't see any good solution.  Sure is nice how uniquing of selectors just happens automatically in Obj-C!  That
	// is basically what we're trying to duplicate here, without language support.
	if (gStringToID.find(p_string) != gStringToID.end())
		EIDOS_TERMINATION << "ERROR (Eidos_RegisterStringForGlobalID): string " << p_string << " has already been registered." << eidos_terminate(nullptr);
	
	if (gIDToString.find(p_string_id) != gIDToString.end())
		EIDOS_TERMINATION << "ERROR (Eidos_RegisterStringForGlobalID): id " << p_string_id << " has already been registered." << eidos_terminate(nullptr);
	
	if (p_string_id >= gEidosID_LastContextEntry)
		EIDOS_TERMINATION << "ERROR (Eidos_RegisterStringForGlobalID): id " << p_string_id << " it out of the legal range for preregistered strings." << eidos_terminate(nullptr);
	
	gStringToID[p_string] = p_string_id;
	gIDToString[p_string_id] = &p_string;
}

void Eidos_RegisterGlobalStringsAndIDs(void)
{
	static bool been_here = false;
	
	if (!been_here)
	{
		been_here = true;
		
		Eidos_RegisterStringForGlobalID(gEidosStr_method, gEidosID_method);
		Eidos_RegisterStringForGlobalID(gEidosStr_size, gEidosID_size);
		Eidos_RegisterStringForGlobalID(gEidosStr_property, gEidosID_property);
		Eidos_RegisterStringForGlobalID(gEidosStr_str, gEidosID_str);
		Eidos_RegisterStringForGlobalID(gEidosStr_applyValue, gEidosID_applyValue);
		
		Eidos_RegisterStringForGlobalID(gEidosStr_T, gEidosID_T);
		Eidos_RegisterStringForGlobalID(gEidosStr_F, gEidosID_F);
		Eidos_RegisterStringForGlobalID(gEidosStr_NULL, gEidosID_NULL);
		Eidos_RegisterStringForGlobalID(gEidosStr_PI, gEidosID_PI);
		Eidos_RegisterStringForGlobalID(gEidosStr_E, gEidosID_E);
		Eidos_RegisterStringForGlobalID(gEidosStr_INF, gEidosID_INF);
		Eidos_RegisterStringForGlobalID(gEidosStr_NAN, gEidosID_NAN);
		
		Eidos_RegisterStringForGlobalID(gEidosStr__TestElement, gEidosID__TestElement);
		Eidos_RegisterStringForGlobalID(gEidosStr__yolk, gEidosID__yolk);
		Eidos_RegisterStringForGlobalID(gEidosStr__increment, gEidosID__increment);
		Eidos_RegisterStringForGlobalID(gEidosStr__cubicYolk, gEidosID__cubicYolk);
		Eidos_RegisterStringForGlobalID(gEidosStr__squareTest, gEidosID__squareTest);
		
		Eidos_RegisterStringForGlobalID(gEidosStr_weights, gEidosID_weights);
		Eidos_RegisterStringForGlobalID(gEidosStr_n, gEidosID_n);
		Eidos_RegisterStringForGlobalID(gEidosStr_x, gEidosID_x);
		Eidos_RegisterStringForGlobalID(gEidosStr_y, gEidosID_y);
		Eidos_RegisterStringForGlobalID(gEidosStr_z, gEidosID_z);
		Eidos_RegisterStringForGlobalID(gEidosStr_color, gEidosID_color);
	}
}

EidosGlobalStringID EidosGlobalStringIDForString(const std::string &p_string)
{
	//std::cerr << "EidosGlobalStringIDForString: " << p_string << std::endl;
	auto found_iter = gStringToID.find(p_string);
	
	if (found_iter == gStringToID.end())
	{
		// If the hash table does not already contain this key, then we add it to the table as a side effect.
		// We have to copy the string, because we have no idea what the caller might do with the string they passed us.
		// Since the hash table makes its own copy of the key, this copy is used only for the value in the hash tables.
		const std::string *copied_string = new const std::string(p_string);
		uint32_t string_id = gNextUnusedID++;
		
		gStringToID[*copied_string] = string_id;
		gIDToString[string_id] = copied_string;
		
		return string_id;
	}
	else
		return found_iter->second;
}

const std::string &StringForEidosGlobalStringID(EidosGlobalStringID p_string_id)
{
	//std::cerr << "StringForEidosGlobalStringID: " << p_string_id << std::endl;
	auto found_iter = gIDToString.find(p_string_id);
	
	if (found_iter == gIDToString.end())
		return gEidosStr_undefined;
	else
		return *(found_iter->second);
}


// *******************************************************************************************************************
//
//	Support for named / specified colors in Eidos
//

// Named colors.  This list is taken directly from R, used under their GPL-3.

EidosNamedColor gEidosNamedColors[] = {
	{"white", 255, 255, 255},
	{"aliceblue", 240, 248, 255},
	{"antiquewhite", 250, 235, 215},
	{"antiquewhite1", 255, 239, 219},
	{"antiquewhite2", 238, 223, 204},
	{"antiquewhite3", 205, 192, 176},
	{"antiquewhite4", 139, 131, 120},
	{"aquamarine", 127, 255, 212},
	{"aquamarine1", 127, 255, 212},
	{"aquamarine2", 118, 238, 198},
	{"aquamarine3", 102, 205, 170},
	{"aquamarine4", 69, 139, 116},
	{"azure", 240, 255, 255},
	{"azure1", 240, 255, 255},
	{"azure2", 224, 238, 238},
	{"azure3", 193, 205, 205},
	{"azure4", 131, 139, 139},
	{"beige", 245, 245, 220},
	{"bisque", 255, 228, 196},
	{"bisque1", 255, 228, 196},
	{"bisque2", 238, 213, 183},
	{"bisque3", 205, 183, 158},
	{"bisque4", 139, 125, 107},
	{"black", 0, 0, 0},
	{"blanchedalmond", 255, 235, 205},
	{"blue", 0, 0, 255},
	{"blue1", 0, 0, 255},
	{"blue2", 0, 0, 238},
	{"blue3", 0, 0, 205},
	{"blue4", 0, 0, 139},
	{"blueviolet", 138, 43, 226},
	{"brown", 165, 42, 42},
	{"brown1", 255, 64, 64},
	{"brown2", 238, 59, 59},
	{"brown3", 205, 51, 51},
	{"brown4", 139, 35, 35},
	{"burlywood", 222, 184, 135},
	{"burlywood1", 255, 211, 155},
	{"burlywood2", 238, 197, 145},
	{"burlywood3", 205, 170, 125},
	{"burlywood4", 139, 115, 85},
	{"cadetblue", 95, 158, 160},
	{"cadetblue1", 152, 245, 255},
	{"cadetblue2", 142, 229, 238},
	{"cadetblue3", 122, 197, 205},
	{"cadetblue4", 83, 134, 139},
	{"chartreuse", 127, 255, 0},
	{"chartreuse1", 127, 255, 0},
	{"chartreuse2", 118, 238, 0},
	{"chartreuse3", 102, 205, 0},
	{"chartreuse4", 69, 139, 0},
	{"chocolate", 210, 105, 30},
	{"chocolate1", 255, 127, 36},
	{"chocolate2", 238, 118, 33},
	{"chocolate3", 205, 102, 29},
	{"chocolate4", 139, 69, 19},
	{"coral", 255, 127, 80},
	{"coral1", 255, 114, 86},
	{"coral2", 238, 106, 80},
	{"coral3", 205, 91, 69},
	{"coral4", 139, 62, 47},
	{"cornflowerblue", 100, 149, 237},
	{"cornsilk", 255, 248, 220},
	{"cornsilk1", 255, 248, 220},
	{"cornsilk2", 238, 232, 205},
	{"cornsilk3", 205, 200, 177},
	{"cornsilk4", 139, 136, 120},
	{"cyan", 0, 255, 255},
	{"cyan1", 0, 255, 255},
	{"cyan2", 0, 238, 238},
	{"cyan3", 0, 205, 205},
	{"cyan4", 0, 139, 139},
	{"darkblue", 0, 0, 139},
	{"darkcyan", 0, 139, 139},
	{"darkgoldenrod", 184, 134, 11},
	{"darkgoldenrod1", 255, 185, 15},
	{"darkgoldenrod2", 238, 173, 14},
	{"darkgoldenrod3", 205, 149, 12},
	{"darkgoldenrod4", 139, 101, 8},
	{"darkgray", 169, 169, 169},
	{"darkgreen", 0, 100, 0},
	{"darkgrey", 169, 169, 169},
	{"darkkhaki", 189, 183, 107},
	{"darkmagenta", 139, 0, 139},
	{"darkolivegreen", 85, 107, 47},
	{"darkolivegreen1", 202, 255, 112},
	{"darkolivegreen2", 188, 238, 104},
	{"darkolivegreen3", 162, 205, 90},
	{"darkolivegreen4", 110, 139, 61},
	{"darkorange", 255, 140, 0},
	{"darkorange1", 255, 127, 0},
	{"darkorange2", 238, 118, 0},
	{"darkorange3", 205, 102, 0},
	{"darkorange4", 139, 69, 0},
	{"darkorchid", 153, 50, 204},
	{"darkorchid1", 191, 62, 255},
	{"darkorchid2", 178, 58, 238},
	{"darkorchid3", 154, 50, 205},
	{"darkorchid4", 104, 34, 139},
	{"darkred", 139, 0, 0},
	{"darksalmon", 233, 150, 122},
	{"darkseagreen", 143, 188, 143},
	{"darkseagreen1", 193, 255, 193},
	{"darkseagreen2", 180, 238, 180},
	{"darkseagreen3", 155, 205, 155},
	{"darkseagreen4", 105, 139, 105},
	{"darkslateblue", 72, 61, 139},
	{"darkslategray", 47, 79, 79},
	{"darkslategray1", 151, 255, 255},
	{"darkslategray2", 141, 238, 238},
	{"darkslategray3", 121, 205, 205},
	{"darkslategray4", 82, 139, 139},
	{"darkslategrey", 47, 79, 79},
	{"darkturquoise", 0, 206, 209},
	{"darkviolet", 148, 0, 211},
	{"deeppink", 255, 20, 147},
	{"deeppink1", 255, 20, 147},
	{"deeppink2", 238, 18, 137},
	{"deeppink3", 205, 16, 118},
	{"deeppink4", 139, 10, 80},
	{"deepskyblue", 0, 191, 255},
	{"deepskyblue1", 0, 191, 255},
	{"deepskyblue2", 0, 178, 238},
	{"deepskyblue3", 0, 154, 205},
	{"deepskyblue4", 0, 104, 139},
	{"dimgray", 105, 105, 105},
	{"dimgrey", 105, 105, 105},
	{"dodgerblue", 30, 144, 255},
	{"dodgerblue1", 30, 144, 255},
	{"dodgerblue2", 28, 134, 238},
	{"dodgerblue3", 24, 116, 205},
	{"dodgerblue4", 16, 78, 139},
	{"firebrick", 178, 34, 34},
	{"firebrick1", 255, 48, 48},
	{"firebrick2", 238, 44, 44},
	{"firebrick3", 205, 38, 38},
	{"firebrick4", 139, 26, 26},
	{"floralwhite", 255, 250, 240},
	{"forestgreen", 34, 139, 34},
	{"gainsboro", 220, 220, 220},
	{"ghostwhite", 248, 248, 255},
	{"gold", 255, 215, 0},
	{"gold1", 255, 215, 0},
	{"gold2", 238, 201, 0},
	{"gold3", 205, 173, 0},
	{"gold4", 139, 117, 0},
	{"goldenrod", 218, 165, 32},
	{"goldenrod1", 255, 193, 37},
	{"goldenrod2", 238, 180, 34},
	{"goldenrod3", 205, 155, 29},
	{"goldenrod4", 139, 105, 20},
	{"gray", 190, 190, 190},
	{"gray0", 0, 0, 0},
	{"gray1", 3, 3, 3},
	{"gray2", 5, 5, 5},
	{"gray3", 8, 8, 8},
	{"gray4", 10, 10, 10},
	{"gray5", 13, 13, 13},
	{"gray6", 15, 15, 15},
	{"gray7", 18, 18, 18},
	{"gray8", 20, 20, 20},
	{"gray9", 23, 23, 23},
	{"gray10", 26, 26, 26},
	{"gray11", 28, 28, 28},
	{"gray12", 31, 31, 31},
	{"gray13", 33, 33, 33},
	{"gray14", 36, 36, 36},
	{"gray15", 38, 38, 38},
	{"gray16", 41, 41, 41},
	{"gray17", 43, 43, 43},
	{"gray18", 46, 46, 46},
	{"gray19", 48, 48, 48},
	{"gray20", 51, 51, 51},
	{"gray21", 54, 54, 54},
	{"gray22", 56, 56, 56},
	{"gray23", 59, 59, 59},
	{"gray24", 61, 61, 61},
	{"gray25", 64, 64, 64},
	{"gray26", 66, 66, 66},
	{"gray27", 69, 69, 69},
	{"gray28", 71, 71, 71},
	{"gray29", 74, 74, 74},
	{"gray30", 77, 77, 77},
	{"gray31", 79, 79, 79},
	{"gray32", 82, 82, 82},
	{"gray33", 84, 84, 84},
	{"gray34", 87, 87, 87},
	{"gray35", 89, 89, 89},
	{"gray36", 92, 92, 92},
	{"gray37", 94, 94, 94},
	{"gray38", 97, 97, 97},
	{"gray39", 99, 99, 99},
	{"gray40", 102, 102, 102},
	{"gray41", 105, 105, 105},
	{"gray42", 107, 107, 107},
	{"gray43", 110, 110, 110},
	{"gray44", 112, 112, 112},
	{"gray45", 115, 115, 115},
	{"gray46", 117, 117, 117},
	{"gray47", 120, 120, 120},
	{"gray48", 122, 122, 122},
	{"gray49", 125, 125, 125},
	{"gray50", 127, 127, 127},
	{"gray51", 130, 130, 130},
	{"gray52", 133, 133, 133},
	{"gray53", 135, 135, 135},
	{"gray54", 138, 138, 138},
	{"gray55", 140, 140, 140},
	{"gray56", 143, 143, 143},
	{"gray57", 145, 145, 145},
	{"gray58", 148, 148, 148},
	{"gray59", 150, 150, 150},
	{"gray60", 153, 153, 153},
	{"gray61", 156, 156, 156},
	{"gray62", 158, 158, 158},
	{"gray63", 161, 161, 161},
	{"gray64", 163, 163, 163},
	{"gray65", 166, 166, 166},
	{"gray66", 168, 168, 168},
	{"gray67", 171, 171, 171},
	{"gray68", 173, 173, 173},
	{"gray69", 176, 176, 176},
	{"gray70", 179, 179, 179},
	{"gray71", 181, 181, 181},
	{"gray72", 184, 184, 184},
	{"gray73", 186, 186, 186},
	{"gray74", 189, 189, 189},
	{"gray75", 191, 191, 191},
	{"gray76", 194, 194, 194},
	{"gray77", 196, 196, 196},
	{"gray78", 199, 199, 199},
	{"gray79", 201, 201, 201},
	{"gray80", 204, 204, 204},
	{"gray81", 207, 207, 207},
	{"gray82", 209, 209, 209},
	{"gray83", 212, 212, 212},
	{"gray84", 214, 214, 214},
	{"gray85", 217, 217, 217},
	{"gray86", 219, 219, 219},
	{"gray87", 222, 222, 222},
	{"gray88", 224, 224, 224},
	{"gray89", 227, 227, 227},
	{"gray90", 229, 229, 229},
	{"gray91", 232, 232, 232},
	{"gray92", 235, 235, 235},
	{"gray93", 237, 237, 237},
	{"gray94", 240, 240, 240},
	{"gray95", 242, 242, 242},
	{"gray96", 245, 245, 245},
	{"gray97", 247, 247, 247},
	{"gray98", 250, 250, 250},
	{"gray99", 252, 252, 252},
	{"gray100", 255, 255, 255},
	{"green", 0, 255, 0},
	{"green1", 0, 255, 0},
	{"green2", 0, 238, 0},
	{"green3", 0, 205, 0},
	{"green4", 0, 139, 0},
	{"greenyellow", 173, 255, 47},
	{"grey", 190, 190, 190},
	{"grey0", 0, 0, 0},
	{"grey1", 3, 3, 3},
	{"grey2", 5, 5, 5},
	{"grey3", 8, 8, 8},
	{"grey4", 10, 10, 10},
	{"grey5", 13, 13, 13},
	{"grey6", 15, 15, 15},
	{"grey7", 18, 18, 18},
	{"grey8", 20, 20, 20},
	{"grey9", 23, 23, 23},
	{"grey10", 26, 26, 26},
	{"grey11", 28, 28, 28},
	{"grey12", 31, 31, 31},
	{"grey13", 33, 33, 33},
	{"grey14", 36, 36, 36},
	{"grey15", 38, 38, 38},
	{"grey16", 41, 41, 41},
	{"grey17", 43, 43, 43},
	{"grey18", 46, 46, 46},
	{"grey19", 48, 48, 48},
	{"grey20", 51, 51, 51},
	{"grey21", 54, 54, 54},
	{"grey22", 56, 56, 56},
	{"grey23", 59, 59, 59},
	{"grey24", 61, 61, 61},
	{"grey25", 64, 64, 64},
	{"grey26", 66, 66, 66},
	{"grey27", 69, 69, 69},
	{"grey28", 71, 71, 71},
	{"grey29", 74, 74, 74},
	{"grey30", 77, 77, 77},
	{"grey31", 79, 79, 79},
	{"grey32", 82, 82, 82},
	{"grey33", 84, 84, 84},
	{"grey34", 87, 87, 87},
	{"grey35", 89, 89, 89},
	{"grey36", 92, 92, 92},
	{"grey37", 94, 94, 94},
	{"grey38", 97, 97, 97},
	{"grey39", 99, 99, 99},
	{"grey40", 102, 102, 102},
	{"grey41", 105, 105, 105},
	{"grey42", 107, 107, 107},
	{"grey43", 110, 110, 110},
	{"grey44", 112, 112, 112},
	{"grey45", 115, 115, 115},
	{"grey46", 117, 117, 117},
	{"grey47", 120, 120, 120},
	{"grey48", 122, 122, 122},
	{"grey49", 125, 125, 125},
	{"grey50", 127, 127, 127},
	{"grey51", 130, 130, 130},
	{"grey52", 133, 133, 133},
	{"grey53", 135, 135, 135},
	{"grey54", 138, 138, 138},
	{"grey55", 140, 140, 140},
	{"grey56", 143, 143, 143},
	{"grey57", 145, 145, 145},
	{"grey58", 148, 148, 148},
	{"grey59", 150, 150, 150},
	{"grey60", 153, 153, 153},
	{"grey61", 156, 156, 156},
	{"grey62", 158, 158, 158},
	{"grey63", 161, 161, 161},
	{"grey64", 163, 163, 163},
	{"grey65", 166, 166, 166},
	{"grey66", 168, 168, 168},
	{"grey67", 171, 171, 171},
	{"grey68", 173, 173, 173},
	{"grey69", 176, 176, 176},
	{"grey70", 179, 179, 179},
	{"grey71", 181, 181, 181},
	{"grey72", 184, 184, 184},
	{"grey73", 186, 186, 186},
	{"grey74", 189, 189, 189},
	{"grey75", 191, 191, 191},
	{"grey76", 194, 194, 194},
	{"grey77", 196, 196, 196},
	{"grey78", 199, 199, 199},
	{"grey79", 201, 201, 201},
	{"grey80", 204, 204, 204},
	{"grey81", 207, 207, 207},
	{"grey82", 209, 209, 209},
	{"grey83", 212, 212, 212},
	{"grey84", 214, 214, 214},
	{"grey85", 217, 217, 217},
	{"grey86", 219, 219, 219},
	{"grey87", 222, 222, 222},
	{"grey88", 224, 224, 224},
	{"grey89", 227, 227, 227},
	{"grey90", 229, 229, 229},
	{"grey91", 232, 232, 232},
	{"grey92", 235, 235, 235},
	{"grey93", 237, 237, 237},
	{"grey94", 240, 240, 240},
	{"grey95", 242, 242, 242},
	{"grey96", 245, 245, 245},
	{"grey97", 247, 247, 247},
	{"grey98", 250, 250, 250},
	{"grey99", 252, 252, 252},
	{"grey100", 255, 255, 255},
	{"honeydew", 240, 255, 240},
	{"honeydew1", 240, 255, 240},
	{"honeydew2", 224, 238, 224},
	{"honeydew3", 193, 205, 193},
	{"honeydew4", 131, 139, 131},
	{"hotpink", 255, 105, 180},
	{"hotpink1", 255, 110, 180},
	{"hotpink2", 238, 106, 167},
	{"hotpink3", 205, 96, 144},
	{"hotpink4", 139, 58, 98},
	{"indianred", 205, 92, 92},
	{"indianred1", 255, 106, 106},
	{"indianred2", 238, 99, 99},
	{"indianred3", 205, 85, 85},
	{"indianred4", 139, 58, 58},
	{"ivory", 255, 255, 240},
	{"ivory1", 255, 255, 240},
	{"ivory2", 238, 238, 224},
	{"ivory3", 205, 205, 193},
	{"ivory4", 139, 139, 131},
	{"khaki", 240, 230, 140},
	{"khaki1", 255, 246, 143},
	{"khaki2", 238, 230, 133},
	{"khaki3", 205, 198, 115},
	{"khaki4", 139, 134, 78},
	{"lavender", 230, 230, 250},
	{"lavenderblush", 255, 240, 245},
	{"lavenderblush1", 255, 240, 245},
	{"lavenderblush2", 238, 224, 229},
	{"lavenderblush3", 205, 193, 197},
	{"lavenderblush4", 139, 131, 134},
	{"lawngreen", 124, 252, 0},
	{"lemonchiffon", 255, 250, 205},
	{"lemonchiffon1", 255, 250, 205},
	{"lemonchiffon2", 238, 233, 191},
	{"lemonchiffon3", 205, 201, 165},
	{"lemonchiffon4", 139, 137, 112},
	{"lightblue", 173, 216, 230},
	{"lightblue1", 191, 239, 255},
	{"lightblue2", 178, 223, 238},
	{"lightblue3", 154, 192, 205},
	{"lightblue4", 104, 131, 139},
	{"lightcoral", 240, 128, 128},
	{"lightcyan", 224, 255, 255},
	{"lightcyan1", 224, 255, 255},
	{"lightcyan2", 209, 238, 238},
	{"lightcyan3", 180, 205, 205},
	{"lightcyan4", 122, 139, 139},
	{"lightgoldenrod", 238, 221, 130},
	{"lightgoldenrod1", 255, 236, 139},
	{"lightgoldenrod2", 238, 220, 130},
	{"lightgoldenrod3", 205, 190, 112},
	{"lightgoldenrod4", 139, 129, 76},
	{"lightgoldenrodyellow", 250, 250, 210},
	{"lightgray", 211, 211, 211},
	{"lightgreen", 144, 238, 144},
	{"lightgrey", 211, 211, 211},
	{"lightpink", 255, 182, 193},
	{"lightpink1", 255, 174, 185},
	{"lightpink2", 238, 162, 173},
	{"lightpink3", 205, 140, 149},
	{"lightpink4", 139, 95, 101},
	{"lightsalmon", 255, 160, 122},
	{"lightsalmon1", 255, 160, 122},
	{"lightsalmon2", 238, 149, 114},
	{"lightsalmon3", 205, 129, 98},
	{"lightsalmon4", 139, 87, 66},
	{"lightseagreen", 32, 178, 170},
	{"lightskyblue", 135, 206, 250},
	{"lightskyblue1", 176, 226, 255},
	{"lightskyblue2", 164, 211, 238},
	{"lightskyblue3", 141, 182, 205},
	{"lightskyblue4", 96, 123, 139},
	{"lightslateblue", 132, 112, 255},
	{"lightslategray", 119, 136, 153},
	{"lightslategrey", 119, 136, 153},
	{"lightsteelblue", 176, 196, 222},
	{"lightsteelblue1", 202, 225, 255},
	{"lightsteelblue2", 188, 210, 238},
	{"lightsteelblue3", 162, 181, 205},
	{"lightsteelblue4", 110, 123, 139},
	{"lightyellow", 255, 255, 224},
	{"lightyellow1", 255, 255, 224},
	{"lightyellow2", 238, 238, 209},
	{"lightyellow3", 205, 205, 180},
	{"lightyellow4", 139, 139, 122},
	{"limegreen", 50, 205, 50},
	{"linen", 250, 240, 230},
	{"magenta", 255, 0, 255},
	{"magenta1", 255, 0, 255},
	{"magenta2", 238, 0, 238},
	{"magenta3", 205, 0, 205},
	{"magenta4", 139, 0, 139},
	{"maroon", 176, 48, 96},
	{"maroon1", 255, 52, 179},
	{"maroon2", 238, 48, 167},
	{"maroon3", 205, 41, 144},
	{"maroon4", 139, 28, 98},
	{"mediumaquamarine", 102, 205, 170},
	{"mediumblue", 0, 0, 205},
	{"mediumorchid", 186, 85, 211},
	{"mediumorchid1", 224, 102, 255},
	{"mediumorchid2", 209, 95, 238},
	{"mediumorchid3", 180, 82, 205},
	{"mediumorchid4", 122, 55, 139},
	{"mediumpurple", 147, 112, 219},
	{"mediumpurple1", 171, 130, 255},
	{"mediumpurple2", 159, 121, 238},
	{"mediumpurple3", 137, 104, 205},
	{"mediumpurple4", 93, 71, 139},
	{"mediumseagreen", 60, 179, 113},
	{"mediumslateblue", 123, 104, 238},
	{"mediumspringgreen", 0, 250, 154},
	{"mediumturquoise", 72, 209, 204},
	{"mediumvioletred", 199, 21, 133},
	{"midnightblue", 25, 25, 112},
	{"mintcream", 245, 255, 250},
	{"mistyrose", 255, 228, 225},
	{"mistyrose1", 255, 228, 225},
	{"mistyrose2", 238, 213, 210},
	{"mistyrose3", 205, 183, 181},
	{"mistyrose4", 139, 125, 123},
	{"moccasin", 255, 228, 181},
	{"navajowhite", 255, 222, 173},
	{"navajowhite1", 255, 222, 173},
	{"navajowhite2", 238, 207, 161},
	{"navajowhite3", 205, 179, 139},
	{"navajowhite4", 139, 121, 94},
	{"navy", 0, 0, 128},
	{"navyblue", 0, 0, 128},
	{"oldlace", 253, 245, 230},
	{"olivedrab", 107, 142, 35},
	{"olivedrab1", 192, 255, 62},
	{"olivedrab2", 179, 238, 58},
	{"olivedrab3", 154, 205, 50},
	{"olivedrab4", 105, 139, 34},
	{"orange", 255, 165, 0},
	{"orange1", 255, 165, 0},
	{"orange2", 238, 154, 0},
	{"orange3", 205, 133, 0},
	{"orange4", 139, 90, 0},
	{"orangered", 255, 69, 0},
	{"orangered1", 255, 69, 0},
	{"orangered2", 238, 64, 0},
	{"orangered3", 205, 55, 0},
	{"orangered4", 139, 37, 0},
	{"orchid", 218, 112, 214},
	{"orchid1", 255, 131, 250},
	{"orchid2", 238, 122, 233},
	{"orchid3", 205, 105, 201},
	{"orchid4", 139, 71, 137},
	{"palegoldenrod", 238, 232, 170},
	{"palegreen", 152, 251, 152},
	{"palegreen1", 154, 255, 154},
	{"palegreen2", 144, 238, 144},
	{"palegreen3", 124, 205, 124},
	{"palegreen4", 84, 139, 84},
	{"paleturquoise", 175, 238, 238},
	{"paleturquoise1", 187, 255, 255},
	{"paleturquoise2", 174, 238, 238},
	{"paleturquoise3", 150, 205, 205},
	{"paleturquoise4", 102, 139, 139},
	{"palevioletred", 219, 112, 147},
	{"palevioletred1", 255, 130, 171},
	{"palevioletred2", 238, 121, 159},
	{"palevioletred3", 205, 104, 137},
	{"palevioletred4", 139, 71, 93},
	{"papayawhip", 255, 239, 213},
	{"peachpuff", 255, 218, 185},
	{"peachpuff1", 255, 218, 185},
	{"peachpuff2", 238, 203, 173},
	{"peachpuff3", 205, 175, 149},
	{"peachpuff4", 139, 119, 101},
	{"peru", 205, 133, 63},
	{"pink", 255, 192, 203},
	{"pink1", 255, 181, 197},
	{"pink2", 238, 169, 184},
	{"pink3", 205, 145, 158},
	{"pink4", 139, 99, 108},
	{"plum", 221, 160, 221},
	{"plum1", 255, 187, 255},
	{"plum2", 238, 174, 238},
	{"plum3", 205, 150, 205},
	{"plum4", 139, 102, 139},
	{"powderblue", 176, 224, 230},
	{"purple", 160, 32, 240},
	{"purple1", 155, 48, 255},
	{"purple2", 145, 44, 238},
	{"purple3", 125, 38, 205},
	{"purple4", 85, 26, 139},
	{"red", 255, 0, 0},
	{"red1", 255, 0, 0},
	{"red2", 238, 0, 0},
	{"red3", 205, 0, 0},
	{"red4", 139, 0, 0},
	{"rosybrown", 188, 143, 143},
	{"rosybrown1", 255, 193, 193},
	{"rosybrown2", 238, 180, 180},
	{"rosybrown3", 205, 155, 155},
	{"rosybrown4", 139, 105, 105},
	{"royalblue", 65, 105, 225},
	{"royalblue1", 72, 118, 255},
	{"royalblue2", 67, 110, 238},
	{"royalblue3", 58, 95, 205},
	{"royalblue4", 39, 64, 139},
	{"saddlebrown", 139, 69, 19},
	{"salmon", 250, 128, 114},
	{"salmon1", 255, 140, 105},
	{"salmon2", 238, 130, 98},
	{"salmon3", 205, 112, 84},
	{"salmon4", 139, 76, 57},
	{"sandybrown", 244, 164, 96},
	{"seagreen", 46, 139, 87},
	{"seagreen1", 84, 255, 159},
	{"seagreen2", 78, 238, 148},
	{"seagreen3", 67, 205, 128},
	{"seagreen4", 46, 139, 87},
	{"seashell", 255, 245, 238},
	{"seashell1", 255, 245, 238},
	{"seashell2", 238, 229, 222},
	{"seashell3", 205, 197, 191},
	{"seashell4", 139, 134, 130},
	{"sienna", 160, 82, 45},
	{"sienna1", 255, 130, 71},
	{"sienna2", 238, 121, 66},
	{"sienna3", 205, 104, 57},
	{"sienna4", 139, 71, 38},
	{"skyblue", 135, 206, 235},
	{"skyblue1", 135, 206, 255},
	{"skyblue2", 126, 192, 238},
	{"skyblue3", 108, 166, 205},
	{"skyblue4", 74, 112, 139},
	{"slateblue", 106, 90, 205},
	{"slateblue1", 131, 111, 255},
	{"slateblue2", 122, 103, 238},
	{"slateblue3", 105, 89, 205},
	{"slateblue4", 71, 60, 139},
	{"slategray", 112, 128, 144},
	{"slategray1", 198, 226, 255},
	{"slategray2", 185, 211, 238},
	{"slategray3", 159, 182, 205},
	{"slategray4", 108, 123, 139},
	{"slategrey", 112, 128, 144},
	{"snow", 255, 250, 250},
	{"snow1", 255, 250, 250},
	{"snow2", 238, 233, 233},
	{"snow3", 205, 201, 201},
	{"snow4", 139, 137, 137},
	{"springgreen", 0, 255, 127},
	{"springgreen1", 0, 255, 127},
	{"springgreen2", 0, 238, 118},
	{"springgreen3", 0, 205, 102},
	{"springgreen4", 0, 139, 69},
	{"steelblue", 70, 130, 180},
	{"steelblue1", 99, 184, 255},
	{"steelblue2", 92, 172, 238},
	{"steelblue3", 79, 148, 205},
	{"steelblue4", 54, 100, 139},
	{"tan", 210, 180, 140},
	{"tan1", 255, 165, 79},
	{"tan2", 238, 154, 73},
	{"tan3", 205, 133, 63},
	{"tan4", 139, 90, 43},
	{"thistle", 216, 191, 216},
	{"thistle1", 255, 225, 255},
	{"thistle2", 238, 210, 238},
	{"thistle3", 205, 181, 205},
	{"thistle4", 139, 123, 139},
	{"tomato", 255, 99, 71},
	{"tomato1", 255, 99, 71},
	{"tomato2", 238, 92, 66},
	{"tomato3", 205, 79, 57},
	{"tomato4", 139, 54, 38},
	{"turquoise", 64, 224, 208},
	{"turquoise1", 0, 245, 255},
	{"turquoise2", 0, 229, 238},
	{"turquoise3", 0, 197, 205},
	{"turquoise4", 0, 134, 139},
	{"violet", 238, 130, 238},
	{"violetred", 208, 32, 144},
	{"violetred1", 255, 62, 150},
	{"violetred2", 238, 58, 140},
	{"violetred3", 205, 50, 120},
	{"violetred4", 139, 34, 82},
	{"wheat", 245, 222, 179},
	{"wheat1", 255, 231, 186},
	{"wheat2", 238, 216, 174},
	{"wheat3", 205, 186, 150},
	{"wheat4", 139, 126, 102},
	{"whitesmoke", 245, 245, 245},
	{"yellow", 255, 255, 0},
	{"yellow1", 255, 255, 0},
	{"yellow2", 238, 238, 0},
	{"yellow3", 205, 205, 0},
	{"yellow4", 139, 139, 0},
	{"yellowgreen", 154, 205, 50},
	{nullptr, 0, 0, 0}
};

void EidosGetColorComponents(std::string &p_color_name, float *p_red_component, float *p_green_component, float *p_blue_component)
{
	// Colors can be specified either in hex as "#RRGGBB" or as a named color from the list above
	if ((p_color_name.length() == 7) && (p_color_name[0] == '#'))
	{
		try
		{
			unsigned long r = stoul(p_color_name.substr(1, 2), nullptr, 16);
			unsigned long g = stoul(p_color_name.substr(3, 2), nullptr, 16);
			unsigned long b = stoul(p_color_name.substr(5, 2), nullptr, 16);
			
			*p_red_component = r / 255.0f;
			*p_green_component = g / 255.0f;
			*p_blue_component = b / 255.0f;
			return;
		}
		catch (...)
		{
			EIDOS_TERMINATION << "ERROR (EidosGetColorComponents): color specification \"" << p_color_name << "\" is malformed." << eidos_terminate();
		}
	}
	else
	{
		for (EidosNamedColor *color_table = gEidosNamedColors; color_table->name; ++color_table)
		{
			if (p_color_name == color_table->name)
			{
				*p_red_component = color_table->red / 255.0f;
				*p_green_component = color_table->green / 255.0f;
				*p_blue_component = color_table->blue / 255.0f;
				return;
			}
		}
	}
	
	EIDOS_TERMINATION << "ERROR (EidosGetColorComponents): color named \"" << p_color_name << "\" could not be found." << eidos_terminate();
}


















































