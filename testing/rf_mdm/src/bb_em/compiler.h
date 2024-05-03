/**
 ****************************************************************************************
 *
 * @file rvds/compiler.h
 *
 * @brief Definitions of compiler specific directives.
 ****************************************************************************************
 */

#ifndef _COMPILER_H_
#define _COMPILER_H_

/*
 * TYPE MACRO
 ****************************************************************************************
 */

/// define the static keyword for this compiler
#define __STATIC static
#define __CONST  const

/// define the force inlining attribute for this compiler
#define __INLINE__ __forceinline static
    
/// define size of an empty array (used to declare structure with an array size not defined)
#define __ARRAY_EMPTY

#endif // _COMPILER_H_
