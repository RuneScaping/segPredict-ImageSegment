
/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2006-2010  Joris Mooij  [joris dot mooij at libdai dot org]
 *  Copyright (C) 2006-2007  Radboud University Nijmegen, The Netherlands
 */


/// \file
/// \brief Defines class BP, which implements (Loopy) Belief Propagation


#ifndef __defined_libdai_bp_h
#define __defined_libdai_bp_h


#include <string>
#include <dai/daialg.h>
#include <dai/factorgraph.h>
#include <dai/properties.h>
#include <dai/enum.h>


namespace dai {


/// Approximate inference algorithm "(Loopy) Belief Propagation"
/** The Loopy Belief Propagation algorithm uses message passing
 *  to approximate marginal probability distributions ("beliefs") for variables
 *  and factors (more precisely, for the subset of variables depending on the factor).
 *  There are two variants, the sum-product algorithm (corresponding to 
 *  finite temperature) and the max-product algorithm (corresponding to 
 *  zero temperature).
 *
 *  The messages \f$m_{I\to i}(x_i)\f$ are passed from factors \f$I\f$ to variables \f$i\f$. 
 *  In case of the sum-product algorith, the update equation is: 
 *    \f[ m_{I\to i}(x_i) \propto \sum_{x_{N_I\setminus\{i\}}} f_I(x_I) \prod_{j\in N_I\setminus\{i\}} \prod_{J\in N_j\setminus\{I\}} m_{J\to j}\f]
 *  and in case of the max-product algorithm:
 *    \f[ m_{I\to i}(x_i) \propto \max_{x_{N_I\setminus\{i\}}} f_I(x_I) \prod_{j\in N_I\setminus\{i\}} \prod_{J\in N_j\setminus\{I\}} m_{J\to j}\f]
 *  In order to improve convergence, the updates can be damped. For improved numerical stability,
 *  the updates can be done in the log-domain alternatively.
 *
 *  After convergence, the variable beliefs are calculated by:
 *    \f[ b_i(x_i) \propto \prod_{I\in N_i} m_{I\to i}(x_i)\f]
 *  and the factor beliefs are calculated by:
 *    \f[ b_I(x_I) \propto f_I(x_I) \prod_{j\in N_I} \prod_{J\in N_j\setminus\{I\}} m_{J\to j}(x_j) \f]
 *  The logarithm of the partition sum is calculated by:
 *    \f[ \log Z = \sum_i (1 - |N_i|) \sum_{x_i} b_i(x_i) \log b_i(x_i) - \sum_I \sum_{x_I} b_I(x_I) \log \frac{b_I(x_I)}{f_I(x_I)} \f]
 *
 *  For the max-product algorithm, a heuristic way of finding the MAP state (the 
 *  joint configuration of all variables which has maximum probability) is provided
 *  by the findMaximum() method, which can be called after convergence.
 *
 *  \note There are two implementations, an optimized one (the default) which caches IndexFor objects,
 *  and a slower, less complicated one which is easier to maintain/understand. The slower one can be 
 *  enabled by defining DAI_BP_FAST as false in the source file.
 */
class BP : public DAIAlgFG {
    protected:
        /// Type used for index cache
        typedef std::vector<size_t> ind_t;
        /// Type used for storing edge properties
        struct EdgeProp {
            /// Index cached for this edge
            ind_t  index;
            /// Old message living on this edge
            Prob   message;
            /// New message living on this edge
            Prob   newMessage;
            /// Residual for this edge
            Real   residual;
        };
        /// Stores all edge properties
        std::vector<std::vector<EdgeProp> > _edges;
        /// Type of lookup table (only used for maximum-residual BP)
        typedef std::multimap<Real, std::pair<std::size_t, std::size_t> > LutType;
        /// Lookup table (only used for maximum-residual BP)
        std::vector<std::vector<LutType::iterator> > _edge2lut;
        /// Lookup table (only used for maximum-residual BP)
        LutType _lut;
        /// Maximum difference between variable beliefs encountered so far
        Real _maxdiff;
        /// Number of iterations needed
        size_t _iters;
        /// The history of message updates (only recorded if \a recordSentMessages is \c true)
        std::vector<std::pair<std::size_t, std::size_t> > _sentMessages;

    public:
        /// Parameters for BP
        struct Properties {
            /// Enumeration of possible update schedules
            /** The following update schedules have been defined:
             *  - PARALL parallel updates
             *  - SEQFIX sequential updates using a fixed sequence
             *  - SEQRND sequential updates using a random sequence
             *  - SEQMAX maximum-residual updates [\ref EMK06]
             */
            DAI_ENUM(UpdateType,SEQFIX,SEQRND,SEQMAX,PARALL);

            /// Enumeration of inference variants
            /** There are two inference variants:
             *  - SUMPROD Sum-Product
             *  - MAXPROD Max-Product (equivalent to Min-Sum)
             */
            DAI_ENUM(InfType,SUMPROD,MAXPROD);

            /// Verbosity (amount of output sent to stderr)
            size_t verbose;

            /// Maximum number of iterations
            size_t maxiter;

            /// Tolerance for convergence test
            Real tol;

            /// Whether updates should be done in logarithmic domain or not
            bool logdomain;

            /// Damping constant (0.0 means no damping, 1.0 is maximum damping)
            Real damping;

            /// Message update schedule
            UpdateType updates;

            /// Inference variant
            InfType inference;
        } props;

        /// Name of this inference algorithm
        static const char *Name;

        /// Specifies whether the history of message updates should be recorded
        bool recordSentMessages;

    public:
    /// \name Constructors/destructors
    //@{
        /// Default constructor
        BP() : DAIAlgFG(), _edges(), _edge2lut(), _lut(), _maxdiff(0.0), _iters(0U), _sentMessages(), props(), recordSentMessages(false) {}

        /// Construct from FactorGraph \a fg and PropertySet \a opts
        /** \param opts Parameters @see Properties
         */
        BP( const FactorGraph & fg, const PropertySet &opts ) : DAIAlgFG(fg), _edges(), _maxdiff(0.0), _iters(0U), _sentMessages(), props(), recordSentMessages(false) {
            setProperties( opts );
            construct();
        }

        /// Copy constructor
        BP( const BP &x ) : DAIAlgFG(x), _edges(x._edges), _edge2lut(x._edge2lut),
            _lut(x._lut), _maxdiff(x._maxdiff), _iters(x._iters), _sentMessages(x._sentMessages),
            props(x.props), recordSentMessages(x.recordSentMessages)
        {
            for( LutType::iterator l = _lut.begin(); l != _lut.end(); ++l )
                _edge2lut[l->second.first][l->second.second] = l;
        }

        /// Assignment operator
        BP& operator=( const BP &x ) {
            if( this != &x ) {
                DAIAlgFG::operator=( x );
                _edges = x._edges;
                _lut = x._lut;
                for( LutType::iterator l = _lut.begin(); l != _lut.end(); ++l )
                    _edge2lut[l->second.first][l->second.second] = l;
                _maxdiff = x._maxdiff;
                _iters = x._iters;
                _sentMessages = x._sentMessages;
                props = x.props;
                recordSentMessages = x.recordSentMessages;
            }
            return *this;
        }
    //@}

    /// \name General InfAlg interface
    //@{
        virtual BP* clone() const { return new BP(*this); }
        virtual std::string identify() const;
        virtual Factor belief( const Var &v ) const { return beliefV( findVar( v ) ); }
        virtual Factor belief( const VarSet &vs ) const;
        virtual Factor beliefV( size_t i ) const;
        virtual Factor beliefF( size_t I ) const;
        virtual std::vector<Factor> beliefs() const;
        virtual Real logZ() const;
        virtual void init();
        virtual void init( const VarSet &ns );
        virtual Real run();
        virtual Real maxDiff() const { return _maxdiff; }
        virtual size_t Iterations() const { return _iters; }
        virtual void setProperties( const PropertySet &opts );
        virtual PropertySet getProperties() const;
        virtual std::string printProperties() const;
    //@}

    /// \name Additional interface specific for BP
    //@{
        /// Calculates the joint state of all variables that has maximum probability
        /** \pre Assumes that run() has been called and that \a props.inference == \c MAXPROD
         */
        std::vector<std::size_t> findMaximum() const;

        /// Returns history of which messages have been updated
        const std::vector<std::pair<std::size_t, std::size_t> >& getSentMessages() const {
            return _sentMessages;
        }

        /// Clears history of which messages have been updated
        void clearSentMessages() { _sentMessages.clear(); }
    //@}

    protected:
        /// Returns constant reference to message from the \a _I 'th neighbor of variable \a i to variable \a i
        const Prob & message(size_t i, size_t _I) const { return _edges[i][_I].message; }
        /// Returns reference to message from the \a _I 'th neighbor of variable \a i to variable \a i
        Prob & message(size_t i, size_t _I) { return _edges[i][_I].message; }
        /// Returns constant reference to updated message from the \a _I 'th neighbor of variable \a i to variable \a i
        const Prob & newMessage(size_t i, size_t _I) const { return _edges[i][_I].newMessage; }
        /// Returns reference to updated message from the \a _I 'th neighbor of variable \a i to variable \a i
        Prob & newMessage(size_t i, size_t _I) { return _edges[i][_I].newMessage; }
        /// Returns constant reference to cached index for the edge between variable \a i and its \a _I 'th neighbor
        const ind_t & index(size_t i, size_t _I) const { return _edges[i][_I].index; }
        /// Returns reference to cached index for the edge between variable \a i and its \a _I 'th neighbor
        ind_t & index(size_t i, size_t _I) { return _edges[i][_I].index; }
        /// Returns constant reference to residual for the edge between variable \a i and its \a _I 'th neighbor
        const Real & residual(size_t i, size_t _I) const { return _edges[i][_I].residual; }
        /// Returns reference to residual for the edge between variable \a i and its \a _I 'th neighbor
        Real & residual(size_t i, size_t _I) { return _edges[i][_I].residual; }

        /// Calculate the product of factor \a I and the incoming messages
        /** If \a without_i == \c true, the message coming from variable \a i is omitted from the product
         *  \note This function is used by calcNewMessage() and calcBeliefF()
         */
        virtual Prob calcIncomingMessageProduct( size_t I, bool without_i, size_t i ) const;
        /// Calculate the updated message from the \a _I 'th neighbor of variable \a i to variable \a i
        virtual void calcNewMessage( size_t i, size_t _I );
        /// Replace the "old" message from the \a _I 'th neighbor of variable \a i to variable \a i by the "new" (updated) message
        void updateMessage( size_t i, size_t _I );
        /// Set the residual (difference between new and old message) for the edge between variable \a i and its \a _I 'th neighbor to \a r
        void updateResidual( size_t i, size_t _I, Real r );
        /// Finds the edge which has the maximum residual (difference between new and old message)
        void findMaxResidual( size_t &i, size_t &_I );
        /// Calculates unnormalized belief of variable \a i
        virtual void calcBeliefV( size_t i, Prob &p ) const;
        /// Calculates unnormalized belief of factor \a I
        virtual void calcBeliefF( size_t I, Prob &p ) const {
            p = calcIncomingMessageProduct( I, false, 0 );
        }

        /// Helper function for constructors
        virtual void construct();
};


} // end of namespace dai


#endif