#ifndef CRYPTO_BIGNUMBER_HPP
#define CRYPTO_BIGNUMBER_HPP
#include"byte_array/basic_byte_array.hpp"
#include"byte_array/referenced_byte_array.hpp"
#include"byte_array/const_byte_array.hpp"
#include"assert.hpp"
#include<vector>
#include<algorithm>

namespace fetch {
namespace crypto {
  /* Implements a subset of big number functionality.
   * 
   * The purpose of this library is to implement a subset of number
   * functionalities that is handy when for instance computing
   * proof-of-work or other big uint manipulations.
   *
   * The implementation subclasses a <byte_array::BasicByteArray> such
   * one easily use this in combination with hashes etc.
   *
   * One further thing TODO, is to move this out of crypto as this also
   * have other applications such as effecient simulated annealing.
   */  
class BigUnsigned : public byte_array::BasicByteArray {
public:
  typedef byte_array::BasicByteArray super_type;

  BigUnsigned() {
    Resize( std::max( std::size_t( 256 >> 3 ), sizeof(uint64_t) ) );
    for(std::size_t i=0; i < sizeof(uint64_t) ; ++i) {
      super_type::operator[](i) = 0;
    }    
  }
  
  BigUnsigned(BigUnsigned const &other) : super_type( other.Copy() ) { }  
  BigUnsigned(super_type const &other) : super_type( other.Copy() ) { }
  
  BigUnsigned(uint64_t const& number, std::size_t size = 256) {
    Resize( std::max( size >> 3, sizeof(uint64_t) ) );    

    union {
      uint64_t value;
      uint8_t bytes[ sizeof(uint64_t) ];
    } data;
    data.value = number;

    // FIXME: Assumes little endian    
    for(std::size_t i = 0; i < sizeof( uint64_t ) ; ++i) {
      super_type::operator[]( i ) = data.bytes[i];
    }
  }

  BigUnsigned& operator=(BigUnsigned const &v) {
    super_type::operator=(v);
    return *this;
  }

  BigUnsigned& operator=(byte_array::BasicByteArray const &v) {
    super_type::operator=(v);
    return *this;    
  }

  BigUnsigned& operator=(byte_array::ByteArray const &v) {
    super_type::operator=(v);
    return *this;
  }

  BigUnsigned& operator=(byte_array::ConstByteArray const &v) {
    super_type::operator=(v);
    return *this;
  }
  

  
  template< typename T >
  BigUnsigned& operator=(T const &v) {
    union {
      uint64_t value;
      uint8_t bytes[ sizeof(T) ];
    } data;
    data.value = v;

    if( sizeof(T) > super_type::size()) {
      TODO_FAIL( "BIg number too small" );
    }
    
    // FIXME: Assumes little endian
    std::size_t i = 0;
    for(; i < sizeof( T ) ; ++i) {
      super_type::operator[]( i ) = data.bytes[i];
    }
    
    for(; i < super_type::size() ; ++i) {
      super_type::operator[]( i ) = 0;
    }    
    return *this;
  }
  
  BigUnsigned& operator++() {
    // TODO: Propagation of carry bits this way is slow.
    // If we instead make sure that the size is always a multiple of 8
    // We can do the logic in uint64_t
    // In fact, we should re implement this using vector registers.
    std::size_t i = 0;
    uint8_t val = ++super_type::operator[]( i ) ; 
    while( val == 0 ) {
      ++i;
      if( i == super_type::size() ) {
        TODO_FAIL("Throw error, size too little");
        val = super_type::operator[]( i ) = 1;
      } else {
        val = ++super_type::operator[]( i ) ;
      }
    }
    
    return *this;
  }

  BigUnsigned& operator<<=(std::size_t const &n) {
    std::size_t bits = n & 7;
    std::size_t bytes = n >> 3;
    std::size_t old = super_type::size() - bytes;


    for(std::size_t i=old; i != 0; ) {
      --i;
      super_type::operator[](i+bytes) = super_type::operator[](i);
    }
    
    for(std::size_t i=0; i< bytes; ++i) {
      super_type::operator[](i) = 0;
    }

    std::size_t nbits = 8 - bits;
    uint8_t carry = 0;    
    for(std::size_t i=0; i < size(); ++i) {
      uint8_t val = super_type::operator[](i);

      super_type::operator[](i) =  (val << bits) | carry;
      carry = val >> nbits;
    }

    return *this;
  }

  uint8_t operator[](std::size_t const &n) const {
    return super_type::operator[](n);
  }

  bool operator<(BigUnsigned const &other) const {
    std::size_t s1 = TrimmedSize();
    std::size_t s2 = other.TrimmedSize();

    if( s1 != s2) {
      return s1 < s2;
    }
    if(s1 == 0) return false;

    --s1;
    while( (s1 != 0)  && (super_type::operator[](s1) == other[s1]) ) {
      --s1;
    }
    
    return super_type::operator[](s1) < other[s1];
  }
  
  bool operator>(BigUnsigned const &other) const {
    return other < (*this);
  }

  std::size_t TrimmedSize() const {
    std::size_t ret = super_type::size();
    while( (ret != 0) && (super_type::operator[](ret -1) == 0) ) {
      --ret;
    }
    return ret;
  }
  
};

};
};

#endif
