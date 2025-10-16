// Copyright (c) 2024, Evangelion Manuhutu

#ifndef TYPES_HPP
#define TYPES_HPP

#include <memory>

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long long i64;
typedef int i32;
typedef short i16;
typedef char i8;

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename... Args>
Ref<T> CreateRef(Args&&... args){
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename... Args>
Scope<T> CreateScope(Args... args){
    return std::make_unique<T>(std::forward<Args>(args)...);
}

#endif //TYPES_H
