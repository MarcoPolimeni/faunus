#include <faunus/faunus.h>
#include <faunus/multipole.h>
using namespace Faunus;                          // use Faunus namespace
using namespace Faunus::Move;
using namespace Faunus::Potential;

typedef Space<Geometry::Cuboid,DipoleParticle> Tspace; 
typedef DipoleDipoleWolfDamped Tpair1;
typedef CombinedPairPotential<LennardJones,Tpair1> Tpair;

#ifdef POLARIZE
typedef Move::PolarizeMove<AtomicTranslation<Tspace> > TmoveTran;
typedef Move::PolarizeMove<AtomicRotation<Tspace> > TmoveRot;
#else
typedef Move::AtomicTranslation<Tspace> TmoveTran;   
typedef Move::AtomicRotation<Tspace> TmoveRot;
#endif

template<class Tpairpot, class Tid>
bool savePotential(Tpairpot pot, Tid ida, Tid idb, string file) {
  std::ofstream f(file.c_str());
  if (f) {
    //double min=1.1 * (atom[ida].radius+atom[idb].radius);
    DipoleParticle a,b;
    a=atom[ida];
    b=atom[idb];
    a.mu = Point(1,0,0);
    b.mu = Point(1,0,0);
    /*f << "# Pair potential: " << pot.brief() << endl
      << "# Atoms: " << atom[ida].name << "<->" << atom[idb].name
      << endl;*/
    for (double r=0.6; r<=4.5; r+=0.01) {
      f << std::left << std::setw(10) << r << " "
        << pot(a,b,Point(r,0,0)) << endl;
    }
    return true;
  }
  return false;
}

template<class Tpairpot, class Tid>
bool savePotential1(Tpairpot pot, Tid ida, Tid idb, string file) {
  std::ofstream f(file.c_str());
  if (f) {
    //double min=1.1 * (atom[ida].radius+atom[idb].radius);
    DipoleParticle a,b;
    a=atom[ida];
    b=atom[idb];
    a.mu = Point(1,0,0);
    b.mu = Point(0,1,0);
    /*f << "# Pair potential: " << pot.brief() << endl
      << "# Atoms: " << atom[ida].name << "<->" << atom[idb].name
      << endl;*/
    for (double r=0.6; r<=4.5; r+=0.01) {
      f << std::left << std::setw(10) << r << " "
        << pot(a,b,Point(r,0,0)) << endl;
    }
    return true;
  }
  return false;
}

int main() {
  //::atom.includefile("stockmayer.json");         // load atom properties
  InputMap in("stockmayer.input");               // open parameter file for user input
  Energy::NonbondedVector<Tspace,Tpair> pot(in); // non-bonded only
  EnergyDrift sys;                               // class for tracking system energy drifts
  Tspace spc(in);                // create simulation space, particles etc.
  Group sol;
  sol.addParticles(spc, in);                     // group for particles
  MCLoop loop(in);                               // class for mc loop counting
  Analysis::RadialDistribution<> rdf(0.05);       // particle-particle g(r)
  Analysis::Table2D<double,Average<double> > mucorr(0.1);       // particle-particle g(r)
  Analysis::Table2D<double,double> mucorr_distribution(0.1);
  TmoveTran trans(in,pot,spc);
  TmoveRot rot(in,pot,spc);
  trans.setGroup(sol);                                // tells move class to act on sol group
  rot.setGroup(sol);                                  // tells move class to act on sol group
  spc.load("state");
  spc.p = spc.trial;
  UnitTest test(in);               // class for unit testing
  DipoleWRL sdp;
  FormatXTC xtc(spc.geo.len.norm());
  savePotential(Tpair1(in), atom["sol"].id, atom["sol"].id, "pot_dipdip.dat");
  savePotential1(Tpair1(in), atom["sol"].id, atom["sol"].id, "pot_dipdip1.dat");
  return 1;
  sys.init( Energy::systemEnergy(spc,pot,spc.p)  );   // initial energy
  while ( loop.macroCnt() ) {                         // Markov chain 
    while ( loop.microCnt() ) {
      if (slp_global() > 0.5)
        sys+=trans.move( sol.size() );                // translate
      else
        sys+=rot.move( sol.size() );                  // rotate

      if (slp_global()<1.5)
        for (auto i=sol.front(); i<sol.back(); i++) { // salt rdf
          for (auto j=i+1; j<=sol.back(); j++) {
            double r =spc.geo.dist(spc.p[i],spc.p[j]); 
            rdf(r)++;
            mucorr(r) += spc.p[i].mu.dot(spc.p[j].mu);
            mucorr_distribution(spc.p[i].mu.dot(spc.p[j].mu)) += 1;
          }
        }
      if (slp_global()>0.99)
        xtc.save(textio::prefix+"out.xtc", spc.p);  
    }    
    sys.checkDrift(Energy::systemEnergy(spc,pot,spc.p)); // compare energy sum with current
    cout << loop.timing();
  }

  // perform unit tests
  trans.test(test);
  rot.test(test);
  sys.test(test);
  sdp.saveDipoleWRL("stockmayer.wrl",spc,sol);
  FormatPQR().save("confout.pqr", spc.p);
  rdf.save("gofr.dat");                               // save g(r) to disk
  mucorr.save("mucorr.dat");                               // save g(r) to disk
  mucorr_distribution.save("mucorr_distribution.dat");
  std::cout << spc.info() + pot.info() + trans.info()
    + rot.info() + sys.info() + test.info(); // final info
  spc.save("state");

  return test.numFailed();
}
/**  @page example_stockmayer Example: Stockmayer potential
 *
 This will simulate a Stockmayer potential in a cubic box.

 Run this example from the `examples` directory:

 ~~~~~~~~~~~~~~~~~~~
 $ make
 $ cd src/examples
 $ ./stockmayer.run
 ~~~~~~~~~~~~~~~~~~~

 stockmayer.cpp
 ============

 @includelineno examples/stockmayer.cpp

*/
