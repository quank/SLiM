//
//  eidos_symbol_table.cpp
//  Eidos
//
//  Created by Ben Haller on 4/12/15.
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


#include "eidos_symbol_table.h"
#include "eidos_value.h"
#include "eidos_global.h"
#include "eidos_token.h"
#include "eidos_type_table.h"

#include <algorithm>
#include <utility>
#include <limits>
#include <cmath>


using std::string;
using std::vector;
using std::endl;
using std::istringstream;
using std::istream;
using std::ostream;


//
//	EidosSymbolTable
//
#pragma mark EidosSymbolTable

EidosSymbolTable::EidosSymbolTable(EidosSymbolTableType p_table_type, EidosSymbolTable *p_parent_table, bool p_start_with_hash)
{
	// Set up the symbol table itself
	table_type_ = p_table_type;
	using_internal_symbols_ = !p_start_with_hash;
	internal_symbol_count_ = 0;
	
	if (!p_parent_table)
	{
		// If no parent table is given, then we construct a base table for Eidos containing the standard constants.
		// We statically allocate our base symbols for fast setup / teardown.
#ifdef DEBUG
		if (table_type_ != EidosSymbolTableType::kEidosIntrinsicConstantsTable)
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::EidosSymbolTable): (internal error) symbol tables must have a parent table, except the Eidos intrinsic constants table." << eidos_terminate(nullptr);
#endif
		
		static EidosSymbolTableEntry *trueConstant = nullptr;
		static EidosSymbolTableEntry *falseConstant = nullptr;
		static EidosSymbolTableEntry *nullConstant = nullptr;
		static EidosSymbolTableEntry *piConstant = nullptr;
		static EidosSymbolTableEntry *eConstant = nullptr;
		static EidosSymbolTableEntry *infConstant = nullptr;
		static EidosSymbolTableEntry *nanConstant = nullptr;
		
		if (!trueConstant)
		{
			trueConstant = new EidosSymbolTableEntry(gEidosID_T, gStaticEidosValue_LogicalT);
			falseConstant = new EidosSymbolTableEntry(gEidosID_F, gStaticEidosValue_LogicalF);
			nullConstant = new EidosSymbolTableEntry(gEidosID_NULL, gStaticEidosValueNULL);
			piConstant = new EidosSymbolTableEntry(gEidosID_PI, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(M_PI)));
			eConstant = new EidosSymbolTableEntry(gEidosID_E, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(M_E)));
			infConstant = new EidosSymbolTableEntry(gEidosID_INF, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(std::numeric_limits<double>::infinity())));
			nanConstant = new EidosSymbolTableEntry(gEidosID_NAN, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(std::numeric_limits<double>::quiet_NaN())));
		}
		
		// We can use InitializeConstantSymbolEntry() here since we obey its requirements (see header)
		InitializeConstantSymbolEntry(*nanConstant);
		InitializeConstantSymbolEntry(*infConstant);
		InitializeConstantSymbolEntry(*piConstant);
		InitializeConstantSymbolEntry(*eConstant);
		InitializeConstantSymbolEntry(*nullConstant);
		InitializeConstantSymbolEntry(*falseConstant);
		InitializeConstantSymbolEntry(*trueConstant);
	}
	else
	{
		// If a parent table is given, we adopt it and do not add Eidos constants; they will be in the parent chain
		parent_symbol_table_ = p_parent_table;
		
#ifdef DEBUG
		if (table_type_ == EidosSymbolTableType::kEidosIntrinsicConstantsTable)
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::EidosSymbolTable): (internal error) the Eidos intrinsic constants table cannot have a parent." << eidos_terminate(nullptr);
		if (parent_symbol_table_->table_type_ == EidosSymbolTableType::kVariablesTable)
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::EidosSymbolTable): (internal error) parent symbol tables must be constant in the current design." << eidos_terminate(nullptr);
#endif
	}
}

EidosSymbolTable::~EidosSymbolTable(void)
{
}

std::vector<std::string> EidosSymbolTable::_SymbolNames(bool p_include_constants, bool p_include_variables) const
{
	std::vector<std::string> symbol_names;
	
	// start with the symbols from our parent symbol table
	if (parent_symbol_table_)
		symbol_names = parent_symbol_table_->_SymbolNames(p_include_constants, p_include_variables);
	
	if ((p_include_constants && (table_type_ != EidosSymbolTableType::kVariablesTable)) ||
		(p_include_variables && (table_type_ == EidosSymbolTableType::kVariablesTable)))
	{
		if (using_internal_symbols_)
		{
			for (size_t symbol_index = 0; symbol_index < internal_symbol_count_; ++symbol_index)
				symbol_names.emplace_back(StringForEidosGlobalStringID(internal_symbols_[symbol_index].symbol_name_));
		}
		else
		{
			for (auto symbol_slot_iter = hash_symbols_.begin(); symbol_slot_iter != hash_symbols_.end(); ++symbol_slot_iter)
				symbol_names.emplace_back(StringForEidosGlobalStringID(symbol_slot_iter->first));
		}
	}
	
	return symbol_names;
}

bool EidosSymbolTable::ContainsSymbol(EidosGlobalStringID p_symbol_name) const
{
	if (using_internal_symbols_)
	{
		// We can compare global string IDs; since all symbol names should be uniqued, this should be safe
		for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
			if (internal_symbols_[symbol_index].symbol_name_ == p_symbol_name)
				return true;
	}
	else
	{
		if (hash_symbols_.find(p_symbol_name) != hash_symbols_.end())
			return true;
	}
	
	// We didn't get a hit, so try our parent table
	if (parent_symbol_table_)
		return parent_symbol_table_->ContainsSymbol(p_symbol_name);
	
	return false;
}

EidosValue_SP EidosSymbolTable::_GetValue(EidosGlobalStringID p_symbol_name, const EidosToken *p_symbol_token) const
{
	if (using_internal_symbols_)
	{
		// We can compare global string IDs; since all symbol names should be uniqued, this should be safe
		for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
		{
			const EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
			
			if (symbol_slot->symbol_name_ == p_symbol_name)
				return symbol_slot->symbol_value_SP_;
		}
	}
	else
	{
		auto symbol_slot_iter = hash_symbols_.find(p_symbol_name);
		
		if (symbol_slot_iter != hash_symbols_.end())
			return symbol_slot_iter->second;
	}
	
	// We didn't get a hit, so try our parent table
	if (parent_symbol_table_)
		return parent_symbol_table_->_GetValue(p_symbol_name, p_symbol_token);
	
	EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_GetValue): undefined identifier " << StringForEidosGlobalStringID(p_symbol_name) << "." << eidos_terminate(p_symbol_token);
}

// same as above except for handling the p_is_const flag
EidosValue_SP EidosSymbolTable::_GetValue_IsConst(EidosGlobalStringID p_symbol_name, const EidosToken *p_symbol_token, bool *p_is_const) const
{
	if (using_internal_symbols_)
	{
		// We can compare global string IDs; since all symbol names should be uniqued, this should be safe
		for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
		{
			const EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
			
			if (symbol_slot->symbol_name_ == p_symbol_name)
			{
				*p_is_const = (table_type_ != EidosSymbolTableType::kVariablesTable);
				return symbol_slot->symbol_value_SP_;
			}
		}
	}
	else
	{
		auto symbol_slot_iter = hash_symbols_.find(p_symbol_name);
		
		if (symbol_slot_iter != hash_symbols_.end())
		{
			*p_is_const = (table_type_ != EidosSymbolTableType::kVariablesTable);
			return symbol_slot_iter->second;
		}
	}
	
	// We didn't get a hit, so try our parent table
	if (parent_symbol_table_)
		return parent_symbol_table_->_GetValue_IsConst(p_symbol_name, p_symbol_token, p_is_const);	// the parent sets p_is_const
	
	EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_GetValue): undefined identifier " << StringForEidosGlobalStringID(p_symbol_name) << "." << eidos_terminate(p_symbol_token);
}

void EidosSymbolTable::_SwitchToHash(void)
{
	if (using_internal_symbols_)
	{
		for (size_t symbol_index = 0; symbol_index < internal_symbol_count_; ++symbol_index)
		{
			EidosSymbolTable_InternalSlot &old_symbol_slot = internal_symbols_[symbol_index];
			
			// if we own the symbol name, we can move it to the new hash table entry, otherwise a copy is necessary
			hash_symbols_.insert(std::pair<EidosGlobalStringID, EidosValue_SP>(old_symbol_slot.symbol_name_, std::move(old_symbol_slot.symbol_value_SP_)));
			
			// clean up the built-in slot; probably mostly unnecessary, but prevents hard-to-find bugs
			old_symbol_slot.symbol_value_SP_.reset();
		}
		
		using_internal_symbols_ = false;
		internal_symbol_count_ = 0;
	}
}

void EidosSymbolTable::SetValueForSymbol(EidosGlobalStringID p_symbol_name, EidosValue_SP p_value)
{
	// If we have the only reference to the value, we don't need to copy it; otherwise we copy, since we don't want to hold
	// onto a reference that somebody else might modify under us (or that we might modify under them, with syntaxes like
	// x[2]=...; and x=x+1;). If the value is invisible then we copy it, since the symbol table never stores invisible values.
	if ((p_value->use_count() != 1) || p_value->Invisible())
		p_value = p_value->CopyValues();
	
	if (using_internal_symbols_)
	{
		int symbol_slot;
		
		// We can compare global string IDs; since all symbol names should be uniqued, this should be safe
		for (symbol_slot = (int)internal_symbol_count_ - 1; symbol_slot >= 0; --symbol_slot)
			if (internal_symbols_[symbol_slot].symbol_name_ == p_symbol_name)
				break;
		
		if (symbol_slot == -1)
		{
			// The symbol is not already defined in this table.  Before we can define it, we need to check that it is not defined in a parent table.
			// At present, we assume that if it is defined in a parent table it is a constant, which is true for now.
			if (parent_symbol_table_ && parent_symbol_table_->ContainsSymbol(p_symbol_name))
				EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetValueForSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' cannot be redefined because it is a constant." << eidos_terminate(nullptr);
			
			if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
			{
				EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
				
				new_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
				new_symbol_slot_ptr->symbol_name_ = p_symbol_name;
				
				++internal_symbol_count_;
				return;
			}
			
			_SwitchToHash();
		}
		else
		{
			// The symbol is already defined in this table, so it can be redefined (illegal cases were caught above already).
			// We replace the existing symbol value, of course.  Everything else gets inherited, since we're replacing the value in an existing slot;
			// we can continue using the same symbol name, name length, constness (since that is guaranteed to be false here), etc.
			internal_symbols_[symbol_slot].symbol_value_SP_ = std::move(p_value);
			return;
		}
	}
	
	// fall-through to the hash table case
	auto existing_symbol_slot_iter = hash_symbols_.find(p_symbol_name);
	
	if (existing_symbol_slot_iter == hash_symbols_.end())
	{
		// The symbol is not already defined in this table.  Before we can define it, we need to check that it is not defined in a parent table.
		// At present, we assume that if it is defined in a parent table it is a constant, which is true for now.
		if (parent_symbol_table_ && parent_symbol_table_->ContainsSymbol(p_symbol_name))
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetValueForSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' cannot be redefined because it is a constant." << eidos_terminate(nullptr);
		
		hash_symbols_.insert(std::pair<EidosGlobalStringID, EidosValue_SP>(p_symbol_name, std::move(p_value)));
	}
	else
	{
		existing_symbol_slot_iter->second = std::move(p_value);
	}
}

void EidosSymbolTable::SetValueForSymbolNoCopy(EidosGlobalStringID p_symbol_name, EidosValue_SP p_value)
{
	// So, this is a little weird.  SetValueForSymbol() copies the passed value, as explained in its comment above.
	// If a few cases, however, we want to play funny games and prevent that copy from occurring so that we can munge
	// values directly inside a value we just set in the symbol table.  Evaluate_For() is the worst offender in this
	// because it wants to set up an index variable once and then munge its value directly each time through the loop,
	// for speed.  _ProcessSubscriptAssignment() also does it in one case, where it needs to change a singleton into a
	// vector value so that it can do a subscripted assignment.  For that special purpose, this function is provided.
	// DO NOT USE THIS UNLESS YOU KNOW WHAT YOU'RE DOING!  It can lead to seriously weird behavior if used incorrectly.
	if (p_value->Invisible())
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetValueForSymbol): (internal) no copy requested with invisible value." << eidos_terminate(nullptr);
	
	if (using_internal_symbols_)
	{
		int symbol_slot;
		
		// We can compare global string IDs; since all symbol names should be uniqued, this should be safe
		for (symbol_slot = (int)internal_symbol_count_ - 1; symbol_slot >= 0; --symbol_slot)
			if (internal_symbols_[symbol_slot].symbol_name_ == p_symbol_name)
				break;
		
		if (symbol_slot == -1)
		{
			// The symbol is not already defined in this table.  Before we can define it, we need to check that it is not defined in a parent table.
			// At present, we assume that if it is defined in a parent table it is a constant, which is true for now.
			if (parent_symbol_table_ && parent_symbol_table_->ContainsSymbol(p_symbol_name))
				EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetValueForSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' cannot be redefined because it is a constant." << eidos_terminate(nullptr);
			
			if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
			{
				EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
				
				new_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
				new_symbol_slot_ptr->symbol_name_ = p_symbol_name;
				
				++internal_symbol_count_;
				return;
			}
			
			_SwitchToHash();
		}
		else
		{
			// The symbol is already defined in this table, so it can be redefined (illegal cases were caught above already).
			// We replace the existing symbol value, of course.  Everything else gets inherited, since we're replacing the value in an existing slot;
			// we can continue using the same symbol name, name length, constness (since that is guaranteed to be false here), etc.
			internal_symbols_[symbol_slot].symbol_value_SP_ = std::move(p_value);
			return;
		}
	}
	
	// fall-through to the hash table case
	auto existing_symbol_slot_iter = hash_symbols_.find(p_symbol_name);
	
	if (existing_symbol_slot_iter == hash_symbols_.end())
	{
		// The symbol is not already defined in this table.  Before we can define it, we need to check that it is not defined in a parent table.
		// At present, we assume that if it is defined in a parent table it is a constant, which is true for now.
		if (parent_symbol_table_ && parent_symbol_table_->ContainsSymbol(p_symbol_name))
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetValueForSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' cannot be redefined because it is a constant." << eidos_terminate(nullptr);
		
		hash_symbols_.insert(std::pair<EidosGlobalStringID, EidosValue_SP>(p_symbol_name, std::move(p_value)));
	}
	else
	{
		existing_symbol_slot_iter->second = std::move(p_value);
	}
}

void EidosSymbolTable::DefineConstantForSymbol(EidosGlobalStringID p_symbol_name, EidosValue_SP p_value)
{
	// First make sure this symbol is not in use as either a variable or a constant
	if (ContainsSymbol(p_symbol_name))
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::DefineConstantForSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' is already defined." << eidos_terminate(nullptr);
	
	// Search through our parent chain for a defined constants table; if we don't find one, add one
	EidosSymbolTable *definedConstantsTable;
	
	for (definedConstantsTable = this; definedConstantsTable != nullptr; definedConstantsTable = definedConstantsTable->parent_symbol_table_)
		if (definedConstantsTable->table_type_ == EidosSymbolTableType::kEidosDefinedConstantsTable)
			break;
	
	if (!definedConstantsTable)
	{
		EidosSymbolTable *childTable;
		
		for (childTable = this; childTable != nullptr; childTable = childTable->parent_symbol_table_)
			if (childTable->parent_symbol_table_ && childTable->parent_symbol_table_->table_type_ == EidosSymbolTableType::kEidosIntrinsicConstantsTable)
				break;
		
		if (!childTable)
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::DefineConstantForSymbol): (internal) could not find child symbol table of the intrinsic constants table." << eidos_terminate(nullptr);
		
		EidosSymbolTable *intrinsicConstantsTable = childTable->parent_symbol_table_;
		
		// Make a defined constants table and insert it in between
		definedConstantsTable = new EidosSymbolTable(EidosSymbolTableType::kEidosDefinedConstantsTable, intrinsicConstantsTable);
		childTable->parent_symbol_table_ = definedConstantsTable;
	}
	
	// If we have the only reference to the value, we don't need to copy it; otherwise we copy, since we don't want to hold
	// onto a reference that somebody else might modify under us (or that we might modify under them, with syntaxes like
	// x[2]=...; and x=x+1;). If the value is invisible then we copy it, since the symbol table never stores invisible values.
	if ((p_value->use_count() != 1) || p_value->Invisible())
		p_value = p_value->CopyValues();
	
	// Then ask the defined constants table to add the constant
	definedConstantsTable->InitializeConstantSymbolEntry(p_symbol_name, std::move(p_value));
}

void EidosSymbolTable::_RemoveSymbol(EidosGlobalStringID p_symbol_name, bool p_remove_constant)
{
	if (using_internal_symbols_)
	{
		int symbol_slot;
		
		// We can compare global string IDs; since all symbol names should be uniqued, this should be safe
		for (symbol_slot = (int)internal_symbol_count_ - 1; symbol_slot >= 0; --symbol_slot)
			if (internal_symbols_[symbol_slot].symbol_name_ == p_symbol_name)
				break;
		
		if (symbol_slot >= 0)
		{
			if (table_type_ != EidosSymbolTableType::kVariablesTable)
			{
				if (table_type_ == EidosSymbolTableType::kEidosIntrinsicConstantsTable)
					EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_RemoveSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' is an intrinsic Eidos constant and thus cannot be removed." << eidos_terminate(nullptr);
				if (!p_remove_constant)
					EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_RemoveSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' is a constant and thus cannot be removed." << eidos_terminate(nullptr);
			}
			
			EidosSymbolTable_InternalSlot *existing_symbol_slot_ptr = internal_symbols_ + symbol_slot;
			
			// clean up the slot being removed
			existing_symbol_slot_ptr->symbol_value_SP_.reset();
			
			// remove the slot from the vector
			EidosSymbolTable_InternalSlot *backfill_symbol_slot_ptr = internal_symbols_ + (--internal_symbol_count_);
			
			if (existing_symbol_slot_ptr != backfill_symbol_slot_ptr)
			{
				*existing_symbol_slot_ptr = std::move(*backfill_symbol_slot_ptr);
				
				// clean up the backfill slot; probably unnecessary, but prevents hard-to-find bugs
				backfill_symbol_slot_ptr->symbol_value_SP_.reset();
			}
		}
		else
		{
			// If it wasn't defined in us, then it might be defined in a parent
			if (parent_symbol_table_)
				parent_symbol_table_->_RemoveSymbol(p_symbol_name, p_remove_constant);
		}
	}
	else
	{
		auto remove_iter = hash_symbols_.find(p_symbol_name);
		
		if (remove_iter != hash_symbols_.end())
		{
			// We found the symbol in ourselves, so remove it unless we are a constant table
			if (table_type_ != EidosSymbolTableType::kVariablesTable)
			{
				if (table_type_ == EidosSymbolTableType::kEidosIntrinsicConstantsTable)
					EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_RemoveSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' is an intrinsic Eidos constant and thus cannot be removed." << eidos_terminate(nullptr);
				if (!p_remove_constant)
					EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_RemoveSymbol): identifier '" << StringForEidosGlobalStringID(p_symbol_name) << "' is a constant and thus cannot be removed." << eidos_terminate(nullptr);
			}
			
			hash_symbols_.erase(remove_iter);
		}
		else
		{
			// If it wasn't defined in us, then it might be defined in a parent
			if (parent_symbol_table_)
				parent_symbol_table_->_RemoveSymbol(p_symbol_name, p_remove_constant);
		}
	}
}

void EidosSymbolTable::_InitializeConstantSymbolEntry(EidosGlobalStringID p_symbol_name, EidosValue_SP p_value)
{
#ifdef DEBUG
	if (p_value->Invisible())
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_InitializeConstantSymbolEntry): (internal error) this method should be called only for non-invisible objects." << eidos_terminate(nullptr);
	if (table_type_ == EidosSymbolTableType::kVariablesTable)
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::_InitializeConstantSymbolEntry): (internal error) this method should be called only on constant symbol tables." << eidos_terminate(nullptr);
#endif
	
	// we assume that this symbol is not yet defined, for maximal set-up speed
	if (using_internal_symbols_)
	{
		if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
		{
			EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
			
			new_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
			new_symbol_slot_ptr->symbol_name_ = p_symbol_name;
			
			++internal_symbol_count_;
			return;
		}
		
		_SwitchToHash();
	}
	
	// fall-through to the hash table case
	hash_symbols_.insert(std::pair<EidosGlobalStringID, EidosValue_SP>(p_symbol_name, std::move(p_value)));
}

void EidosSymbolTable::AddSymbolsToTypeTable(EidosTypeTable *p_type_table) const
{
	// start with the symbols from our parent symbol table
	if (parent_symbol_table_)
		parent_symbol_table_->AddSymbolsToTypeTable(p_type_table);
	
	if (using_internal_symbols_)
	{
		for (size_t symbol_index = 0; symbol_index < internal_symbol_count_; ++symbol_index)
		{
			EidosValue *symbol_value = internal_symbols_[symbol_index].symbol_value_SP_.get();
			EidosValueType symbol_type = symbol_value->Type();
			EidosValueMask symbol_type_mask = (1 << (int)symbol_type);
			const EidosObjectClass *symbol_class = ((symbol_type == EidosValueType::kValueObject) ? ((EidosValue_Object *)symbol_value)->Class() : nullptr);
			EidosTypeSpecifier symbol_type_specifier = EidosTypeSpecifier{symbol_type_mask, symbol_class};
			
			p_type_table->SetTypeForSymbol(internal_symbols_[symbol_index].symbol_name_, symbol_type_specifier);
		}
	}
	else
	{
		for (auto symbol_slot_iter = hash_symbols_.begin(); symbol_slot_iter != hash_symbols_.end(); ++symbol_slot_iter)
		{
			EidosValue *symbol_value = symbol_slot_iter->second.get();
			EidosValueType symbol_type = symbol_value->Type();
			EidosValueMask symbol_type_mask = (1 << (int)symbol_type);
			const EidosObjectClass *symbol_class = ((symbol_type == EidosValueType::kValueObject) ? ((EidosValue_Object *)symbol_value)->Class() : nullptr);
			EidosTypeSpecifier symbol_type_specifier = EidosTypeSpecifier{symbol_type_mask, symbol_class};
			
			p_type_table->SetTypeForSymbol(symbol_slot_iter->first, symbol_type_specifier);
		}
	}
}

std::ostream &operator<<(std::ostream &p_outstream, const EidosSymbolTable &p_symbols)
{
	std::vector<std::string> read_only_symbol_names = p_symbols.ReadOnlySymbols();
	std::vector<std::string> read_write_symbol_names = p_symbols.ReadWriteSymbols();
	std::vector<std::string> symbol_names;
	
	symbol_names.insert(symbol_names.end(), read_only_symbol_names.begin(), read_only_symbol_names.end());
	symbol_names.insert(symbol_names.end(), read_write_symbol_names.begin(), read_write_symbol_names.end());
	std::sort(symbol_names.begin(), symbol_names.end());
	
	for (auto symbol_name_iter = symbol_names.begin(); symbol_name_iter != symbol_names.end(); ++symbol_name_iter)
	{
		const std::string &symbol_name = *symbol_name_iter;
		EidosValue_SP symbol_value = p_symbols.GetValueOrRaiseForSymbol(EidosGlobalStringIDForString(symbol_name));
		int symbol_count = symbol_value->Count();
		bool is_const = std::find(read_only_symbol_names.begin(), read_only_symbol_names.end(), symbol_name) != read_only_symbol_names.end();
		
		if (symbol_count <= 2)
			p_outstream << symbol_name << (is_const ? " => (" : " -> (") << symbol_value->Type() << ") " << *symbol_value << endl;
		else
		{
			EidosValue_SP first_value = symbol_value->GetValueAtIndex(0, nullptr);
			EidosValue_SP second_value = symbol_value->GetValueAtIndex(1, nullptr);
			
			p_outstream << symbol_name << (is_const ? " => (" : " -> (") << symbol_value->Type() << ") " << *first_value << " " << *second_value << " ... (" << symbol_count << " values)" << endl;
		}
	}
	
	return p_outstream;
}




































































