#include <stdarg.h>

#include <stdio.h>

#include <string.h>


#include <time.h>



#include "common.h"
#include "compiler.h"
#include "debug.h"


#include "object.h"
#include "memory.h"

#include "vm.h"

#define TABLE_MAX_LOAD 0.75
#define TABLE_MIN_LOAD 0.25

VM vm; 

static Value clockNative(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

bool dictSet(ObjDict* dict, Value key, Value value);
static void adjDictCapacity(ObjDict *dict, int capacityMask);
static uint32_t hashObject(Obj *object);
static inline uint32_t hashBits(uint64_t hash);
bool dictGet(ObjDict *dict, Value key, Value *value);


static void resetStack() {
  vm.stackTop = vm.stack;

  vm.frameCount = 0;


  vm.openUpvalues = NULL;

}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];


    ObjFunction* function = frame->closure->function;
    
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ",
            function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}

void defineNative(const char* name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

static uint32_t hashValue(Value value) {
  if (IS_OBJ(value)) {
    return hashObject(AS_OBJ(value));
  }

  return hashBits(value);
}

static inline uint32_t hashBits(uint64_t hash) {
  // From v8's ComputeLongHash() which in turn cites:
  // Thomas Wang, Integer Hash Functions.
  // http://www.concentric.net/~Ttwang/tech/inthash.htm
  hash = ~hash + (hash << 18);  // hash = (hash << 18) - hash - 1;
  hash = hash ^ (hash >> 31);
  hash = hash * 21;  // hash = (hash + (hash << 2)) + (hash << 4);
  hash = hash ^ (hash >> 11);
  hash = hash + (hash << 6);
  hash = hash ^ (hash >> 22);
  return (uint32_t) (hash & 0x3fffffff);
}

static uint32_t hashObject(Obj *object) {
  switch (object->type) {
    case OBJ_STRING: {
      return ((ObjString *) object)->hash;
    }

      // Should never get here
    default: {
#ifdef DEBUG_PRINT_CODE
      printf("Object: ");
            printValue(OBJ_VAL(object));
            printf(" not hashable!\n");
            exit(1);
#endif
      return -1;
    }
  }
}

bool dictGet(ObjDict *dict, Value key, Value *value) {
  if (dict->count == 0) return false;

  DictItem *entry;
  uint32_t index = hashValue(key) & dict->capacityMask;
  uint32_t psl = 0;

  for (;;) {
    entry = &dict->entries[index];

    if (IS_EMPTY(entry->key) || psl > entry->psl) {
      return false;
    }

    if (valuesEqual(key, entry->key)) {
      break;
    }

    index = (index + 1) & dict->capacityMask;
    psl++;
  }

  *value = entry->value;
  return true;
}

static void adjDictCapacity(ObjDict *dict, int capacityMask) {
  DictItem *entries = ALLOCATE(DictItem, capacityMask + 1);
  for (int i = 0; i <= capacityMask; i++) {
    entries[i].key = EMPTY_VAL;
    entries[i].value = NIL_VAL;
    entries[i].psl = 0;
  }

  DictItem *oldEntries = dict->entries;
  int oldMask = dict->capacityMask;

  dict->count = 0;
  dict->entries = entries;
  dict->capacityMask = capacityMask;

  for (int i = 0; i <= oldMask; i++) {
    DictItem *entry = &oldEntries[i];
    if (IS_EMPTY(entry->key)) continue;

    dictSet(dict, entry->key, entry->value);
  }

  FREE_ARRAY(DictItem, oldEntries, oldMask + 1);
}

bool dictSet(ObjDict* dict, Value key, Value value) {
  if (dict->count + 1 > (dict->capacityMask + 1) * TABLE_MAX_LOAD) {
    int capacityMask = GROW_CAPACITY(dict->capacityMask + 1) - 1;
    adjDictCapacity(dict, capacityMask);
  }

  uint32_t index = hashValue(key) & dict->capacityMask;
  DictItem *bucket;
  bool isNewKey = false;

  DictItem entry;
  entry.key = key;
  entry.value = value;
  entry.psl = 0;

  for (;;) {
    bucket = &dict->entries[index];

    if (IS_EMPTY(bucket->key)) {
      isNewKey = true;
      break;
    } else {
      if (valuesEqual(key, bucket->key)) {
        break;
      }

      if (entry.psl > bucket->psl) {
        isNewKey = true;
        DictItem tmp = entry;
        entry = *bucket;
        *bucket = tmp;
      }
    }

    index = (index + 1) & dict->capacityMask;
    entry.psl++;
  }

  *bucket = entry;
  if (isNewKey) dict->count++;
  return isNewKey;
}

bool isValidKey(Value value) {
  if (IS_NIL(value) || IS_BOOL(value) || IS_NUMBER(value) ||
      IS_STRING(value)) {
    return true;
  }

  return false;
}

void initVM() {
  resetStack();

  vm.objects = NULL;

  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);
  vm.initString = NULL;
  vm.initString = copyString("init", 4);
  defineNative("clock", clockNative);
}

void freeVM() {

  freeTable(&vm.globals);


  freeTable(&vm.strings);


  vm.initString = NULL;


  freeObjects();

}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}


Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}


static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static bool call(ObjClosure* closure, int argCount) {



  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        closure->function->arity, argCount);


    return false;
  }



  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }


  CallFrame* frame = &vm.frames[vm.frameCount++];


  frame->closure = closure;
  frame->ip = closure->function->chunk.code;


  frame->slots = vm.stackTop - argCount - 1;
  return true;
}


static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {

      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);

        vm.stackTop[-argCount - 1] = bound->receiver;

        return call(bound->method, argCount);
      }

      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));

        Value initializer;
        if (tableGet(&klass->methods, vm.initString,
                     &initializer)) {
          return call(AS_CLOSURE(initializer), argCount);

        } else if (argCount != 0) {
          runtimeError("Expected 0 arguments but got %d.",
                       argCount);
          return false;

        }


        return true;
      }


      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);



        
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(argCount, vm.stackTop - argCount);
        vm.stackTop -= argCount + 1;
        push(result);
        return true;
      }


      default:
        
        break;
    }
  }

  runtimeError("Can only call functions and classes.");
  return false;
}


static bool invokeFromClass(ObjClass* klass, ObjString* name,
                            int argCount) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  return call(AS_CLOSURE(method), argCount);
}


static bool invoke(ObjString* name, int argCount) {
  Value receiver = peek(argCount);

  if (isObjType(receiver, OBJ_CLASS)) {
      ObjInstance* instance = AS_INSTANCE(receiver);


      Value value;
      if (tableGet(&instance->fields, name, &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
      }

      return invokeFromClass(instance->klass, name, argCount);

  } else if (isObjType(receiver, OBJ_ENUM)) {
      ObjEnum* _enum = AS_ENUM(receiver);

      Value value;
      if (tableGet(&_enum->variables, name, &value)) {
        return callValue(value, argCount);
      }

      runtimeError("'%s' enum has no property '%s'.", _enum->name->chars, name->chars);
      return false;
    } else if (isObjType(receiver, OBJ_INSTANCE)) {
      ObjInstance* instance = AS_INSTANCE(receiver);
      Value value;
      if (tableGet(&instance->klass->methods, name, &value)) {
        return call(AS_CLOSURE(value), argCount);
      }
      if (tableGet(&instance->fields, name, &value)) {
        return callValue(value, argCount);
      }

      runtimeError("Undefined property '%s'.", name->chars);
      return false;
    }

  runtimeError("Only instances have methods.");
  return false;
}


static bool bindMethod(ObjClass* klass, ObjString* name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMethod* bound = newBoundMethod(peek(0),
                                         AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}


static ObjUpvalue* captureUpvalue(Value* local) {

  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* upvalue = vm.openUpvalues;

  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }


  ObjUpvalue* createdUpvalue = newUpvalue(local);

  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }


  return createdUpvalue;
}


static void closeUpvalues(Value* last) {
  while (vm.openUpvalues != NULL &&
         vm.openUpvalues->location >= last) {
    ObjUpvalue* upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}


static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}


static bool isFalsey(Value value) {
  return IS_NIL(value) ||
         (IS_BOOL(value) && !AS_BOOL(value)) ||
         (IS_NUMBER(value) && AS_NUMBER(value) == 0) ||
         (IS_STRING(value) && AS_CSTRING(value)[0] == '\0') ||
         (IS_LIST(value) && AS_LIST(value)->values.count == 0) ||
         (IS_DICT(value) && AS_DICT(value)->count == 0);
}


static void concatenate() {


  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));


  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);

  pop();
  pop();

  push(OBJ_VAL(result));
}


static InterpretResult run() {

  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  register uint8_t* ip = frame->ip;

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))



#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])


#define READ_STRING() AS_STRING(READ_CONSTANT())


#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)


  for (;;) {

#define STORE_FRAME frame->ip = ip

#define R_ERROR(...)                                              \
        do {                                                                \
            STORE_FRAME;                                                    \
            runtimeError(__VA_ARGS__);                                  \
            return INTERPRET_RUNTIME_ERROR;                                 \
        } while (0)

#define R_ERROR_T(error, distance)                                    \
        do {                                                                       \
            STORE_FRAME;                                                           \
            int valLength = 0;                                                     \
            char *val = valueTypeToString(peek(distance), &valLength);     \
            runtimeError(error, val);                                          \
            FREE_ARRAY(char, val, valLength + 1);                              \
            return INTERPRET_RUNTIME_ERROR;                                        \
        } while (0)
#ifdef DEBUG_TRACE_EXECUTION
    #define DISPATCH()                                                                        \
            do                                                                                    \
            {                                                                                     \
                printf("          ");                                                             \
                for (Value *stackValue = vm->stack; stackValue < vm->stackTop; stackValue++) {    \
                    printf("[ ");                                                                 \
                    printValue(*stackValue);                                                      \
                    printf(" ]");                                                                 \
                }                                                                                 \
                printf("\n");                                                                     \
                disassembleInstruction(&frame->closure->function->chunk,                          \
                        (int) (ip - frame->closure->function->chunk.code));                \
                goto *dispatchTable[instruction = READ_BYTE()];                                   \
            }                                                                                     \
            while (false)
#else
#endif

#define INTERPRET_LOOP                                        \
            loop:                                                 \
                switch (instruction = READ_BYTE())

#define DISPATCH() goto loop

#define CASE_CODE(name) case OP_##name



    uint8_t instruction;
    INTERPRET_LOOP
    {

      CASE_CODE(CONSTANT): {
        Value constant = READ_CONSTANT();


        push(constant);
        DISPATCH();
      }


      CASE_CODE(NIL): push(NIL_VAL); DISPATCH();
      CASE_CODE(TRUE): push(BOOL_VAL(true)); DISPATCH();
      CASE_CODE(FALSE): push(BOOL_VAL(false)); DISPATCH();


      CASE_CODE(POP): pop(); DISPATCH();



      CASE_CODE(GET_LOCAL): {
        uint8_t slot = READ_BYTE();


        push(frame->slots[slot]);

        DISPATCH();
      }



      CASE_CODE(SET_LOCAL): {
        uint8_t slot = READ_BYTE();


        frame->slots[slot] = peek(0);

        DISPATCH();
      }



      CASE_CODE(GET_GLOBAL): {
        ObjString* name = READ_STRING();
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          R_ERROR("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        DISPATCH();
      }



      CASE_CODE(DEFINE_GLOBAL): {
        ObjString* name = READ_STRING();
        tableSet(&vm.globals, name, peek(0));
        pop();
        DISPATCH();
      }



      CASE_CODE(SET_GLOBAL): {
        ObjString* name = READ_STRING();
        if (tableSet(&vm.globals, name, peek(0))) {
          tableDelete(&vm.globals, name); 
          R_ERROR("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        DISPATCH();
      }



      CASE_CODE(GET_UPVALUE): {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->location);
        DISPATCH();
      }



      CASE_CODE(SET_UPVALUE): {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->location = peek(0);
        DISPATCH();
      }



      CASE_CODE(GET_PROPERTY): {
        Value receiver = peek(0);

        if (!IS_OBJ(receiver)) {
          R_ERROR("Object has no properties.");
        }

        switch (getObjType(receiver)) {
          case OBJ_CLASS: {
            ObjInstance* instance = AS_INSTANCE(peek(0));
            ObjString* name = READ_STRING();

            Value value;
            if (tableGet(&instance->fields, name, &value)) {
              pop();
              push(value);
              DISPATCH();
            }

            if (!bindMethod(instance->klass, name)) {
              return INTERPRET_RUNTIME_ERROR;
            }
            DISPATCH();
          }
          case OBJ_ENUM: {
            ObjEnum* _enum = AS_ENUM(receiver);
            ObjString* name = READ_STRING();
            Value value;

            if (tableGet(&_enum->variables, name, &value)) {
              pop();
              push(value);
              DISPATCH();
            }
            R_ERROR("'%s' enum has no property '%s'.", _enum->name->chars, name->chars);
            DISPATCH();
          }
          default: {
            R_ERROR("Only instances have properties.");
            return INTERPRET_RUNTIME_ERROR;
          }
        }
      }



      CASE_CODE(SET_PROPERTY): {

        if (!IS_INSTANCE(peek(1))) {
          R_ERROR("Only instances have fields.");
          return INTERPRET_RUNTIME_ERROR;
        }


        ObjInstance* instance = AS_INSTANCE(peek(1));
        tableSet(&instance->fields, READ_STRING(), peek(0));
        
        Value value = pop();
        pop();
        push(value);
        DISPATCH();
      }



      CASE_CODE(GET_SUPER): {
        ObjString* name = READ_STRING();
        ObjClass* superclass = AS_CLASS(pop());
        if (!bindMethod(superclass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        DISPATCH();
      }



      CASE_CODE(EQUAL): {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        DISPATCH();
      }



      CASE_CODE(GREATER):  BINARY_OP(BOOL_VAL, >); DISPATCH();
      CASE_CODE(LESS):     BINARY_OP(BOOL_VAL, <); DISPATCH();





      CASE_CODE(ADD): {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else if (IS_LIST(peek(0)) && IS_LIST(peek(1))) {
          ObjList* list1 = AS_LIST(peek(1));
          ObjList* list2 = AS_LIST(peek(0));

          ObjList* listx = newList();
          push(OBJ_VAL(listx));

          for (int i = 0; i < list1->values.count; ++i) {
            writeValueArray(&listx->values, list1->values.values[i]);
          }

          for (int i = 0; i < list2->values.count; ++i) {
            writeValueArray(&listx->values, list2->values.values[i]);
          }
          pop();
          pop();
          pop();

          push(OBJ_VAL(listx));
        } else {
          R_ERROR(
              "Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        DISPATCH();
      }


      CASE_CODE(SUBTRACT): BINARY_OP(NUMBER_VAL, -); DISPATCH();
      CASE_CODE(MULTIPLY): BINARY_OP(NUMBER_VAL, *); DISPATCH();
      CASE_CODE(DIVIDE):   BINARY_OP(NUMBER_VAL, /); DISPATCH();


      CASE_CODE(NOT):
        push(BOOL_VAL(isFalsey(pop())));
        DISPATCH();


      CASE_CODE(NEGATE):
        if (!IS_NUMBER(peek(0))) {
          R_ERROR("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        push(NUMBER_VAL(-AS_NUMBER(pop())));
        DISPATCH();



      CASE_CODE(PRINT): {
        printValue(pop());
        printf("\n");
        DISPATCH();
      }



      CASE_CODE(JUMP): {
        uint16_t offset = READ_SHORT();


        frame->ip += offset;

        DISPATCH();
      }



      CASE_CODE(JUMP_IF_FALSE): {
        uint16_t offset = READ_SHORT();


        if (isFalsey(peek(0))) frame->ip += offset;

        DISPATCH();
      }



      CASE_CODE(LOOP): {
        uint16_t offset = READ_SHORT();


        frame->ip -= offset;

        DISPATCH();
      }



      CASE_CODE(CALL): {
        int argCount = READ_BYTE();
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }

        frame = &vm.frames[vm.frameCount - 1];

        DISPATCH();
      }



      CASE_CODE(INVOKE): {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
        if (!invoke(method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        DISPATCH();
      }
      


      CASE_CODE(SUPER_INVOKE): {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
        ObjClass* superclass = AS_CLASS(pop());
        if (!invokeFromClass(superclass, method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        DISPATCH();
      }



      CASE_CODE(CLOSURE): {
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
        ObjClosure* closure = newClosure(function);
        push(OBJ_VAL(closure));

        for (int i = 0; i < closure->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            closure->upvalues[i] =
                captureUpvalue(frame->slots + index);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }

        DISPATCH();
      }



      CASE_CODE(CLOSE_UPVALUE):
        closeUpvalues(vm.stackTop - 1);
        pop();
        DISPATCH();


      CASE_CODE(RETURN): {




        Value result = pop();


        closeUpvalues(frame->slots);


        vm.frameCount--;
        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.stackTop = frame->slots;
        push(result);

        frame = &vm.frames[vm.frameCount - 1];
        DISPATCH();

      }


      CASE_CODE(CLASS):
        push(OBJ_VAL(newClass(READ_STRING())));
        DISPATCH();

      CASE_CODE(ENUM): {
        ObjEnum *_enum = newEnum(READ_STRING());
        push(OBJ_VAL(_enum));
        DISPATCH();
      }

      CASE_CODE(SET_ENUM_VALUE): {
        Value value = peek(0);
        ObjEnum* _enum = AS_ENUM(peek(1));

        tableSet(&_enum->variables, READ_STRING(), value);
        pop();
        DISPATCH();
      }

      CASE_CODE(INHERIT): {
        Value superclass = peek(1);

        if (!IS_CLASS(superclass)) {
          R_ERROR("Superclass must be a class.");
          return INTERPRET_RUNTIME_ERROR;
        }


        ObjClass* subclass = AS_CLASS(peek(0));
        tableAddAll(&AS_CLASS(superclass)->methods,
                    &subclass->methods);
        pop();
        DISPATCH();
      }



      CASE_CODE(METHOD):
        defineMethod(READ_STRING());
        DISPATCH();

      CASE_CODE(NEW_LIST): {
        ObjList* list = newList();
        push(OBJ_VAL(list));
        break;
      }

      CASE_CODE(ADD_LIST): {
        Value addValue = pop();
        Value listValue = pop();

        ObjList* list = AS_LIST(listValue);
        writeValueArray(&list->values, addValue);

        push(OBJ_VAL(list));
        break;
      }

      CASE_CODE(UNPACK_LIST): {
        int varCount = READ_BYTE();

        if (!IS_LIST(peek(0))) {
          R_ERROR("Cannot unpack a value which is not of type list.");
        }

        ObjList* list = AS_LIST(pop());

        if (varCount != list->values.count) {
          if (varCount >= list->values.count) {
            R_ERROR("Too many values to unpack.");
          } else {
            R_ERROR("Not enough values to unpack.");
          }
        }

        for (int i = 0; i < list->values.count; ++i) {
          push(list->values.values[i]);
        }

        DISPATCH();
      }

      CASE_CODE(SUBSCRIPT): {
        Value indexValue = peek(0);
        Value subscriptValue = peek(1);

        if (!IS_OBJ(subscriptValue)) {
          R_ERROR_T("'%s' is not subscriptable", 1);
        }

        switch (getObjType(subscriptValue)) {
          case OBJ_LIST: {
            if (!IS_NUMBER(indexValue)) {
              R_ERROR("List index must be an integer value.");
            }

            ObjList* list = AS_LIST(subscriptValue);
            int index = AS_NUMBER(indexValue);

            if (index < 0) {
              index = list->values.count + index;
            }

            if (index >= 0 && index < list->values.count) {
              pop();
              pop();
              push(list->values.values[index]);
              DISPATCH();
            }

            R_ERROR("List index out of range.");
          }

          case OBJ_DICT: {
            ObjDict* dict = AS_DICT(subscriptValue);
            if (!isValidKey(indexValue)) {
              R_ERROR("Type of Dictionary key must be immutable");
            }

            Value value;
            pop();
            pop();
            if (dictGet(dict, indexValue, &value)) {
              push(value);
              DISPATCH();
            }

            R_ERROR("Key '%s' does not exist inside dictionary.", valueToString(indexValue));
          }

          default: R_ERROR_T("'%s' is not subscriptable", 1);
        }
      }

      CASE_CODE(SUBSCRIPT_ASSIGN): {
        Value assignValue = peek(0);
        Value indexValue = peek(1);
        Value subscriptValue = peek(2);

        if (!IS_OBJ(subscriptValue)) {
          R_ERROR_T("'%s' does not support item assignment", 2);
        }

        switch (getObjType(subscriptValue)) {
          case OBJ_LIST: {
            if (!IS_NUMBER(indexValue)) {
              R_ERROR("List index must be an integer value.");
            }

            ObjList *list = AS_LIST(subscriptValue);
            int index = AS_NUMBER(indexValue);

            if (index < 0)
              index = list->values.count + index;

            if (index >= 0 && index < list->values.count) {
              list->values.values[index] = assignValue;
              pop();
              pop();
              pop();
              push(NIL_VAL);
              DISPATCH();
            }

            R_ERROR("List index out of bounds.");
            return INTERPRET_RUNTIME_ERROR;
          }

          case OBJ_DICT: {
            ObjDict* dict = AS_DICT(subscriptValue);
            if (!isValidKey(indexValue)) {
              R_ERROR("Type of Dictionary key must be immutable");
            }

            dictSet(dict, indexValue, assignValue);
            pop();
            pop();
            pop();
            push(NIL_VAL);
            DISPATCH();
          }

          default: {
            R_ERROR_T("'%s' does not support item assignment", 2);
          }
        }
      }

      CASE_CODE(SUBSCRIPT_PUSH): {
        Value value = peek(0);
        Value indexValue = peek(1);
        Value subscriptValue = peek(2);

        if (!IS_OBJ(subscriptValue)) {
          R_ERROR_T("'%s' does not support item assignment.", 2);
        }

        switch (getObjType(subscriptValue)) {
          case OBJ_LIST: {
            if (!IS_NUMBER(indexValue)) {
              R_ERROR("List index must be an integer value.");
            }

            ObjList* list = AS_LIST(subscriptValue);
            int index = AS_NUMBER(indexValue);

            if (index < 0) {
              index = list->values.count + index;
            }

            if (index >= 0 && index < list->values.count) {
              vm.stackTop[-1] = list->values.values[index];
              push(value);
              DISPATCH();
            }
            R_ERROR("List index out of range.");
            return INTERPRET_RUNTIME_ERROR;
          }

          case OBJ_DICT: {
            ObjDict *dict = AS_DICT(subscriptValue);
            if (!isValidKey(indexValue)) {
              R_ERROR("Type of Dictionary key must be immutable");
            }

            Value dictValue;
            if (!dictGet(dict, indexValue, &dictValue)) {
              R_ERROR("Key '%s' does not exist inside dictionary.", valueToString(indexValue));
            }

            vm.stackTop[-1] = dictValue;
            push(value);

            DISPATCH();
          }

          default: R_ERROR_T("'%s' does not support item assignment.", 2);
        }

        }

      CASE_CODE(NEW_DICT): {
        int count = READ_BYTE();
        ObjDict *dict = newDict();
        push(OBJ_VAL(dict));

        for (int i = count * 2; i > 0; i -= 2) {
          if (!isValidKey(peek(i))) {
            R_ERROR("Type of Dictionary key must be immutable.");
          }

          dictSet(dict, peek(i), peek(i - 1));
        }

        vm.stackTop -= count * 2 + 1;
        push(OBJ_VAL(dict));

        DISPATCH();
      }

    }
  }

#undef READ_BYTE

#undef READ_SHORT


#undef READ_CONSTANT


#undef READ_STRING


#undef BINARY_OP

}


void hack(bool b) {
  
  
  run();
  if (b) hack(false);
}




InterpretResult interpret(const char* source) {



  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));




  ObjClosure* closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  callValue(OBJ_VAL(closure), 0);






  return run();


}

