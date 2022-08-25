/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2006-2009  Joris Mooij  [joris dot mooij at libdai dot org]
 *  Copyright (C) 2006-2007  Radboud University Nijmegen, The Netherlands
 */


/// \file
/// \brief Defines TProb<> and Prob classes which represent (probability) vectors (e.g., probability distributions of discrete random variables)


#ifndef __defined_libdai_prob_h
#define __defined_libdai_prob_h


#include <cmath>
#include <vector>
#include <ostream>
#include <algorithm>
#include <numeric>
#include <functional>
#include <dai/util.h>
#include <dai/exceptions.h>


namespace dai {


/// Function object that returns the value itself
template<typename T> struct fo_id : public std::unary_function<T, T> {
    /// Returns \a x
    T operator()( const T &x ) const {
        return x;
    }
};


/// Function object that takes the absolute value
template<typename T> struct fo_abs : public std::unary_function<T, T> {
    /// Returns abs(\a x)
    T operator()( const T &x ) const {
        if( x < (T)0 )
            return -x;
        else
            return x;
    }
};


/// Function object that takes the exponent
template<typename T> struct fo_exp : public std::unary_function<T, T> {
    /// Returns exp(\a x)
    T operator()( const T &x ) const {
        return exp( x );
    }
};


/// Function object that takes the logarithm
template<typename T> struct fo_log : public std::unary_function<T, T> {
    /// Returns log(\a x)
    T operator()( const T &x ) const {
        return log( x );
    }
};


/// Function object that takes the logarithm, except that log(0) is defined to be 0
template<typename T> struct fo_log0 : public std::unary_function<T, T> {
    /// Returns (\a x == 0 ? 0 : log(\a x))
    T operator()( const T &x ) const {
        if( x )
            return log( x );
        else
            return 0;
    }
};


/// Function object that takes the inverse
template<typename T> struct fo_inv : public std::unary_function<T, T> {
    /// Returns 1 / \a x
    T operator()( const T &x ) const {
        return 1 / x;
    }
};


/// Function object that takes the inverse, except that 1/0 is defined to be 0
template<typename T> struct fo_inv0 : public std::unary_function<T, T> {
    /// Returns (\a x == 0 ? 0 : (1 / \a x))
    T operator()( const T &x ) const {
        if( x )
            return 1 / x;
        else
            return 0;
    }
};


/// Function object that returns p*log0(p)
template<typename T> struct fo_plog0p : public std::unary_function<T, T> {
    /// Returns \a p * log0(\a p)
    T operator()( const T &p ) const {
        return p * dai::log0(p);
    }
};


/// Function object similar to std::divides(), but different in that dividing by zero results in zero
template<typename T> struct fo_divides0 : public std::binary_function<T, T, T> {
    /// Returns (\a y == 0 ? 0 : (\a x / \a y))
    T operator()( const T &x, const T &y ) const {
        if( y == (T)0 )
            return (T)0;
        else
            return x / y;
    }
};


/// Function object useful for calculating the KL distance
template<typename T> struct fo_KL : public std::binary_function<T, T, T> {
    /// Returns (\a p == 0 ? 0 : (\a p * (log(\a p) - log(\a q))))
    T operator()( const T &p, const T &q ) const {
        if( p == (T)0 )
            return (T)0;
        else
            return p * (log(p) - log(q));
    }
};


/// Function object useful for calculating the Hellinger distance
template<typename T> struct fo_Hellinger : public std::binary_function<T, T, T> {
    /// Returns (sqrt(\a p) - sqrt(\a q))^2
    T operator()( const T &p, const T &q ) const {
        T x = sqrt(p) - sqrt(q);
        return x * x;
    }
};


/// Function object that returns x to the power y
template<typename T> struct fo_pow : public std::binary_function<T, T, T> {
    /// Returns (\a x ^ \a y)
    T operator()( const T &x, const T &y ) const {
        if( y != 1 )
            return std::pow( x, y );
        else
            return x;
    }
};


/// Function object that returns the maximum of two values
template<typename T> struct fo_max : public 