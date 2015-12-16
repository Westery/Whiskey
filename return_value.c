#include "return_value.h"

#include <stdlib.h>
#include "exception.h"



const wsky_ReturnValue wsky_ReturnValue_TRUE = {
  .v = {
    .type = wsky_Type_BOOL,
    .v = {
      .boolValue = true
    }
  },
  .exception = NULL
};
const wsky_ReturnValue wsky_ReturnValue_FALSE = {
  .v = {
    .type = wsky_Type_BOOL,
    .v = {
      .boolValue = false
    }
  },
  .exception = NULL
};
const wsky_ReturnValue wsky_ReturnValue_NULL = {
  .v = {
    .type = wsky_Type_OBJECT,
    .v = {
      .objectValue = NULL
    }
  },
  .exception = NULL
};
const wsky_ReturnValue wsky_ReturnValue_ZERO = {
  .v = {
    .type = wsky_Type_INT,
    .v = {
      .intValue = 0
    }
  },
  .exception = NULL
};



wsky_ReturnValue wsky_ReturnValue_fromBool(bool n) {
  return n ? wsky_ReturnValue_TRUE : wsky_ReturnValue_FALSE;
}


wsky_ReturnValue wsky_ReturnValue_fromInt(int64_t n) {
  wsky_ReturnValue r = {
    .exception = NULL,
    .v = wsky_Value_fromInt(n)
  };
  return r;
}

wsky_ReturnValue wsky_ReturnValue_fromFloat(double n) {
  wsky_ReturnValue r = {
    .exception = NULL,
    .v = wsky_Value_fromFloat(n)
  };
  return r;
}

wsky_ReturnValue wsky_ReturnValue_fromValue(wsky_Value v) {
  wsky_ReturnValue r = {
    .exception = NULL,
    .v = v,
  };
  return r;
}

wsky_ReturnValue wsky_ReturnValue_fromObject(wsky_Object *object) {
  wsky_ReturnValue r = {
    .exception = NULL,
    .v = wsky_Value_fromObject(object)
  };
  return r;
}

wsky_ReturnValue wsky_ReturnValue_fromException(wsky_Exception *e) {
  wsky_ReturnValue r = {
    .exception = e,
    .v = wsky_Value_NULL
  };
  return r;
}



wsky_ReturnValue wsky_ReturnValue_newException(const char *message) {
  wsky_RETURN_EXCEPTION(wsky_Exception_new(message, NULL));
}
