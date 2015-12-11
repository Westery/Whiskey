#ifndef VALUE_H_
# define VALUE_H_

#include "type.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

struct wsky_Object_s;
typedef struct wsky_Object_s wsky_Object;

struct wsky_Value_s;
typedef struct wsky_Value_s wsky_Value;

struct wsky_Value_s {
  wsky_Type type;
  union {
    bool boolValue;
    int64_t intValue;
    double floatValue;
    wsky_Object *objectValue;
  } v;
};


extern const wsky_Value wsky_Value_TRUE;
extern const wsky_Value wsky_Value_FALSE;
extern const wsky_Value wsky_Value_NULL;
extern const wsky_Value wsky_Value_ZERO;

wsky_Value wsky_Value_fromBool(bool n);
wsky_Value wsky_Value_fromObject(wsky_Object *object);
wsky_Value wsky_Value_fromInt(int64_t n);
wsky_Value wsky_Value_fromFloat(double n);

bool wsky_Value_isNull(const wsky_Value value);

wsky_Value wsky_vaBuildValue(const char *format, va_list parameters);
wsky_Value wsky_buildValue(const char *format, ...);
int wsky_buildValues(wsky_Value *values, const char *format, ...);

int wsky_vaParseValue(wsky_Value value, const char format,
		       va_list parameters);
int wsky_vaParseValues(wsky_Value *values, const char *format,
			va_list parameters);
int wsky_parseValues(wsky_Value *values, const char *format, ...);

#endif /* !VALUE_H_ */
