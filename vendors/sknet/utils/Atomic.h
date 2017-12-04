/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_UTILS_ATOMIC_H
#define ANDROID_UTILS_ATOMIC_H
#include<stdio.h>
#include<stdlib.h>
#include <stdint.h>
//#include<sys/types.h>

inline int android_atomic_inc(volatile int32_t *count){
    int ref = *count;
    (*count) ++;
    return ref;
}

inline int android_atomic_dec(volatile int32_t *count){
    int ref = *count;
    (*count) --;
    return ref;
}

inline int android_atomic_add(int value,volatile int32_t *result){
    return *result += value ;
}


inline int android_atomic_cmpxchg(int oldvalue ,int newvalue,volatile int32_t *op2){
    if(oldvalue == *op2){
        *op2 = newvalue;
        return 0;
    }else{
        return 1;
    }
}

inline void android_atomic_or(int mode, volatile int32_t *op2){
    *op2 |= mode;
}


#endif // ANDROID_UTILS_ATOMIC_H
