/*
 * Copyright (c) 2015-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef SCOPE_H_
#define SCOPE_H_

#include "gbldefs.h"
#include "symtab.h"

/**
   \file
   Functions for dealing with lexical scopes and the lifetimes of
   scoped variables.
 */

#if DEBUG + 0

#define ICHECK(x)                                       \
  assert((x), "CHECK(" #x "): false at " __FILE__ ":",  \
         __LINE__, ERR_Informational)

#else

#define ICHECK(x)

#endif

/*
 * Scope tracking.
 *
 * During inlining, as we're scanning the current function ILMs top to bottom,
 * call track_scope_label(label_sptr) whenever an IM_LABEL is seen in order to
 * keep track of the current scope.
 *
 * The global variable current_scope points to the currently open scope.
 */
extern int current_scope;

/**
   \brief ...
 */
bool scope_contains(SPTR outer, SPTR inner);

/**
   \brief ...
 */
int insert_begin_scope_label(int block_sptr);

/**
   \brief ...
 */
int insert_end_scope_label(int block_sptr);

/**
   \brief ...
 */
bool is_scope_label_ili(int ilix);

/**
   \brief ...
 */
bool is_scope_label(int label_sptr);

/**
   \brief ...
 */
bool is_scope_labels_only_bih(int bihx);

/**
   \brief ...
 */
void scope_verify(void);

/*
 * Inliner support.
 *
 * The functions and global variables below are used by the C and Fortran
 * inliners.
 */

/*
 * While inlining a callee (see func_sptr below), this is an sptr to a new
 * ST_BLOCK representing the callee function scope.
 */
extern int new_callee_scope;

/**
   \brief ...
 */
void begin_inlined_scope(int func_sptr);

/**
   \brief ...
 */
void cancel_inlined_scope(void);

/**
   \brief ...
 */
void create_inlined_scope(int callee_sptr);

/**
   \brief ...
 */
void end_inlined_scopes(int new_open_count);

/**
   \brief ...
 */
void end_inlined_scope(void);


/**
   \brief ...
 */
void find_scope_labels(int numilms);

/**
   \brief ...
 */
void remove_scope_labels(void);

/**
   \brief ...
 */
void reset_new_callee_scope(void);

/**
   \brief ...
 */
void track_scope_label(int label);

/**
   \brief ...
 */
void track_scope_reset(void);


#endif /* SCOPE_H_ */
