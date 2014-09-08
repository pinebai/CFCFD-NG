/*
 * viscosity.d
 * Interface for all viscosity models.
 *
 * Author: Rowan G. and Peter J.
 * Version: 2014-08-19 -- initial cut
 */

import gasmodel;

interface Viscosity {
    Viscosity dup() const;
    double eval(in GasState Q);
    final void update_viscosity(ref GasState Q) {
	Q.mu = eval(Q);
    }
}
