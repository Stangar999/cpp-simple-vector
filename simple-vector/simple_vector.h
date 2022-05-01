﻿#pragma once

#include <cassert>
#include <initializer_list>
#include <iterator>

#include "array_ptr.h"

class ReserveProxyObj{
public:
    ReserveProxyObj() = delete;
    ReserveProxyObj(size_t capacity)
        :capacity_(capacity){

    }
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(ReserveProxyObj Obj)
        :arr_(Obj.capacity_)
        ,size_(0)
        ,capacity_(Obj.capacity_)
    {
    }

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        :arr_(size)
        ,size_(size)
        ,capacity_(size)
    {
        std::fill(arr_.Get(), arr_.Get() + size, Type());
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : SimpleVector(size)
    {
        std::fill(arr_.Get(), arr_.Get() + size, value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
         : SimpleVector(init.size())
    {
        std::copy(init.begin(),init.end(),arr_.Get());
    }

    SimpleVector(const SimpleVector& other)
        :arr_(other.GetCapacity())
        ,size_(other.GetSize())
        ,capacity_(other.GetCapacity())
    {
        std::copy(other.begin(),other.end(),arr_.Get());
    }

    SimpleVector(SimpleVector&& other)
        :arr_(std::move(other.arr_))
        ,size_(std::exchange(other.size_, 0))
        ,capacity_(std::exchange(other.capacity_, 0))
    {
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if(&rhs == this){
            return *this;
        }
        ArrayPtr<Type> tmp(rhs.GetCapacity());
        std::copy( rhs.begin(), rhs.begin() + rhs.GetCapacity(), tmp.Get() );
        arr_.swap(tmp);
        size_ = rhs.GetSize();
        capacity_ = rhs.GetCapacity();
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs){
        if(&rhs == this){
            return *this;
        }
        arr_ = std::move(rhs.arr_);
        size_ = std::exchange(rhs.size_, 0);
        capacity_ = std::exchange(rhs.capacity_, 0);
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if(size_ < capacity_ ){
            arr_[size_++] = item;
        }else{
            if(capacity_ == 0 && size_ == 0 ){
                ArrayPtr<Type> tmp(++capacity_);
                tmp[size_++] = item;
                arr_.swap(tmp);
            }else{
                ArrayPtr<Type> tmp(GetCapacity()*2);
                std::copy(arr_.Get(), arr_.Get() + size_, tmp.Get());
                tmp[size_++] = item;
                arr_.swap(tmp);
                capacity_*=2;
            }
        }
    }

    void PushBack(Type&& item) {
        if(size_ < capacity_ ){
            arr_[size_++] = std::move(item);
        }else{
            if(capacity_ == 0 && size_ == 0 ){
                ArrayPtr<Type> tmp(++capacity_);
                tmp[size_++] = std::move(item);
                arr_ = std::move(tmp);
            }else{
                ArrayPtr<Type> tmp(GetCapacity()*2);
                std::move(arr_.Get(), arr_.Get() + size_, tmp.Get());
                tmp[size_++] = std::move(item);
                arr_ = std::move(tmp);
                capacity_*=2;
            }
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        Iterator tmp_pos = const_cast<Iterator>(pos);
        if(size_ < capacity_ ){
            std::copy_backward(tmp_pos, end(), end() + 1);
            arr_[pos - begin()] = value;
            ++size_;
            return tmp_pos;
        }else{
            if(capacity_ == 0 && size_ == 0 ){
                ArrayPtr<Type> tmp(++capacity_);
                tmp[size_++] = value;
                arr_.swap(tmp);
                return arr_.Get();
            }else{
                size_t posIndex = pos - begin();
                ArrayPtr<Type> tmp(GetCapacity()*2);
                auto it_tmp = std::copy(arr_.Get(),  tmp_pos, tmp.Get());
                std::copy(tmp_pos, arr_.Get() + GetCapacity(), it_tmp + 1);
                tmp[posIndex] = value;
                arr_.swap(tmp);
                ++size_;
                capacity_*=2;
                return &arr_[posIndex];
            }
        }
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        Iterator tmp_pos = const_cast<Iterator>(pos);
        if(size_ < capacity_ ){
            std::move_backward(tmp_pos, end(), end() + 1);
            arr_[tmp_pos-begin()] = std::move(value);
            ++size_;
            return tmp_pos;
        }else{
            if(capacity_ == 0 && size_ == 0 ){
                ArrayPtr<Type> tmp(++capacity_);
                tmp[size_++] = std::move(value);
                arr_ = std::move(tmp);
                return arr_.Get();
            }else{
                size_t posIndex = pos - begin();
                ArrayPtr<Type> tmp(GetCapacity()*2);
                auto it_tmp = std::move(arr_.Get(),  tmp_pos, tmp.Get());
                std::move(tmp_pos, arr_.Get() + GetCapacity(), it_tmp + 1);
                tmp[posIndex] = std::move(value);
                arr_ = std::move(tmp);
                ++size_;
                capacity_ *= 2;
                return &arr_[posIndex];
            }
        }
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(size_);
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        Iterator tmp_pos = const_cast<Iterator>(pos);
        std::move(tmp_pos + 1, end(), tmp_pos);
        --size_;
        return tmp_pos;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        arr_.swap(other.arr_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    void Reserve(size_t new_capacity){
        if(new_capacity <= capacity_){
            return;
        }
        ArrayPtr<Type> tmp(new_capacity);
        std::move(arr_.Get(), arr_.Get() + size_, tmp.Get());
        arr_.swap(tmp);
        capacity_ = new_capacity;
    };

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return arr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return arr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if(index >= size_){
            throw std::out_of_range("");
        }
        return arr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if(index >= size_){
            throw std::out_of_range("");
        }
        return arr_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if(new_size <= size_){
            size_ = new_size;
        }else if (new_size <= capacity_) {
            //std::fill( std::make_move_iterator(&arr_[size_]), std::make_move_iterator(&arr_[new_size]), Type() );
            size_ = new_size;
        }else{
            ArrayPtr<Type> tmp(new_size);
            std::move( arr_.Get(), arr_.Get() + size_, tmp.Get() );
            //std::fill( std::make_move_iterator(&tmp[size_]), std::make_move_iterator(&tmp[new_size]), Type() );
            arr_ = std::move(tmp);
            size_ = new_size;
            capacity_ = new_size;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return arr_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return arr_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return arr_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return arr_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return begin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return end();
    }
private:
    ArrayPtr<Type> arr_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};


template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
