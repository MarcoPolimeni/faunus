/*! \file Example how to simulate a NaCl solution
 *  \example widom-example.C
 *
 * This example program calculates the excess chemical
 * potential of NaCl in an aqueous solution using Widom's
 * particle insertion method.
 * Note: No equilibration run is incorporated.
 */
#include "faunus/faunus.h"
#include "faunus/potentials/pot_coulomb.h"

using namespace std;
using namespace Faunus;                 // Access to Faunus classes

int main() {
  inputfile in("pythontest.conf");      // Read input file (not complete at this moment)
  box cell(in);                         // We want a cubic cell
  canonical nvt;                        // Use the canonical ensemble
  interaction<pot_coulomb> pot(in);     // Energy functions
  saltmove sm(nvt, cell, pot);          // Class for salt movements
  widom widom(10);                      // Class for Widom particle insertions
  widom.add(cell);                      // Locate species in the container
  group salt;
  salt.add( cell, particle::NA, 60 );   // Insert some sodium ions
  salt.add( cell, particle::CL, 60 );   // Insert some chloride ions
  systemenergy sys(pot.energy(cell.p)); // Track system energy
  for (int macro=0; macro<10; macro++) {        // Markov chain
    for (int micro=0; micro<1e2; micro++) {
      sys+=sm.move(salt);                       // Displace salt particles
      widom.insert(cell,pot);                   // Widom particle insertion analysis
    }                                           // END of micro loop
    sys.update(pot.energy(cell.p));             // Update system energy
  }                                             // END of macro loop and simulation
  cout << cell.info() << sys.info()
    << sm.info() << widom.info();               // Print information!
}

