#pragma once
#include <vector>
#include <iterator>
#include <initializer_list>
#include <list>
#include <stdexcept>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
private:
    // во сколько раз увеличится размер вектора, когда
    // количество элементов станет больше, чем количество bucket'ов
    // возможно если его изменить, то все поломается
    static const int resize_value = 2;  
    // а вот это значение точно можете менять как вам захочется(но только не 0)
    static const size_t baseCapacity = 10;
    std::vector<std::list<std::pair<const KeyType, ValueType>>> data;
    size_t value_count = 0;
    Hash hasher;

    void reallocate(size_t new_size) {
        std::vector<std::list<std::pair<const KeyType, ValueType>>> new_data(new_size);
        std::swap(data, new_data);
        value_count = 0;
        for (const auto &bucket : new_data) {
            for (const auto &value: bucket) {
                insert(value);
            }
        }
    }

public:
    explicit HashMap(Hash _hasher = Hash())
    : hasher(_hasher) {
        data.resize(baseCapacity);
    }

    template<typename iter>
    HashMap(iter begin, iter end, Hash _hasher = Hash())
    : hasher(_hasher) {
        hasher = _hasher;
        data.resize(baseCapacity);
        while (begin != end) {
            insert(*begin++);
        }
    }

    HashMap(const std::initializer_list<std::pair<const KeyType, ValueType>>& out, Hash _hasher = Hash())
    : hasher(_hasher) {
        data.resize(baseCapacity);
        auto iter = out.begin();
        while (iter != out.end()) {
            insert(*iter++);
        }
    }

    // constructor from another hash_map
    HashMap(const HashMap& other)
        : hasher(other.hasher) {
        clear();
        for (const auto& bucket : other.data) {
            for (const auto& value : bucket) {
                insert(value);
            }
        }
    }

    // operator =
    HashMap& operator= (const HashMap& other) {
        if (this == &other) {
            return *this;
        }
        clear();
        hasher = other.hasher;
        for (const auto& bucket : other.data) {
            for (const auto& value : bucket) {
                insert(value);
            }
        }
        return *this;
    }

    void insert(const std::pair<const KeyType, ValueType>& value) {
        size_t bucket_id = hasher(value.first) % data.size();
        bool value_copy_exists = false;
        for (const auto& all : data[bucket_id]) {
            if (all.first == value.first) {
                value_copy_exists = true;
                // wrong, because
                // HashMap{{1, 1}, {1, 2}} = {1, 1}
                // all.second = value.second;
                break;
            }
        }
        if (!value_copy_exists) {
            data[bucket_id].push_back(value);
            ++value_count;
        }
        if (value_count > data.size()) {
            reallocate(data.size() * resize_value);
        }
    }

    size_t size() const {
        return value_count;
    }
    bool empty() const {
        return value_count == 0;
    }
    Hash hash_function() const {
        return hasher;
    }

    void erase(const KeyType& key) {
        size_t bucket_id = hasher(key) % data.size();
        bool has_deleted = false;
        for (auto iter = data[bucket_id].begin(); iter != data[bucket_id].end(); ++iter) {
            if (iter->first == key) {
                data[bucket_id].erase(iter);
                has_deleted = true;
                break;
            }
        }
        if (has_deleted) {
            --value_count;
            if (value_count == data.size() / resize_value / 2 && (data.size() / resize_value)) {
                reallocate(data.size() / resize_value);
            }
        }
    }


    // entry ITERATOR 
    using Pair = std::pair<const KeyType, ValueType>;
    class iterator {
        std::vector<std::list<Pair>>* data_pointer;
        typename std::vector<std::list<Pair>>::iterator bucket;
        typename std::list<Pair>::iterator value;


    public:
        iterator() = default;
        iterator(const iterator &other) {
            data_pointer = other.data_pointer;
            bucket = other.bucket;
            value = other.value;
        }
        iterator(
            typename std::vector<std::list<Pair>>* _data_pointer
            , typename std::vector<std::list<Pair>>::iterator _bucket
            , typename std::list<Pair>::iterator _value)
        : data_pointer(_data_pointer)
        , bucket(_bucket)
        , value(_value) {};

        iterator& operator++ () {
            if (bucket == data_pointer->end()) {
                // if we actually at the end: do strange shit
                return *this;
            }
            ++value;
            if (value == bucket->end()) {
                while (true) {
                    ++bucket;
                    if (bucket == data_pointer->end()) {
                        value = data_pointer->begin()->begin();
                        break;
                    }
                    if (bucket->begin() != bucket->end()) {
                        // if list has at least 1 element
                        value = bucket->begin();
                        break;
                    }
                }
            }
            return *this;
        }

        iterator operator++ (int) {
            iterator old(*this);
            ++*this;
            return old;
        }

        bool operator== (const iterator& other) const {
            return (data_pointer == other.data_pointer
                && bucket == other.bucket
                && value == other.value);
        }
        bool operator!= (const iterator& other) const {
            return !(*this == other);
        }

        Pair& operator* () {
            return *value; 
        }
        typename std::list<Pair>::iterator operator-> () {
            return value;
        }
    };
    iterator begin() {
        if (value_count != 0) {
            for (auto iter = data.begin(); iter != data.end(); ++iter) {
                if (iter->size() != 0) {
                    return iterator(&data, iter, iter->begin());
                }
            }
        }
        return end();
    }
    iterator end() {
        return iterator(&data, data.end(), data.begin()->begin());
    }
    iterator find(const KeyType& key) {
        size_t bucket_id = hasher(key) % data.size();
        for (auto iter = data[bucket_id].begin(); iter != data[bucket_id].end(); ++iter) {
            if (iter->first == key) {
                return iterator(&data, data.begin() + bucket_id, iter);
            }
        }
        return end();
    }
    // exit ITERATOR



    // entry CONST_ITERATOR 
    class const_iterator {
        const std::vector<std::list<Pair>>* data_pointer;
        typename std::vector<std::list<Pair>>::const_iterator bucket;
        typename std::list<Pair>::const_iterator value;


    public:
        const_iterator() = default;
        const_iterator(const const_iterator &other)
            : data_pointer(other.data_pointer)
            , bucket(other.bucket)
            , value(other.value) {}
        const_iterator(
            const typename std::vector<std::list<Pair>>* _data_pointer
            , typename std::vector<std::list<Pair>>::const_iterator _bucket
            , typename std::list<Pair>::const_iterator _value)
        : data_pointer(_data_pointer)
        , bucket(_bucket)
        , value(_value) {};

        const const_iterator& operator++ () {
            if (bucket == data_pointer->end()) {
                // if we actually at the end: do strange shit
                return *this;
            }
            ++value;
            if (value == bucket->end()) {
                while (true) {
                    ++bucket;
                    if (bucket == data_pointer->end()) {
                        value = data_pointer->begin()->begin();
                        break;
                    }
                    if (bucket->begin() != bucket->end()) {
                        // if list at least have 1 element
                        value = bucket->begin();
                        break;
                    }
                }
            }
            return *this;
        }

        const_iterator operator++ (int) {
            const_iterator old(*this);
            ++*this;
            return old;
        }
        bool operator== (const const_iterator& other) const {
            return (data_pointer == other.data_pointer
                && bucket == other.bucket
                && value == other.value);
        }
        bool operator!= (const const_iterator& other) const {
            return !(*this == other);
        }
        const Pair& operator* () const {
            return *value; 
        }
        const typename std::list<Pair>::const_iterator operator-> () const {
            return value;
        }
    };

    const_iterator begin() const {
        if (value_count != 0) {
            for (auto iter = data.begin(); iter != data.end(); ++iter) {
                if (iter->size() != 0) {
                    return const_iterator(&data, iter, iter->begin());
                }
            }
        }
        return end();
    }
    const_iterator end() const {
        return const_iterator(&data, data.end(), data.begin()->begin());
    }
    const_iterator find(const KeyType& key) const {
        size_t bucket_id = hasher(key) % data.size();
        for (auto iter = data[bucket_id].begin(); iter != data[bucket_id].end(); ++iter) {
            if (iter->first == key) {
                return const_iterator(&data, data.begin() + bucket_id, iter);
            }
        }
        return end();
    }
    // exit CONST_ITERATOR


    ValueType& operator[] (const KeyType& key) {
        size_t bucket_id = hasher(key) % data.size();
        for (auto& value : data[bucket_id]) {
            if (key == value.first) {
                return value.second;
            }
        }
        insert({key, ValueType()});
        bucket_id = hasher(key) % data.size();
        for (auto& value : data[bucket_id]) {
            if (key == value.first) {
                return value.second;
            }
        }
        __builtin_unreachable();
        // never will be completed if insert is correct;
    }

    const ValueType& at(const KeyType& key) const {
        size_t bucket_id = hasher(key) % data.size();
        for (const auto& value : data[bucket_id]) {
            if (key == value.first) {
                return value.second; 
            }
        }
        throw std::out_of_range("i'm from LGBTQIAPPSHRAJEKRFWJRTJRGDUWSBFAFJTDGD+ community");
    }
    /*
    // using only with #include <iostream>
    void helper_write_all() const {
        for (const auto& bucket : data) {
            std::cout << "bucket: ";
            for (const auto& value : bucket) {
                std::cout << '[' << value.first << ", " << value.second << "] ";
            }
            std::cout << '\n';
        }
    } */
    void clear() {
        data.clear();
        data.resize(baseCapacity);
        value_count = 0;
    }
};