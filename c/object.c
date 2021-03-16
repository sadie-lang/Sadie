#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"

#include "table.h"

#include "value.h"
#include "vm.h"
#include <stdlib.h>

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;

  object->isMarked = false;

  
  object->next = vm.objects;
  vm.objects = object;


#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

  return object;
}

ObjBoundMethod* newBoundMethod(Value receiver,
                               ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod,
                                       OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjClass* newClass(ObjString* name) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
  initTable(&klass->methods);

  return klass;
}

ObjEnum *newEnum(ObjString* name) {
    ObjEnum* _enum = ALLOCATE_OBJ(ObjEnum, OBJ_ENUM);
    _enum->name = name;

    initTable(&_enum->variables);

    return _enum;
}

ObjClosure* newClosure(ObjFunction* function) {
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*,
                                   function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;

  return closure;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

static ObjString* allocateString(char* chars, int length,
                                 uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  push(OBJ_VAL(string));

  tableSet(&vm.strings, string, NIL_VAL);
  pop();

  return string;
}

static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;

  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }

  return hash;
}

ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL) return interned;

  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

  return allocateString(heapChars, length, hash);
}

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;

  return upvalue;
}

ObjList* newList() {
  ObjList *list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
  initValueArray(&list->values);
  return list;
}

ObjDict* newDict() {
  ObjDict* dict = ALLOCATE_OBJ(ObjDict, OBJ_DICT);

  dict->count = 0;
  dict->capacityMask = -1;
  dict->entries = NULL;

  return dict;
}


static void printFunction(ObjFunction* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }

  printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
      printFunction(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_CLASS:
      printf("%s", AS_CLASS(value)->name->chars);
      break;
    case OBJ_ENUM:
      printf("%s", AS_ENUM(value)->name->chars);
      break;
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_INSTANCE:
      printf("%s instance",
             AS_INSTANCE(value)->klass->name->chars);
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
    case OBJ_LIST: {
      ObjList* list = AS_LIST(value);
      printf("[");

      for (int i = 0; i < list->values.count; ++i) {
        printValue(list->values.values[i]);

        if (i != list->values.count - 1) {
          printf(", ");
        }
      }

      printf("]");
      break;
    }

    case OBJ_DICT: {
      //ObjDict* dict = AS_DICT(value);
      printf("%s", dictToString(value));
      break;
    }
  }
}

char *listToString(Value value) {
  int size = 50;
  ObjList *list = AS_LIST(value);
  char *listString = malloc(sizeof(char) * size);
  memcpy(listString, "[", 1);
  int listStringLength = 1;

  for (int i = 0; i < list->values.count; ++i) {
    Value listValue = list->values.values[i];

    char *element;
    int elementSize;

    if (IS_STRING(listValue)) {
      ObjString *s = AS_STRING(listValue);
      element = s->chars;
      elementSize = s->length;
    } else {
      element = valueToString(listValue);
      elementSize = strlen(element);
    }

    if (elementSize > (size - listStringLength - 3)) {
      if (elementSize > size * 2) {
        size += elementSize * 2 + 3;
      } else {
        size = size * 2 + 3;
      }

      char *newB = realloc(listString, sizeof(char) * size);

      if (newB == NULL) {
        printf("Unable to allocate memory\n");
        exit(71);
      }

      listString = newB;
    }

    if (IS_STRING(listValue)) {
      memcpy(listString + listStringLength, "\"", 1);
      memcpy(listString + listStringLength + 1, element, elementSize);
      memcpy(listString + listStringLength + 1 + elementSize, "\"", 1);
      listStringLength += elementSize + 2;
    } else {
      memcpy(listString + listStringLength, element, elementSize);
      listStringLength += elementSize;
      free(element);
    }

    if (i != list->values.count - 1) {
      memcpy(listString + listStringLength, ", ", 2);
      listStringLength += 2;
    }
  }

  memcpy(listString + listStringLength, "]", 1);
  listString[listStringLength + 1] = '\0';

  return listString;
}

char *dictToString(Value value) {
  int count = 0;
  int size = 50;
  ObjDict *dict = AS_DICT(value);
  char *dictString = malloc(sizeof(char) * size);
  memcpy(dictString, "{", 1);
  int dictStringLength = 1;

  for (int i = 0; i <= dict->capacityMask; ++i) {
    DictItem *item = &dict->entries[i];
    if (IS_EMPTY(item->key)) {
      continue;
    }

    count++;

    char *key;
    int keySize;

    if (IS_STRING(item->key)) {
      ObjString *s = AS_STRING(item->key);
      key = s->chars;
      keySize = s->length;
    } else {
      key = valueToString(item->key);
      keySize = strlen(key);
    }

    if (keySize > (size - dictStringLength - keySize - 4)) {
      if (keySize > size) {
        size += keySize * 2 + 4;
      } else {
        size *= 2 + 4;
      }

      char *newB = realloc(dictString, sizeof(char) * size);

      if (newB == NULL) {
        printf("Unable to allocate memory\n");
        exit(71);
      }

      dictString = newB;
    }

    if (IS_STRING(item->key)) {
      memcpy(dictString + dictStringLength, "\"", 1);
      memcpy(dictString + dictStringLength + 1, key, keySize);
      memcpy(dictString + dictStringLength + 1 + keySize, "\": ", 3);
      dictStringLength += 4 + keySize;
    } else {
      memcpy(dictString + dictStringLength, key, keySize);
      memcpy(dictString + dictStringLength + keySize, ": ", 2);
      dictStringLength += 2 + keySize;
      free(key);
    }

    char *element;
    int elementSize;

    if (IS_STRING(item->value)) {
      ObjString *s = AS_STRING(item->value);
      element = s->chars;
      elementSize = s->length;
    } else {
      element = valueToString(item->value);
      elementSize = strlen(element);
    }

    if (elementSize > (size - dictStringLength - elementSize - 6)) {
      if (elementSize > size) {
        size += elementSize * 2 + 6;
      } else {
        size = size * 2 + 6;
      }

      char *newB = realloc(dictString, sizeof(char) * size);

      if (newB == NULL) {
        printf("Unable to allocate memory\n");
        exit(71);
      }

      dictString = newB;
    }

    if (IS_STRING(item->value)) {
      memcpy(dictString + dictStringLength, "\"", 1);
      memcpy(dictString + dictStringLength + 1, element, elementSize);
      memcpy(dictString + dictStringLength + 1 + elementSize, "\"", 1);
      dictStringLength += 2 + elementSize;
    } else {
      memcpy(dictString + dictStringLength, element, elementSize);
      dictStringLength += elementSize;
      free(element);
    }

    if (count != dict->count) {
      memcpy(dictString + dictStringLength, ", ", 2);
      dictStringLength += 2;
    }
  }

  memcpy(dictString + dictStringLength, "}", 1);
  dictString[dictStringLength + 1] = '\0';

  return dictString;
}

char *classToString(Value value) {
  ObjClass *klass = AS_CLASS(value);
  char *classString = malloc(sizeof(char) * (klass->name->length + 7));
  memcpy(classString, "<Cls ", 5);
  memcpy(classString + 5, klass->name->chars, klass->name->length);
  memcpy(classString + 5 + klass->name->length, ">", 1);
  classString[klass->name->length + 6] = '\0';
  return classString;
}

char *instanceToString(Value value) {
  ObjInstance *instance = AS_INSTANCE(value);
  char *instanceString = malloc(sizeof(char) * (instance->klass->name->length + 12));
  memcpy(instanceString, "<", 1);
  memcpy(instanceString + 1, instance->klass->name->chars, instance->klass->name->length);
  memcpy(instanceString + 1 + instance->klass->name->length, " instance>", 10);
  instanceString[instance->klass->name->length + 11] = '\0';
  return instanceString;
}

char *objectToString(Value value) {
  switch (OBJ_TYPE(value)) {

    case OBJ_NATIVE: {
      char *nativeString = malloc(sizeof(char) * 12);
      memcpy(nativeString, "<fn native>", 11);
      nativeString[11] = '\0';
      return nativeString;
    }

    case OBJ_CLASS: {
      return classToString(value);
    }

    case OBJ_ENUM: {
      ObjEnum *enumObj = AS_ENUM(value);
      char *enumString = malloc(sizeof(char) * (enumObj->name->length + 8));
      memcpy(enumString, "<Enum ", 6);
      memcpy(enumString + 6, enumObj->name->chars, enumObj->name->length);
      memcpy(enumString + 6 + enumObj->name->length, ">", 1);

      enumString[7 + enumObj->name->length] = '\0';

      return enumString;
    }

    case OBJ_BOUND_METHOD: {
      //objBoundMethod *method = AS_BOUND_METHOD(value);
      char *methodString;
      methodString = malloc(sizeof(char) * 16);
      memcpy(methodString, "<Bound Method>", 15);
      methodString[15] = '\0';

      return methodString;
    }

    case OBJ_CLOSURE: {
      ObjClosure *closure = AS_CLOSURE(value);
      char *closureString;

      if (closure->function->name != NULL) {
        closureString = malloc(sizeof(char) * (closure->function->name->length + 6));
        snprintf(closureString, closure->function->name->length + 6, "<fn %s>", closure->function->name->chars);
      } else {
        closureString = malloc(sizeof(char) * 9);
        memcpy(closureString, "<Script>", 8);
        closureString[8] = '\0';
      }

      return closureString;
    }

    case OBJ_FUNCTION: {
      ObjFunction *function = AS_FUNCTION(value);
      char *functionString;

      if (function->name != NULL) {
        functionString = malloc(sizeof(char) * (function->name->length + 6));
        snprintf(functionString, function->name->length + 6, "<fn %s>", function->name->chars);
      } else {
        functionString = malloc(sizeof(char) * 5);
        memcpy(functionString, "<fn>", 4);
        functionString[4] = '\0';
      }

      return functionString;
    }

    case OBJ_INSTANCE: {
      return instanceToString(value);
    }

    case OBJ_STRING: {
      ObjString *stringObj = AS_STRING(value);
      char *string = malloc(sizeof(char) * stringObj->length + 3);
      snprintf(string, stringObj->length + 3, "'%s'", stringObj->chars);
      return string;
    }

    case OBJ_LIST: {
      return listToString(value);
    }

    case OBJ_DICT: {
      return dictToString(value);
    }

    case OBJ_UPVALUE: {
      char *upvalueString = malloc(sizeof(char) * 8);
      memcpy(upvalueString, "upvalue", 7);
      upvalueString[7] = '\0';
      return upvalueString;
    }
  }

  char *unknown = malloc(sizeof(char) * 9);
  snprintf(unknown, 8, "%s", "unknown");
  return unknown;
}