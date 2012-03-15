
#include "LCArray.h"

typedef struct arrayData* arrayDataRef;

LCCompare arrayCompare(LCObjectRef object1, LCObjectRef object2);
void arrayDealloc(LCObjectRef object);
void arraySerialize(LCObjectRef object, void* cookie, callback flush, FILE* fd);

bool resizeBuffer(arrayDataRef array, size_t size);
void mutableArraySerialize(LCObjectRef object, void* cookie, callback flush, FILE* fd);

struct arrayData {
  size_t length;
  size_t bufferLength;
  LCObjectRef* objects;
};
struct LCType typeArray = {
  .immutable = true,
  .dealloc = arrayDealloc,
  .compare = arrayCompare,
  .serialize = arraySerialize
};

struct LCType typeMutableArray = {
  .immutable = false,
  .dealloc = arrayDealloc,
  .compare = arrayCompare,
  .serialize = arraySerialize
};

LCTypeRef LCTypeArray = &typeArray;
LCTypeRef LCTypeMutableArray = &typeMutableArray;

LCArrayRef LCArrayCreate(LCObjectRef objects[], size_t length) {
  if (!objectsImmutable(objects, length)) {
    perror(ErrorObjectImmutable);
    return NULL;
  }
  return LCMutableArrayCreate(objects, length);
};

LCArrayRef LCArrayCreateAppendingObject(LCArrayRef array, LCObjectRef object) {
  return LCArrayCreateAppendingObjects(array, &object, 1);
}

LCArrayRef LCArrayCreateAppendingObjects(LCArrayRef object, LCObjectRef objects[], size_t length) {
  if (!objectsImmutable(objects, length)) {
    perror(ErrorObjectImmutable);
    return NULL;
  }
  arrayDataRef array = objectData(object);
  size_t totalLength = array->length + length;
  arrayDataRef newArray = malloc(sizeof(struct arrayData) + totalLength * sizeof(LCObjectRef));
  if(newArray) {
    newArray->length = totalLength;
    memcpy(newArray->objects, array->objects, array->length * sizeof(LCObjectRef));
    memcpy(&(newArray->objects[array->length]), objects, length * sizeof(LCObjectRef));
    for (LCInteger i=0; i<totalLength; i++) {
      objectRetain(newArray->objects[i]);
    }
    return objectCreate(LCTypeArray, newArray);
  } else {
    return NULL;
  }
}

LCArrayRef LCArrayCreateFromArrays(LCArrayRef arrays[], size_t length) {
  size_t totalLength = 0;
  for (LCInteger i=0; i<length; i++) {
    totalLength = totalLength + LCArrayLength(arrays[i]);
  }
  
  arrayDataRef newArray = malloc(sizeof(struct arrayData) + totalLength * sizeof(LCObjectRef));
  if (newArray) {
    newArray->length = totalLength;
    size_t copyPos = 0;
    for (LCInteger i=0; i<length; i++) {
      size_t copyLength = LCArrayLength(arrays[i]);
      memcpy(&(newArray->objects[copyPos]), LCArrayObjects(arrays[i]), copyLength * sizeof(void*));
      copyPos = copyPos+copyLength;
    }
    for (LCInteger i=0; i<totalLength; i++) {
      objectRetain(newArray->objects[i]);
    }
    return objectCreate(LCTypeArray, newArray);
  } else {
    return NULL;
  }
}

LCObjectRef* LCArrayObjects(LCArrayRef object) {
  arrayDataRef array = objectData(object);
  return array->objects;
}

LCObjectRef LCArrayObjectAtIndex(LCArrayRef object, LCInteger index) {
  arrayDataRef array = objectData(object);
  return array->objects[index];
}

size_t LCArrayLength(LCArrayRef object) {
  arrayDataRef array = objectData(object);
  return array->length;
}

LCArrayRef LCArrayCreateSubArray(LCArrayRef object, LCInteger start, size_t length) {
  size_t arrayLength = LCArrayLength(object);
  LCObjectRef* arrayObjects = LCArrayObjects(object);
  if (start >= arrayLength) {
    return LCArrayCreate(NULL, 0);
  }
  if (length == -1) {
    length = arrayLength - start;
  }
  return LCArrayCreate(&(arrayObjects[start]), length);
}

LCArrayRef LCArrayCreateArrayWithMap(LCArrayRef array, void* info, LCCreateEachCb each) {
  size_t arrayLength = LCArrayLength(array);
  LCArrayRef* arrayObjects = LCArrayObjects(array);
  LCArrayRef newObjects[arrayLength];
  for (LCInteger i=0; i<arrayLength; i++) {
    newObjects[i] = each(i, info, arrayObjects[i]);
  }
  LCArrayRef newArray = LCArrayCreate(newObjects, arrayLength);
  for (LCInteger i=0; i<arrayLength; i++) {
    objectRelease(newObjects[i]);
  }
  return newArray;
}

LCCompare arrayCompare(LCObjectRef array1, LCObjectRef array2) {
  return objectCompare(LCArrayObjectAtIndex(array1, 0), LCArrayObjectAtIndex(array2, 0));
}

void arrayDealloc(LCObjectRef object) {
  for (LCInteger i=0; i<LCArrayLength(object); i++) {
    objectRelease(LCArrayObjectAtIndex(object, i));
  }
  lcFree(objectData(object));
}

void arraySerialize(LCObjectRef object, void* cookie, callback flush, FILE* fd) {
  fprintf(fd, "[");
  for (LCInteger i=0; i<LCArrayLength(object); i++) {
    objectSerialize(LCArrayObjectAtIndex(object, i), fd);
    if (i< LCArrayLength(object) -1) {
      fprintf(fd, ", ");
    }
  }
  fprintf(fd, "]");
}

// LCMutableArray

LCMutableArrayRef LCMutableArrayCreate(LCObjectRef objects[], size_t length) {
  arrayDataRef newArray = malloc(sizeof(struct arrayData));
  if (newArray) {
    newArray->objects = NULL;
    if(length > 0) {
      for(LCInteger i=0; i<length; i++) {
        objectRetain(objects[i]);
      }
      resizeBuffer(newArray, length);
      memcpy(newArray->objects, objects, length * sizeof(void*));  
    } else {
      resizeBuffer(newArray, 10);
    }
    newArray->length = length;
    return objectCreate(LCTypeMutableArray, newArray);
  } else {
    return NULL;
  }
};

inline LCObjectRef* LCMutableArrayObjects(LCMutableArrayRef array) {
  return LCArrayObjects(array);
}

inline LCObjectRef LCMutableArrayObjectAtIndex(LCMutableArrayRef array, LCInteger index) {
  return LCArrayObjectAtIndex(array, index);
}

inline size_t LCMutableArrayLength(LCMutableArrayRef array) {
  return LCArrayLength(array);
}

inline LCMutableArrayRef LCMutableArrayCreateSubArray(LCMutableArrayRef array, LCInteger start, size_t length) {
  return LCArrayCreateSubArray(array, start, length);
}

LCMutableArrayRef LCMutableArrayCreateFromArray(LCArrayRef array) {
  return LCMutableArrayCreate(LCArrayObjects(array), LCArrayLength(array));
}

LCArrayRef LCMutableArrayCreateArray(LCMutableArrayRef array) {
  return LCArrayCreate(LCMutableArrayObjects(array), LCMutableArrayLength(array));
}

LCMutableArrayRef LCMutableArrayCopy(LCMutableArrayRef array) {
  return LCMutableArrayCreate(LCMutableArrayObjects(array), LCMutableArrayLength(array));
}

void LCMutableArrayAddObject(LCMutableArrayRef array, LCObjectRef object) {
  arrayDataRef arrayData = objectData(array);
  size_t arrayLength = LCMutableArrayLength(array);
  if(arrayLength+1 > arrayData->bufferLength) {
    resizeBuffer(arrayData, arrayData->bufferLength*2);
  }
  objectRetain(object);
  LCMutableArrayObjects(array)[arrayLength] = object;
  arrayData->length = arrayLength + 1;
}

void LCMutableArrayAddObjects(LCMutableArrayRef array, LCObjectRef objects[], size_t length) {
  for (LCInteger i=0; i<length; i++) {
    LCMutableArrayAddObject(array, objects[i]);
  }
}

void LCMutableArrayRemoveIndex(LCMutableArrayRef array, LCInteger index) {
  LCObjectRef* arrayObjects = LCMutableArrayObjects(array);
  size_t arrayLength = LCMutableArrayLength(array);
  arrayDataRef arrayData = objectData(array);
  objectRelease(arrayObjects[index]);
  if (index < (arrayLength-1)) {
    size_t objectsToCopy = arrayLength - (index+1);
    memmove(&(arrayObjects[index]), &(arrayObjects[index+1]), objectsToCopy*sizeof(LCObjectRef));
  }
  arrayData->length = arrayLength-1;
}

void LCMutableArrayRemoveObject(LCMutableArrayRef array, LCObjectRef object) {
  for (LCInteger i=0; i<LCMutableArrayLength(object); i++) {
    if(LCMutableArrayObjectAtIndex(array, i) == object) {
      return LCMutableArrayRemoveIndex(array, i);
    }
  }
}

void LCMutableArraySort(LCMutableArrayRef array) {
  objectsSort(LCMutableArrayObjects(array), LCMutableArrayLength(array));
}

bool resizeBuffer(arrayDataRef array, size_t length) {
  void* buffer = realloc(array->objects, sizeof(void*) * length);
  if(buffer) {
    array->objects = buffer;
    array->bufferLength = length;
    return true;
  } else {
    return false;
  }
}
