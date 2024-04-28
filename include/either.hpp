#ifndef EITHER_H
#define EITHER_H

#include <stdexcept>
#include <variant>

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
        : value(std::in_place_type<L>, left) { }

    Either(const R& right)
        : value(std::in_place_type<R>, right) { }

    bool isLeft() const
    {
        return std::holds_alternative<L>(value);
    }

    const L& getLeft() const
    {
        if (!isLeft())
            throw std::logic_error("Either::getLeft() called on a right value.");

        return std::get<L>(value);
    }
    const R& getRight() const
    {
        if (isLeft())
            throw std::logic_error("Either::getRight() called on a left value.");

        return std::get<R>(value);
    }

    template <typename F, typename G>
    auto fold(const F&& rightFunc, const G&& leftFunc) const
    {
        return isLeft() ? leftFunc(getLeft()) : rightFunc(getRight());
    }

private:
    std::variant<L, R> value;
};

#endif
