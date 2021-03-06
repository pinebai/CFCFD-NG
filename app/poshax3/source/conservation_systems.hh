/** \file conservation_systems.hh
 *  \brief Conservation systems for post-shock relaxation
 *
 *  \author Daniel F Potter
 *  \date 30-Jun-2010
 *
 **/

#ifndef CONS_SYS_HH
#define CONS_SYS_HH

#include <vector>

#include "../../../lib/nm/source/no_fuss_linear_algebra.hh"
#include "../../../lib/nm/source/zero_system.hh"
#include "../../../lib/gas/models/gas_data.hh"
#include "../../../lib/gas/models/gas-model.hh"

class FrozenConservationSystem : public ZeroSystem {
public:
    /// \brief Default constructor
    FrozenConservationSystem();
    /// \brief Normal Constructor
    FrozenConservationSystem( Gas_model * gm, Gas_data * Q, double u );

    /// \brief Copy Constructor
    FrozenConservationSystem( const FrozenConservationSystem &c );

    /// \brief Default destructor
    virtual ~FrozenConservationSystem();

    void initialise(Gas_model * gm, Gas_data * Q, double u);

    int f( const std::vector<double> &y, std::vector<double> &G );
    int Jac( const std::vector<double> &y, Valmatrix &dGdy );

    /// \brief Read access to A_, the total mass conserved variable
    double get_A() { return A_; }

    /// \brief Read access to C_, the total energy conserved variable
    double get_C() { return C_; }

    /// \brief Write access to C_, the total energy conserved variable
    void set_C( double C ) { C_ = C; }

private:
    Gas_model * gmodel_;
    Gas_data * Q_;
    double A_;
    double B_;
    double C_;
};

class NoneqConservationSystem : public ZeroSystem {
public:
    /// \brief Default constructor
    NoneqConservationSystem();
    /// \brief Normal Constructor
    NoneqConservationSystem( Gas_model * gm, Gas_data * Q, double u );

    /// \brief Copy Constructor
    NoneqConservationSystem( const NoneqConservationSystem &c );

    /// \brief Default destructor
    virtual ~NoneqConservationSystem();

    void initialise(Gas_model * gm, Gas_data * Q, double u);
    
    int f( const std::vector<double> &y, std::vector<double> &G );
    int Jac( const std::vector<double> &y, Valmatrix &dGdy );
    int encode_conserved( std::vector<double> &y, const Gas_data &Q,
    	                  const double u );
    int set_constants( const std::vector<double> &A );

private:
    Gas_model * gmodel_;
    Gas_data * Q_;
    
    int nsp_;
    int ntm_;
    int ndim_;
    int e_index_;

    std::vector<double> A_;
};

#endif
