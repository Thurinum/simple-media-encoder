#ifndef EITHER_H
#define EITHER_H

#include <stdexcept>

//!
//! \brief A simple monadic class that can hold one of two types at a time.
//! \tparam R The right type. This usually represents a "valid" or "successful result" type.
//! \tparam L The left type. This usually represents an "invalid" or "error result" type.
//!
template <typename R, typename L>
class Either
{
public:
    Either(const L& left)
        : isLeft(true)
        , left(left) { }

    Either(const R& right)
        : isLeft(false)
        , right(right) { }

    ~Either()
    {
        if (isLeft)
            left.~L();
        else
            right.~R();
    }

    const L& getLeft() const
    {
        if (!isLeft)
            throw std::logic_error("Either::getLeft() called on a right value.");

        return left;
    }
    const R& getRight() const
    {
        if (isLeft)
            throw std::logic_error("Either::getRight() called on a left value.");

        return right;
    }
    auto value() const
    {
        return isLeft ? left : right;
    }

    template <typename F, typename G>
    auto fold(const F&& rightFunc, const G&& leftFunc) const
    {
        return isLeft ? leftFunc(left) : rightFunc(right);
    }

private:
    const union {
        L left;
        R right;
    };

    const bool isLeft;
};

#endif
