#ifndef FAUNUS_MULTIPOLE
#define FAUNUS_MULTIPOLE

namespace Faunus {

  template<typename Tenergy>
    void getInducedDipoles(Tenergy &pot, Space &spc, const Eigen::MatrixXd &mu_per, double &limit) { 
      int size = spc.p.size();
      double diLim = size*limit;
      double diConv;
      Eigen::MatrixXd Field(size,3);
      Field.setZero();
      Point E(0,0,0);
      Point mu_ind(0,0,0);
      Point mu_err(0,0,0);

      do {  
        diConv = 0.0;
        Field = getField(pot,spc);
        for(int i = 0; i < size; i++) {
          E = Field.row(i);
          mu_ind = atom[spc.p[i].id].alphamatrix*E;
          mu_err = mu_ind - spc.p[i].mu*spc.p[i].muscalar + mu_per.row(i).transpose();
          diConv = diConv + mu_err.norm();
          mu_ind = mu_ind + mu_per.row(i).transpose();

          spc.p[i].muscalar = mu_ind.norm();
          spc.p[i].mu = mu_ind/spc.p[i].muscalar;
        }
      } while (diConv > diLim);
    }

  /**
   * @brief Returns dipole-dipole interaction which times the 
   *
   * @param mu1 Unit dipole moment vector 1
   * @param mu2 Unit dipole moment vector 2
   * @param mu1xmu2 Product of dipole scalars
   * @param r Vector R_{1->2}
   *
   * T_{ab} = 3*r_a*r_b / |r|^5 , for a ne b
   * T_{aa} = 3*r_a^2 / |r|^5 - 1 / |r|^3
   * Summation of all T_{ab}*mu_a*mu_b
   *
   */
  template<class Tvec>
    double mu2mu(const Tvec &mu1, const Tvec &mu2, double mu1xmu2, const Tvec &r) {
      double R2 = 1/r.squaredNorm();
      double R1 = sqrt(R2);
      double R3 = R1*R2;
      double R5 = R3*R2;
      Eigen::Matrix3d T = 3*R5*r*r.transpose() - R3*Matrix3d::Identity();
      double W = -mu1.transpose()*T*mu2;
      return W*mu1xmu2;
    }

  namespace Potential {

    /**
     * @brief Dipole-dipole interaction
     *
     * More info...
     */
    class DipoleDipole : public PairPotentialBase {
      private:
        string _brief() { return "Dipole-dipole"; }
      protected:
        double _lB;
      public:
        DipoleDipole(InputMap &in) {
          pc::setT ( in.get<double>("temperature", 298.15, "Absolute temperature (K)") );
          double epsilon_r = in.get<double>("epsilon_r",80., "Dielectric constant");
          _lB=pc::lB( epsilon_r );
        }
        template<class Tparticle>
          double operator()(const Tparticle &a, const Tparticle &b, const Point &r) const {
            return _lB*mu2mu(a.mu,b.mu,a.muscalar*b.muscalar,r);
          }

        /** @brief Dipole field at `r` due to dipole `p` */
        template<class Tparticle>
          Point field(const Tparticle &p, const Point &r) const {
            double R2 = 1/r.squaredNorm();
            double R1 = sqrt(R2);
            double R3 = R1*R3;
            Point rn = r*R1;
            return (3.0*p.mu.dot(rn)*rn - p.mu)*p.muscalar*R3;
          }

        string info(char w) { return _brief(); }
    };

    /**
     * @brief Dipole-dipole interaction w. spherical cutoff and reaction field
     *
     * More info...
     */
    class DipoleDipoleRF : public DipoleDipole {
      private:
        string _brief() { return "Dipole-dipole (RF)"; }
        double rc2,eps;
      public:
        DipoleDipoleRF(InputMap &in) : DipoleDipole(in) { 
          rc2 = pow(in.get<double>("dipdip_cutoff",pc::infty), 2);
          eps = in.get<double>("epsilon_rf",80.);
          eps = _lB*(2*(eps-1)/(eps+1))/pow(rc2,1.5);
        }
        template<class Tparticle>
          double operator()(const Tparticle &a, const Tparticle &b, const Point &r) const {
            if (r.squaredNorm() < rc2)
              return DipoleDipole::operator()(a,b,r) - eps*a.mu.dot(b.mu)*a.muscalar*b.muscalar;
            return 0.0;
          }

        string info(char w) { return _brief(); }
    };
  }
}
#endif
