#pragma once
#include "Bitset.hpp"
#include <array>

template <size_t W, size_t H>
class Bitmap
{
  private:
    using BitsetDataType = Bitset<W>;
    using BitmapDataType = std::array<BitsetDataType, H>;

  public:
    Bitmap() = default;
    ~Bitmap() = default;

    // Deleted copy constructor and assignment operator
    Bitmap(const Bitmap &) = delete;
    Bitmap &operator=(const Bitmap &) = delete;

    // Defaulted move constructor and assignment operator
    Bitmap(Bitmap &&) noexcept = default;
    Bitmap &operator=(Bitmap &&) noexcept = default;

    // Accessors
    inline BitmapDataType &getData()
    {
        return m_Data;
    }
    const BitmapDataType &getData() const
    {
        return m_Data;
    }

    BitsetDataType OperateAND() const;

  private:
    BitmapDataType m_Data;
};

template <size_t W, size_t H>
typename Bitmap<W, H>::BitsetDataType Bitmap<W, H>::OperateAND() const
{
    BitsetDataType result;
    result.set(); // Initialize all bits to 1
    for (const auto &bitset : m_Data)
    {
        result &= bitset;
    }
    return result;
}
