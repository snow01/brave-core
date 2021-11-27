#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

// ALL DISALLOW_xxx MACROS ARE DEPRECATED; DO NOT USE IN NEW CODE.
// Use explicit deletions instead.  See the section on copyability/movability in
// //styleguide/c++/c++-dos-and-donts.md for more information.

// DEPRECATED: See above. Makes a class uncopyable.
#define DISALLOW_COPY(TypeName) \
  TypeName(const TypeName&) = delete

// DEPRECATED: See above. Makes a class unassignable.
#define DISALLOW_ASSIGN(TypeName) TypeName& operator=(const TypeName&) = delete

// DEPRECATED: See above. Makes a class uncopyable and unassignable.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName);                 \
  DISALLOW_ASSIGN(TypeName)

// DEPRECATED: See above. Disallow all implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;                           \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

#endif  // BASE_MACROS_H_
