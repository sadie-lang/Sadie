
#include <stdio.h>

#include <string.h>



#include "object.h"

#include "memory.h"
#include "value.h"

void initValueArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values,
                               oldCapacity, array->capacity);
  }
  
  array->values[array->count] = value;
  array->count++;
}


void freeValueArray(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

char *valueTypeToString(Value value, int *length) {
#define CONVERT(typeString, size)                     \
    do {                                              \
        char *string = ALLOCATE(char, size + 1);  \
        memcpy(string, #typeString, size);            \
        string[size] = '\0';                          \
        *length = size;                               \
        return string;                                \
    } while (false)

#define CONVERT_VARIABLE(typeString, size)            \
    do {                                              \
        char *string = ALLOCATE(char, size + 1);  \
        memcpy(string, typeString, size);             \
        string[size] = '\0';                          \
        *length = size;                               \
        return string;                                \
    } while (false)


  if (IS_BOOL(value)) {
    CONVERT(bool, 4);
  } else if (IS_NIL(value)) {
    CONVERT(nil, 3);
  } else if (IS_NUMBER(value)) {
    CONVERT(number, 6);
  } else if (IS_OBJ(value)) {
    switch (OBJ_TYPE(value)) {
      case OBJ_CLASS: {
        CONVERT(class, 5);

        break;
      }
      case OBJ_ENUM: {
        CONVERT(enum, 4);
      }

      case OBJ_INSTANCE: {
        ObjString *className = AS_INSTANCE(value)->klass->name;

        CONVERT_VARIABLE(className->chars, className->length);
      }
      case OBJ_BOUND_METHOD: {
        CONVERT(method, 6);
      }
      case OBJ_CLOSURE:
      case OBJ_FUNCTION: {
        CONVERT(function, 8);
      }
      case OBJ_STRING: {
        CONVERT(string, 6);
      }
      case OBJ_LIST: {
        CONVERT(list, 4);
      }
      case OBJ_NATIVE: {
        CONVERT(native, 6);
      }
      default:
        break;
    }
  }

  CONVERT(unknown, 7);
#undef CONVERT
#undef CONVERT_VARIABLE
}

void printValue(Value value) {

#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    printObject(value);
  }
#else




  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;

    case VAL_OBJ: printObject(value); break;

  }


#endif

}


bool valuesEqual(Value a, Value b) {

#ifdef NAN_BOXING

  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }

  return a == b;
#else

  if (a.type != b.type) return false;

  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);


    case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);

    default:
      return false; 
  }

#endif

}

