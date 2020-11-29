#pragma once

#include <stddef.h>

/**
 * @file rc.h
 * @brief Reference counting system.
 *
 * The reference counting system closely emulates the behavior of std::shared_ptr<T> and
 * std::weak_ptr<T> from C++. The allocater, rc_alloc(), is a wrapper around calloc() and allocates
 * size for both the object and a reference counting object. Although the reference counting object
 * is not explicitly part of any struct in this library, it lives implicitly 'above' the object in
 * memory. The reference counting object stores both a 'strong_count' variable that keeps track of
 * owning references, and a 'weak_count" variable that keeps track of non-owning references. When
 * 'strong_count' hits 0, the object itself is set to NULL, but the reference counter itself is not
 * freed unless 'weak_count' is also 0. Objects that hold weak references can check if the object is
 * still valid via rc_weak_check().
 *
 * @code{.c}
 * int * num = rc_alloc(1, NULL);
 * assert(rc_count(num) == 1);
 * int * num2 = rc_weak_retain(num);
 * assert(rc_weak_count(num) == 1);
 * rc_strong_release((void*)&num);
 * num2 = rc_weak_check((void*)&num2);
 * assert(num2 == NULL);
 * @endcode
 */

/**
 *  @brief A function variable that points to the destruction function of an object
 */
typedef void (*Destructor)(void *);

/**
 * @brief A wrapper around calloc() that adds reference counting.
 * @see rc_array_alloc if you want to add reference counting to an array.
 * @param size The size of the object.
 * @param destructor A function pointer to the device's destructor.
 * @return A pointer to the object that was allocated.
 */
void *rc_alloc(size_t size, Destructor destructor);

/**
 * @brief Increment the strong reference count of an object.
 * @param obj A pointer to the object.
 * @return A pointer to the object.
 */
void *rc_strong_retain(void *obj);

/**
 * @brief Decrement the strong reference count of an object.
 * @param obj The value of the object's pointer.
 */
void rc_strong_release(void **obj);

/**
 * @brief Get the number of strong references for an object.
 * @param obj A pointer to the object.
 * @return The number of references for the object.
 */
size_t rc_strong_count(void *obj);

/**
 * @brief Increment the weak reference count of an object.
 * @param obj A pointer to the object.
 * @return A pointer to the object.
 */
void *rc_weak_retain(void *obj);

/**
 * @brief Decrement the weak reference count of an object.
 * @param obj The value of the object's pointer.
 */
void rc_weak_release(void **obj);

/**
 * @brief Get the number of weak references for an object.
 * @param obj A pointer to the object.
 * @return The number of references for the object.
 */
size_t rc_weak_count(void *obj);

/**
 * @brief Check validity of a weak reference.
 *
 * If the object is no longer valid, the weak counter of the object will be decremented using
 * rc_weak_release(). Thus, if the last weak reference uses this function, the reference counter and
 * object will be properly de-allocated.
 *
 * @see rc_weak_release
 *
 * @param obj The value of the object's pointer.
 * @return A pointer to the object, NULL if the object is no longer valid.
 */
void *rc_weak_check(void **obj);

/**
 * @brief Convert a weak reference to a strong reference.
 *
 * This function decrements the weak reference counter, thus if both the weak and strong reference
 * count are 0, the reference counter and object must be deallocated. If the strong reference count
 * is 0 but the weak reference count is non-zero, a NULL object will be returned. It is vital to
 * check the value of the object after calling this function to ensure the validity. Otherwise, the
 * strong reference will be increment and the now strongly-reference object will be returned.
 *
 * @param obj The value of the object's pointer.
 * @return A pointer to the object, NULL if the object is no longer valid.
 */
void *rc_upgrade(void **obj);

/**
 * @brief Convert a strong reference to a weak reference.
 *
 * This function decrements the strong reference counter, so if the strong reference count has
 * reached 0, the object is no longer valid and will be set to NULL. If the weak reference count is
 * also 0, the reference counter and object will be de-allocated. Otherwise, the weak reference
 * counter is incremented.
 *
 * @param obj The value of the object's pointer.
 * @return A pointer to the object, NULL if the object is no longer valid.
 */
void *rc_downcast(void **obj);
