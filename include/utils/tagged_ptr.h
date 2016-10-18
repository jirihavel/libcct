#ifndef TAGGED_POINTER_H_INCLUDED
#define TAGGED_POINTER_H_INCLUDED

#include <cstdint>

#include <boost/assert.hpp>
#include <boost/integer/static_log2.hpp>

namespace utils {

template<typename T>
inline uintptr_t p2u(T * p) noexcept
{
    union { T * p; std::uintptr_t u; } caster = { .p = p };
    return caster.u;
}

template<typename T>
inline T * u2p(std::uintptr_t u) noexcept
{
    union { T * p; std::uintptr_t u; } caster = { .u = u };
    return caster.p;
}

/** \brief Raw pointer that uses its alignment to store additional data.
 * On some architectures, pointers are aligned, so lower bits are always zero.
 *
 * The exact alignment depends on architecture, compiler & settings.
 *
 * The interface tries to match smart pointer interface with additional methods for payload manipulation.
 *
 * \param T Pointed type.
 * \param TagBits Number of zero bits. Default value is calculated from alignment of T.
 */
template<
    typename T,
    std::size_t TagBits = boost::static_log2<alignof(T)>::value
>
class tagged_ptr;

namespace internal {

template<typename T, std::size_t TagBits>
class tagged_ptr_base
{
protected :
    typedef T elem_type;
    typedef T * ptr_type;

    typedef std::uintptr_t tag_type;

    static constexpr std::size_t tag_bits = TagBits;
    static constexpr std::uintptr_t tag_mask = (1<<tag_bits)-1;
    static constexpr std::uintptr_t ptr_mask = ~tag_mask;

    static bool ptr_aligned(ptr_type p) noexcept
    {
        return (p2u(p) & tag_mask) == 0;
    }
private :
    std::uintptr_t data;
protected :
    constexpr tagged_ptr_base() = default;

    explicit constexpr tagged_ptr_base(std::nullptr_t) noexcept
    : data(0) {}

    explicit tagged_ptr_base(ptr_type p) noexcept
    : data(p2u(p)) {}

    tagged_ptr_base(ptr_type p, tag_type t) noexcept
    : data(p2u(p) | t) {}

    void set(ptr_type p, tag_type t) noexcept
    {
        data = p2u(p) | t;
    }

    void set(ptr_type p) noexcept
    {
        data = p2u(p) | (data & tag_mask);
    }

    ptr_type get() const noexcept
    {
        return u2p<elem_type>(data & ptr_mask);
    }

    void set_tag(tag_type t) noexcept
    {
        data = (data & ptr_mask) | t;
    }

    tag_type get_tag() const noexcept
    {
        return data & tag_mask;
    }

    ptr_type get_untagged() const noexcept
    {
        return u2p<elem_type>(data);
    }
};

}//namespace internal

template<
    typename T,
    std::size_t TagBits
>
class tagged_ptr
    : private internal::tagged_ptr_base<T, TagBits>
{
    typedef internal::tagged_ptr_base<T, TagBits> base;
public :
    typedef typename base::elem_type element_type;
    typedef typename base::ptr_type  pointer_type;
    typedef typename base::tag_type  tag_type;

    /** \brief Default constructor
     * pointer=?, tag=?
     */
    constexpr tagged_ptr() = default;

    /** \brief Construct from nullptr
     * pointer=nullptr, tag=0
     */
    explicit constexpr tagged_ptr(std::nullptr_t) noexcept
        : base(nullptr) {}

    /** \brief Construct from pointer
     * pointer=p, tag=0
     * p must be aligned
     */
    explicit tagged_ptr(pointer_type p) noexcept
        : base(p)
    {
        BOOST_ASSERT(base::ptr_aligned(p));
    }

    /** \brief Construct from tag
     * pointer=nullptr, tag=t
     */
    explicit tagged_ptr(tag_type t) noexcept
        : base(nullptr, t)
    {
        BOOST_ASSERT(t <= base::tag_mask);
    }

    /** \brief Construct from pointer + tag
     * pointer=p, tag=t
     */
    tagged_ptr(pointer_type p, tag_type t) noexcept
        : base(p, t)
    {
        BOOST_ASSERT(base::ptr_aligned(p));
        BOOST_ASSERT(t <= base::tag_mask);
    }

    /** \brief Set contents
     */
    void set(pointer_type p, tag_type t) noexcept
    {
        BOOST_ASSERT(base::ptr_aligned(p));
        BOOST_ASSERT(t <= base::tag_mask);
        base::set(p, t);
    }

    /** \brief Set pointer
     */
    void set(pointer_type p) noexcept
    {
        BOOST_ASSERT(base::ptr_aligned(p));
        base::set(p);
    }

    /** \brief Get pointer
     */
    pointer_type get() const noexcept
    {
        return base::get();
    }

    /** \brief Set tag
     */
    void set_tag(tag_type t) const noexcept
    {
        BOOST_ASSERT(t <= base::tag_mask);
        return base::set_tag(t);
    }

    /** \brief Get tag
     */
    tag_type get_tag() const noexcept
    {
        return base::get_tag();
    }

    /** \brief Get ptr, assuming tag == 0 */
    pointer_type get_untagged() const noexcept
    {
        BOOST_ASSERT(get_tag() == 0);
        return base::get_untagged();
    }

    /** \brief Check pointer
     * pointer != nullptr
     */
    explicit operator bool() const noexcept
    {
        return base::get();
    }

    /** \brief Check pointer
     * pointer != nullptr
     */
    bool operator!() const noexcept
    {
        return !base::get();
    }

    /** \brief Dereference object
     * Exclusive for single objects
     */
    element_type & operator*() const noexcept
    {
        return *base::get();
    }

    /** \brief Dereference object member
     * Exclusive for single objects
     */
    element_type * operator->() const noexcept
    {
        return base::get();
    }
};

template<
    typename T,
    std::size_t TagBits
>
class tagged_ptr<T[], TagBits> : private internal::tagged_ptr_base<T, TagBits>
{
    typedef internal::tagged_ptr_base<T, TagBits> base;
public :
    typedef typename base::elem_type element_type;
    typedef typename base::ptr_type  pointer_type;
    typedef typename base::tag_type  tag_type;

    /** \brief Default constructor
     * pointer=?, tag=?
     */
    constexpr tagged_ptr() = default;

    /** \brief Construct from nullptr
     * pointer=nullptr, tag=0
     */
    explicit constexpr tagged_ptr(std::nullptr_t) noexcept
    : base(nullptr) {}

    /** \brief Construct from pointer
     * pointer=p, tag=0
     * p must be aligned
     */
    explicit tagged_ptr(pointer_type p) noexcept
    : base(p)
    {
        BOOST_ASSERT(base::ptr_aligned(p));
    }

    /** \brief Construct from pointer + tag
     * pointer=p, tag=t
     */
    tagged_ptr(pointer_type p, tag_type t) noexcept
    : base(p, t)
    {
        BOOST_ASSERT(base::ptr_aligned(p));
        BOOST_ASSERT(t <= base::tag_mask);
    }

    /** \brief Set contents
     */
    void set(pointer_type p, tag_type t) noexcept
    {
        BOOST_ASSERT(base::ptr_aligned(p));
        BOOST_ASSERT(t <= base::tag_mask);
        base::set(p, t);
    }

    /** \brief Set pointer
     */
    void set(pointer_type p) noexcept
    {
        BOOST_ASSERT(base::ptr_aligned(p));
        base::set(p);
    }

    /** \brief Get pointer
     */
    pointer_type get() const noexcept
    {
        return base::get();
    }

    /** \brief Set tag
     */
    void set_tag(tag_type t) const noexcept
    {
        BOOST_ASSERT(t <= base::tag_mask);
        return base::set_tag(t);
    }

    /** \brief Get tag
     */
    tag_type get_tag() const noexcept
    {
        return base::get_tag();
    }

    /** \brief Get ptr, assuming tag == 0 */
    pointer_type get_untagged() const noexcept
    {
        BOOST_ASSERT(get_tag() == 0);
        return base::get_untagged();
    }

    /** \brief Check pointer
     * pointer != nullptr
     */
    explicit operator bool() const noexcept
    {
        return base::get();
    }

    /** \brief Check pointer
     * pointer != nullptr
     */
    bool operator!() const noexcept
    {
        return !base::get();
    }

    /** Offset access
     * Exclusive for arrays
     */
    element_type & operator[](size_t i) const noexcept
    {
        return base::get_ptr()[i];
    }
};

}//namespace utils

#endif//TAGGED_POINTER_H_INCLUDED
