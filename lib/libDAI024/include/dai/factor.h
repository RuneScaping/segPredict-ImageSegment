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
/// \brief Defines TFactor<> and Factor classes which represent factors in probability distributions.


#ifndef __defined_libdai_factor_h
#define __defined_libdai_factor_h


#include <iostream>
#include <functional>
#include <cmath>
#include <dai/prob.h>
#include <dai/varset.h>
#include <dai/index.h>
#include <dai/util.h>


namespace dai {


/// Represents a (probability) factor.
/** Mathematically, a \e factor is a function mapping joint states of some
 *  variables to the nonnegative real numbers.
 *  More formally, denoting a discrete variable with label \f$l\f$ by
 *  \f$x_l\f$ and its state space by \f$X_l = \{0,1,\dots,S_l-1\}\f$,
 *  a factor depending on the variables \f$\{x_l\}_{l\in L}\f$ is
 *  a function \f$f_L : \prod_{l\in L} X_l \to [0,\infty)\f$.
 *
 *  In libDAI, a factor is represented by a TFactor<T> object, which has two
 *  components:
 *  \arg a VarSet, corresponding with the set of variables \f$\{x_l\}_{l\in L}\f$
 *  that the factor depends on;
 *  \arg a TProb, a vector containing the value of the factor for each possible
 *  joint state of the variables.
 *
 *  The factor values are stored in the entries of the TProb in a particular
 *  ordering, which is defined by the one-to-one correspondence of a joint state
 *  in \f$\prod_{l\in L} X_l\f$ with a linear index in
 *  \f$\{0,1,\dots,\prod_{l\in L} S_l-1\}\f$ according to the mapping \f$\sigma\f$
 *  induced by dai::calcLinearState().
 *
 *  \tparam T Should be a scalar that is castable from and to double and should support elementary arithmetic operations.
 *  \todo Define a better fileformat for .fg files (maybe using XML)?
 *  \todo Add support for sparse factors.
 */
template <typename T> class TFactor {
    private:
        /// Stores the variables on which the factor depends
        VarSet      _vs;
        /// Stores the factor values
        TProb<T>    _p;

    public:
    /// \name Constructors and destructors
    //@{
        /// Constructs factor depending on no variables with value \a p
        TFactor ( T p = 1 ) : _vs(), _p(1,p) {}

        /// Constructs factor depending on the variable \a v with uniform distribution
        TFactor( const Var &v ) : _vs(v), _p(v.states()) {}

        /// Constructs factor depending on variables in \a vars with uniform distribution
        TFactor( const VarSet& vars ) : _vs(vars), _p(_vs.nrStates()) {}

        /// Constructs factor depending on variables in \a vars with all values set to \a p
        TFactor( const VarSet& vars, T p ) : _vs(vars), _p(_vs.nrStates(),p) {}

        /// Constructs factor depending on variables in \a vars, copying the values from a std::vector<>
        /** \tparam S Type of values of \a x
         *  \param vars contains the variables that the new factor should depend on.
         *  \param x Vector with values to be copied.
         */
        template<typename S>
        TFactor( const VarSet& vars, const std::vector<S> &x ) : _vs(vars), _p(x.begin(), x.begin() + _vs.nrStates(), _vs.nrStates()) {
            DAI_ASSERT( x.size() == vars.nrStates() );
        }

        /// Constructs factor depending on variables in \a vars, copying the values from an array
        /** \param vars contains the variables that the new factor should depend on.
         *  \param p Points to array of values to be added.
         */
        TFactor( const VarSet& vars, const T* p ) : _vs(vars), _p(p, p + _vs.nrStates(), _vs.nrStates()) {}

        /// Constructs factor depending on variables in \a vars, copying the values from \a p
        TFactor( const VarSet& vars, const TProb<T> &p ) : _vs(vars), _p(p) {
            DAI_DEBASSERT( _vs.nrStates() == _p.size() );
        }

        /// Constructs factor depending on variables in \a vars, permuting the values given in \a p accordingly
        TFactor( const std::vector<Var> &vars, const std::vector<T> &p ) : _vs(vars.begin(), vars.end(), vars.size()), _p(p.size()) {
            Permute permindex(vars);
            for( size_t li = 0; li < p.size(); ++li )
                _p[permindex.convertLinearIndex(li)] = p[li];
        }
    //@}

    /// \name Queries
    //@{
        /// Returns constant reference to value vector
        const TProb<T>& p() const { return _p; }

        /// Returns reference to value vector
        TProb<T>& p() { return _p; }

        /// Returns a copy of the \a i 'th entry of the value vector
        T operator[] (size_t i) const { return _p[i]; }

        /// Returns a reference to the \a i 'th entry of the value vector
        T& operator[] (size_t i) { return _p[i]; }

        /// Returns constant reference to variable set (i.e., the variables on which the factor depends)
        const VarSet& vars() const { return _vs; }

        /// Returns reference to variable set (i.e., the variables on which the factor depends)
        VarSet& vars() { return _vs; }

        /// Returns the number of possible joint states of the variables on which the factor depends, \f$\prod_{l\in L} S_l\f$
        /** \note This is equal to the length of the value vector.
         */
        size_t states() const { return _p.size(); }

        /// Returns the Shannon entropy of \c *this, \f$-\sum_i p_i \log p_i\f$
        T entropy() const { return _p.entropy(); }

        /// Returns maximum of all values
        T max() const { return _p.max(); }

        /// Returns minimum of all values
        T min() const { return _p.min(); }

        /// Returns sum of all values
        T sum() const { return _p.sum(); }

        /// Returns maximum absolute value of all values
        T maxAbs() const { return _p.maxAbs(); }

        /// Returns \c true if one or more values are NaN
        bool hasNaNs() const { return _p.hasNaNs(); }

        /// Returns \c true if one or more values are negative
        bool hasNegatives() const { return _p.hasNegatives(); }

        /// Returns strength of this factor (between variables \a i and \a j), as defined in eq. (52) of [\ref MoK07b]
        T strength( const Var &i, const Var &j ) const;
    //@}

    /// \name Unary transformations
    //@{
        /// Returns pointwise absolute value
        TFactor<T> abs() const {
            // Note: the alternative (shorter) way of implementing this,
            //   return TFactor<T>( _vs, _p.abs() );
            // is slower because it invokes the copy constructor of TProb<T>
            TFactor<T> x;
            x._vs = _vs;
            x._p = _p.abs();
            return x;
        }

        /// Returns pointwise exponent
        TFactor<T> exp() const {
            TFactor<T> x;
            x._vs = _vs;
            x._p = _p.exp();
            return x;
        }

        /// Returns pointwise logarithm
        /** If \a zero == \c true, uses <tt>log(0)==0</tt>; otherwise, <tt>log(0)==-Inf</tt>.
         */
        TFactor<T> log(bool zero=false) const {
            TFactor<T> x;
            x._vs = _vs;
            x._p = _p.log(zero);
            return x;
        }
        
        /// Returns pointwise inverse
        /** If \a zero == \c true, uses <tt>1/0==0</tt>; otherwise, <tt>1/0==Inf</tt>.
         */
        TFactor<T> inverse(bool zero=true) const {
            TFactor<T> x;
            x._vs = _vs;
            x._p = _p.inverse(zero);
            return x;
        }

        /// Returns normalized copy of \c *this, using the specified norm
        /** \throw NOT_NORMALIZABLE if the norm is zero
         */
        TFactor<T> normalized( typename TProb<T>::NormType norm=TProb<T>::NORMPROB ) const {
            TFactor<T> x;
            x._vs = _vs;
            x._p = _p.normalized( norm );
            return x;
        }
    //@}

    /// \name Unary operations
    //@{
        /// Draws all values i.i.d. from a uniform distribution on [0,1)
        TFactor<T> & randomize () { _p.randomize(); return *this; }

        /// Sets all values to \f$1/n\f$ where \a n is the number of states
        TFactor<T>& setUniform () { _p.setUniform(); return *this; }

        /// Normalizes factor using the specified norm
        /** \throw NOT_NORMALIZABLE if the norm is zero
         */
        T normalize( typename TProb<T>::NormType norm=TProb<T>::NORMPROB ) { return _p.normalize( norm ); }
    //@}

    /// \name Operations with scalars
    //@{
        /// Sets all values to \a x
        TFactor<T> & fill (T x) { _p.fill( x ); return *this; }

        /// Adds scalar \a x to each value
        TFactor<T>& operator+= (T x) { _p += x; return *this; }

        /// Subtracts scalar \a x from each value
        TFactor<T>& operator-= (T x) { _p -= x; return *this; }

        /// Multiplies each value with scalar \a x
        TFactor<T>& operator*= (T x) { _p *= x; return *this; }

        /// Divides each entry by scalar \a x
        TFactor<T>& operator/= (T x) { _p /= x; return *this; }

        /// Raises values to the power \a x
        TFactor<T>& operator^= (T x) { _p ^= x; return *this; }
    //@}

    /// \name Transformations with scalars
    //@{
        /// Returns sum of \c *this and scalar \a x
        TFactor<T> operator+ (T x) const {
            // Note: the alternative (shorter) way of implementing this,
            //   TFactor<T> result(*this);
            //   result._p += x;
            // is slower because it invokes the copy constructor of TFactor<T>
            TFactor<T> result;
            result._vs = _vs;
            result._p = p() + x;
            return result;
        }

        /// Returns difference of \c *this and scalar \a x
        TFactor<T> operator- (T x) const {
            TFactor<T> result;
            result._vs = _vs;
            result._p = p() - x;
            return result;
        }

        /// Returns product of \c *this with scalar \a x
        TFactor<T> operator* (T x) const {
            TFactor<T> result;
            result._vs = _vs;
            result._p = p() * x;
            return result;
        }

        /// Returns quotient of \c *this with scalar \a x
        TFactor<T> operator/ (T x) const {
            TFactor<T> result;
            result._vs = _vs;
            result._p = p() / x;
            return result;
        }

        /// Returns \c *this raised to the power \a x
        TFactor<T> operator^ (T x) const {
            TFactor<T> result;
            result._vs = _vs;
            result._p = p() ^ x;
            return result;
        }
    //@}

    /// \name Operations with other factors
    //@{
        /// Applies bina