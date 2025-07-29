#include <string.h>
// expansion macro for enum value definition
#define ENUM_VALUE(name,value) name value,

// expansion macro for enum to string conversion
#define ENUM_CASE(name,value) case name: return #name;

// expansion macro for string to enum conversion
#define ENUM_STRCMP(name,value) if (!strcmp(str,#name)) return name;

#define ENUM_CASE_CHK(name, value) case name:

/// Declare the access function and define enum values type
#define DECL_ENUM(EnumType,EnumList) \
	enum EnumType { \
    EnumList(ENUM_VALUE) \
  }; \
  const char *Get##EnumType##String(enum EnumType dummy); \
  enum EnumType Get##EnumType##Value(const char *string); \
  int Check##EnumType(enum EnumType value); \
/// Define the access function names
#define DEF_ENUM(EnumType,EnumList) \
  const char *Get##EnumType##String(enum EnumType value) \
  { \
    switch(value) \
    { \
      EnumList(ENUM_CASE) \
      default: return "Unknown Enum Value."; /* handle input error */ \
    } \
  } \
  enum EnumType Get##EnumType##Value(const char *str) \
  { \
    EnumList(ENUM_STRCMP) \
    return (enum EnumType)0; /* handle input error */ \
  } \
  int Check##EnumType(enum EnumType value) { \
	int rst = -1; \
	switch(value) { \
	  EnumList(ENUM_CASE_CHK) \
	  rst =1; \
	  break;\
	default: \
		rst= 0;\
	}  \
	return rst;\
  }
