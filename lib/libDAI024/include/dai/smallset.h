
/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2002       Martijn Leisink  [martijn@mbfys.kun.nl]
 *  Copyright (C) 2006-2009  Joris Mooij      [joris dot mooij at libdai dot org]
 *  Copyright (C) 2002-2007  Radboud University Nijmegen, The Netherlands
 */


/// \file
/// \brief Defines the SmallSet<> class, which represents a set (optimized for a small number of elements).


#ifndef __defined_libdai_smallset_h
#define __defined_libdai_smallset_h


#include <vector>
#include <algorithm>


namespace dai {


/// Represents a set; the implementation is optimized for a small number of elements.
/** SmallSet uses an ordered <tt>std::vector<</tt><em>T</em><tt>></tt> to represent a set; this is faster than
 *  using a <tt>std::set<</tt><em>T</em><tt>></tt> if the number of elements is small.
 *  \tparam T Should be less-than-comparable.
 */
template <typename T>
class SmallSet {
    private:
        /// The elements in this set
        std::vector<T> _elements;

    public:
    /// \name Constructors and destructors
    //@{
        /// Default constructor (constructs an empty set)
        SmallSet() : _elements() {}

        /// Construct a set consisting of one element
        SmallSet( const T &t ) : _elements() {
            _elements.push_back( t );
        }

        /// Construct a set consisting of two elements
        SmallSet( const T &t1, const T &t2 ) {
            if( t1 < t2 ) {
                _elements.push_back( t1 );
                _elements.push_back( t2 );
            } else if( t2 < t1 ) {
                _elements.push_back( t2 );
                _elements.push_back( t1 );
            } else
                _elements.push_back( t1 );
        }

        /// Construct a SmallSet from a range of elements.
        /** \tparam TIterator Iterates over instances of type \a T.
         *  \param begin Points to first element to be added.
         *  \param end Points just beyond last element to be added.
         *  \param sizeHint For efficiency, the number of elements can be speficied by \a sizeHint.
         */
        template <typename TIterator>
        SmallSet( TIterator begin, TIterator end, size_t sizeHint=0 ) {
            _elements.reserve( sizeHint );
            _elements.insert( _elements.begin(), begin, end );
            std::sort( _elements.begin(), _elements.end() );
            typename std::vector<T>::iterator new_end = std::unique( _elements.begin(), _elements.end() );
            _elements.erase( new_end, _elements.end() );
        }
    //@}

    /// \name Operators for set-theoretic operations
    //@{
        /// Set-minus operator: returns all elements in \c *this, except those in \a x
        SmallSet operator/ ( const SmallSet& x ) const {
            SmallSet res;
            std::set_difference( _elements.begin(), _elements.end(), x._elements.begin(), x._elements.end(), inserter( res._elements, res._elements.begin() ) );
            return res;
        }

        /// Set-union operator: returns all elements in \c *this, plus those in \a x
        SmallSet operator| ( const SmallSet& x ) const {
            SmallSet res;
            std::set_union( _elements.begin(), _elements.end(), x._elements.begin(), x._elements.end(), inserter( res._elements, res._elements.begin() ) );
            return res;
        }

        /// Set-intersection operator: returns all elements in \c *this that are also contained in \a x
        SmallSet operator& ( const SmallSet& x ) const {
            SmallSet res;
            std::set_intersection( _elements.begin(), _elements.end(), x._elements.begin(), x._elements.end(), inserter( res._elements, res._elements.begin() ) );
            return res;
        }

        /// Erases from \c *this all elements in \a x
        SmallSet& operator/= ( const SmallSet& x ) {
            return (*this = (*this / x));
        }

        /// Erases one element
        SmallSet& operator/= ( const T &t ) {
            typename std::vector<T>::iterator pos = lower_bound( _elements.begin(), _elements.end(), t );
            if( pos != _elements.end() )
                if( *pos == t ) // found element, delete it
                    _elements.erase( pos );
            return *this;
        }

        /// Adds to \c *this all elements in \a x
        SmallSet& operator|= ( const SmallSet& x ) {
            return( *this = (*this | x) );
        }

        /// Adds one element
        SmallSet& operator|= ( const T& t ) {
            typename std::vector<T>::iterator pos = lower_bound( _elements.begin(), _elements.end(), t );
            if( pos == _elements.end() || *pos != t ) // insert it
                _elements.insert( pos, t );
            return *this;
        }

        /// Erases from \c *this all elements not in \a x
        SmallSet& operator&= ( const SmallSet& x ) {
            return (*this = (*this & x));
        }

        /// Returns \c true if \c *this is a subset of \a x
        bool operator<< ( const SmallSet& x ) const {
            return std::includes( x._elements.begin(), x._elements.end(), _elements.begin(), _elements.end() );
        }

        /// Returns \c true if \a x is a subset of \c *this
        bool operator>> ( const SmallSet& x ) const {
            return std::includes( _elements.begin(), _elements.end(), x._elements.begin(), x._elements.end() );
        }
    //@}

    /// \name Queries
    //@{
        /// Returns \c true if \c *this and \a x have elements in common
        bool intersects( const SmallSet& x ) const {
            return( (*this & x).size() > 0 );
        }

        /// Returns \c true if \c *this contains the element \a t
        bool contains( const T &t ) const {
            return std::binary_search( _elements.begin(), _elements.end(), t );
        }

        /// Returns number of elements
        typename std::vector<T>::size_type size() const { return _elements.size(); }

        /// Returns whether \c *this is empty
        bool empty() const { return _elements.size() == 0; }

        /// Returns reference to the elements
        std::vector<T>& elements() { return _elements; }

        /// Returns constant reference to the elements
        const std::vector<T>& elements() const { return _elements; }
    //@}

        /// Constant iterator over the elements
        typedef typename std::vector<T>::const_iterator const_iterator;
        /// Iterator over the elements
        typedef typename std::vector<T>::iterator iterator;
        /// Constant reverse iterator over the elements
        typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
        /// Reverse iterator over the elements
        typedef typename std::vector<T>::reverse_iterator reverse_iterator;

    /// \name Iterator interface
    //@{
        /// Returns iterator that points to the first element
        iterator begin() { return _elements.begin(); }
        /// Returns constant iterator that points to the first element
        const_iterator begin() const { return _elements.begin(); }

        /// Returns iterator that points beyond the last element
        iterator end() { return _elements.end(); }
        /// Returns constant iterator that points beyond the last element
        const_iterator end() const { return _elements.end(); }

        /// Returns reverse iterator that points to the last element
        reverse_iterator rbegin() { return _elements.rbegin(); }
        /// Returns constant reverse iterator that points to the last element
        const_reverse_iterator rbegin() const { return _elements.rbegin(); }

        /// Returns reverse iterator that points beyond the first element
        reverse_iterator rend() { return _elements.rend(); }
        /// Returns constant reverse iterator that points beyond the first element
        const_reverse_iterator rend() const { return _elements.rend(); }

        /// Returns reference to first element
        T& front() { return _elements.at(0); }
        /// Returns constant reference to first element
        const T& front() const { return _elements.at(0); }
    //@}

    /// \name Comparison operators
    //@{
        /// Returns \c true if \a a and \a b are identical
        friend bool operator==( const SmallSet &a, const SmallSet &b ) {
            return (a._elements == b._elements);
        }

        /// Returns \c true if \a a and \a b are not identical
        friend bool operator!=( const SmallSet &a, const SmallSet &b ) {
            return !(a._elements == b._elements);
        }

        /// Lexicographical comparison of elements
        friend bool operator<( const SmallSet &a, const SmallSet &b ) {
            return a._elements < b._elements;
        }
    //@}
};


} // end of namespace dai


#endif