
/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2007       Bastian Wemmenhove
 *  Copyright (C) 2007-2009  Joris Mooij  [joris dot mooij at libdai dot org]
 *  Copyright (C) 2007       Radboud University Nijmegen, The Netherlands
 */


/// \file
/// \brief Defines class MR, which implements loop corrections as proposed by Montanari and Rizzo


#ifndef __defined_libdai_mr_h
#define __defined_libdai_mr_h


#include <vector>
#include <string>
#include <dai/factorgraph.h>
#include <dai/daialg.h>
#include <dai/enum.h>
#include <dai/properties.h>
#include <dai/exceptions.h>
#include <boost/dynamic_bitset.hpp>


namespace dai {


/// Approximate inference algorithm by Montanari and Rizzo [\ref MoR05]
/** \author Bastian Wemmenhove wrote the original implementation before it was merged into libDAI
 *  \todo Clean up code (use a BipartiteGraph-like implementation for the graph structure, and BBP for response propagation).
 */
class MR : public DAIAlgFG {
    private:
        /// Is the underlying factor graph supported?
        bool supported;
        /// con[i] = connectivity of spin \a i
        std::vector<size_t>                             con;
        /// nb[i] are the neighbours of spin \a i
        std::vector<std::vector<size_t> >               nb;
        /// tJ[i][_j] is the hyperbolic tangent of the interaction between spin \a i and its neighbour nb[i][_j]
        std::vector<std::vector<Real> >                 tJ;
        /// theta[i] is the local field on spin \a i
        std::vector<Real>                               theta;
        /// M[i][_j] is \f$ M^{(i)}_j \f$
        std::vector<std::vector<Real> >                 M;
        /// The \a _j 'th neighbour of spin \a i has spin \a i as its kindex[i][_j]'th neighbour
        std::vector<std::vector<size_t> >               kindex;
        /// Cavity correlations
        std::vector<std::vector<std::vector<Real> > >   cors;
        /// Maximum connectivity
        static const size_t kmax = 31;
        /// Type used for managing a subset of neighbors
        typedef boost::dynamic_bitset<> sub_nb;
        /// Number of variables (spins)
        size_t N;
        /// Magnetizations
        std::vector<Real> Mag;
        /// Maximum difference encountered so far
        Real _maxdiff;
        /// Number of iterations needed
        size_t _iters;

    public:
        /// Parameters for MR
        struct Properties {
            /// Enumeration of different types of update equations
            /** The possible update equations are:
             *  - FULL full updates, slow but accurate
             *  - LINEAR linearized updates, faster but less accurate
             */
            DAI_ENUM(UpdateType,FULL,LINEAR);

            /// Enumeration of different ways of initializing the cavity correlations
            /** The possible cavity initializations are:
             *  - RESPPROP using response propagation ("linear response")
             *  - CLAMPING using clamping and BP
             *  - EXACT using JunctionTree
             */
            DAI_ENUM(InitType,RESPPROP,CLAMPING,EXACT);

            /// Verbosity (amount of output sent to stderr)
            size_t verbose;

            /// Tolerance for convergence test
            Real tol;

            /// Update equations
            UpdateType updates;

            /// How to initialize the cavity correlations
            InitType inits;
        } props;

        /// Name of this inference method
        static const char *Name;

    public:
        /// Default constructor
        MR() : DAIAlgFG(), supported(), con(), nb(), tJ(), theta(), M(), kindex(), cors(), N(), Mag(), _maxdiff(), _iters(), props() {}

        /// Construct from FactorGraph \a fg and PropertySet \a opts
        /** \param opts Parameters @see Properties
         *  \note This implementation only deals with binary variables and pairwise interactions.
         *  \throw NOT_IMPLEMENTED if \a fg has factors depending on three or more variables or has variables with more than two possible states.
         */
        MR( const FactorGraph &fg, const PropertySet &opts );


    /// \name General InfAlg interface
    //@{
        virtual MR* clone() const { return new MR(*this); }
        virtual std::string identify() const;
        virtual Factor belief( const Var &v ) const { return beliefV( findVar( v ) ); }
        virtual Factor belief( const VarSet &/*vs*/ ) const { DAI_THROW(NOT_IMPLEMENTED); return Factor(); }
        virtual Factor beliefV( size_t i ) const;
        virtual std::vector<Factor> beliefs() const;
        virtual Real logZ() const { DAI_THROW(NOT_IMPLEMENTED); return 0.0; }
        virtual void init() {}
        virtual void init( const VarSet &/*ns*/ ) { DAI_THROW(NOT_IMPLEMENTED); }
        virtual Real run();
        virtual Real maxDiff() const { return _maxdiff; }
        virtual size_t Iterations() const { return _iters; }
        virtual void setProperties( const PropertySet &opts );
        virtual PropertySet getProperties() const;
        virtual std::string printProperties() const;
    //@}

    private:
        /// Returns the signum of \a a
        Real sign(Real a) { return (a >= 0) ? 1.0 : -1.0; }

        /// Initialize N, con, nb, tJ, theta
        void init(size_t Nin, Real *_w, Real *_th);
        
        /// Initialize kindex
        void makekindex();
        
        /// Initialize cors
        Real init_cor();
        
        /// Calculate cors using response propagation
        Real init_cor_resp();
        
        /// Iterate update equations for cavity fields
        void solvemcav();
        
        /// Calculate magnetizations
        void solveM();

        /// Calculate the product of all tJ[i][_j] for _j in A
        /** \param i variable index
         *  \param A subset of neighbors of variable \a i
         */
        Real _tJ(size_t i, sub_nb A);
        
        /// Calculate \f$ \Omega^{(i)}_{j,l} \f$ as defined in [\ref MoR05] eqn. (2.15)
        Real Omega(size_t i, size_t _j, size_t _l);
        
        /// Calculate \f$ T^{(i)}_A \f$ as defined in [\ref MoR05] eqn. (2.17) with \f$ A = \{l_1,l_2,\dots\} \f$
        /** \param i variable index
         *  \param A subset of neighbors of variable \a i
         */
        Real T(size_t i, sub_nb A);
        
        /// Calculates \f$ T^{(i)}_j \f$ where \a j is the \a _j 'th neighbor of \a i
        Real T(size_t i, size_t _j);
        
        /// Calculates \f$ \Gamma^{(i)}_{j,l_1l_2} \f$ as defined in [\ref MoR05] eqn. (2.16)
        Real Gamma(size_t i, size_t _j, size_t _l1, size_t _l2);
        
        /// Calculates \f$ \Gamma^{(i)}_{l_1l_2} \f$ as defined in [\ref MoK07] on page 1141
        Real Gamma(size_t i, size_t _l1, size_t _l2);
        
        /// Approximates moments of variables in \a A
        /** Calculate the moment of variables in \a A from M and cors, neglecting higher order cumulants,
         *  defined as the sum over all partitions of A into subsets of cardinality two at most of the
         *  product of the cumulants (either first order, i.e. M, or second order, i.e. cors) of the
         *  entries of the partitions.
         *
         *  \param i variable index
         *  \param A subset of neighbors of variable \a i
         */
        Real appM(size_t i, sub_nb A);
        
        /// Calculate sum over all even/odd subsets B of \a A of _tJ(j,B) appM(j,B)
        /** \param j variable index
         *  \param A subset of neighbors of variable \a j
         *  \param sum_even on return, will contain the sum over all even subsets
         *  \param sum_odd on return, will contain the sum over all odd subsets
         */
        void sum_subs(size_t j, sub_nb A, Real *sum_even, Real *sum_odd);
};


} // end of namespace dai


#endif