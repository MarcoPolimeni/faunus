#ifndef FAU_HISTOGRAM_H
#define FAU_HISTOGRAM_H

#include "faunus/point.h"
#include "faunus/average.h"
#include "faunus/xytable.h"
#include "faunus/container.h"
#include "faunus/group.h"

namespace Faunus {
  /*!
   * \brief Histogram class
   * \author Mikael Lund
   */
  class histogram : private xytable<float,unsigned long int> {
    friend class FAUrdf;
    private:
    unsigned long int cnt;
    float xmaxi,xmini;  // ugly solution!
    public:
    histogram(float, float, float);
    string comment;                     //!< User defined comment
    void add(float);
    void write(string);
    virtual float get(float);

    //! Example of histogram class
    //! \example histogram-test.C
  };
  //! \param res x value resolution
  //! \param min minimum x value
  //! \param max maximum x value
  histogram::histogram(float res, float min, float max)
    : xytable<float,unsigned long int>(res,min,max) {
      cnt=0;
      xmaxi=max;
      xmini=min;
    }

  //! Increment bin for x value
  void histogram::add(float x) {
    if (x>=xmaxi || x<=xmini) return;
    (*this)(x)++;
    cnt++;
  }

  //! Get bin for x value
  //! \return \f$ \frac{N(r)}{N_{tot}}\f$
  float histogram::get(float x) { return (*this)(x)/float(cnt); }

  //! Show results for all x
  void histogram::write(string file) {
    float g;
    std::ofstream f(file.c_str());
    if (f) {
      f.precision(6);
      for (double x=xmin; x<xmax(); x+=xres) {
        g=get(x);
        if (g!=0.0)
          f << x << " " << g << "\n";
      }
      f.close();
    }
  }

  //-----------------------------------------------------------------
  /*!
   *  \brief Class to calculate the radial distribution between particles.
   *
   *  The internal histogram vector will be automatically expanded but an initial
   *  maximum x valued can be specified so as to utilize memory more efficiently.
   *  \author Mikael Lund
   *  \date Prague, April 2007
   *  \todo Needs testing!
   *
   */
  class FAUrdf : public histogram {
    private:
      short a,b;                   //!< Particle types to investigate
      float volume(float);         //!< Volume of shell r->r+xres
      unsigned int npart;
    public:
      FAUrdf(float=.5, float=0);
      FAUrdf(short, short, float=.5, float=0); 
      void update(container &, point &, string);//!< Search around a point
      void update(container &);                 //!< Update histogram vector
      void update(container &, group &);        //!< Update histogram vector - search only in group
      void update(vector<particle> &);         //!< Update histogram vector
      void update(container &, point &, point &);//!< Update for two points
      float get(float);                        //!< Get g(x)
  };

  /*!
   * \param species1 Particle type 1
   * \param species2 Particle type 2
   * \param resolution Histogram resolution (binwidth)
   * \param xmaximum Maximum x value (used for better memory utilisation)
   */
  FAUrdf::FAUrdf(short species1, short species2, float resolution, float xmaximum) :
    histogram(resolution, 0, xmaximum)
  {
    a=species1;
    b=species2;
  }
  FAUrdf::FAUrdf(float resolution, float xmaximum) : histogram(resolution, 0, xmaximum) {};

  /*!
   * Update histogram between two known points
   * \note Uses the container distance function
   */
  void FAUrdf::update(container &c, point &a, point &b) {
    add( c.dist(a, b) );
  }

  /*!
   * Calculate all distances between between species 1
   * and 2 and update the histogram.
   *
   * \note Uses the container function to calculate distances
   */
  void FAUrdf::update(container &c) {
    int n=c.p.size();
    npart=0;
#pragma omp for
    for (int i=0; i<n-1; i++)
      for (int j=i+1; j<n; j++) 
        if ( (c.p[i].id==a && c.p[j].id==b)
            || (c.p[j].id==a && c.p[i].id==b) ) {
          npart++;
          update( c, c.p[i], c.p[j] );
        }
  }

  void FAUrdf::update(container &c, group &g) {
    int n=g.end+1;
    npart=0;
//#pragma omp for
    for (int i=g.beg; i<n-1; i++) {
      for (int j=i+1; j<n; j++) { 
        if ( (c.p[i].id==a && c.p[j].id==b)
            || (c.p[j].id==a && c.p[i].id==b) ) {
          npart++;
          add( c.dist(c.p[i], c.p[j]) );
        }
      }
    }
  }

  void FAUrdf::update(container &c, point &p, string name) {
    int id=c.atom[name].id, n=c.p.size();
    npart=0;
//#pragma omp for
    for (int i=0; i<n; ++i)
      if ( c.p[i].id==id ) {
        npart++;
        add( c.dist(p, c.p[i] ));
      }
  }

  /*!
   * Calculate all distances between between species 1
   * and 2 and update the histogram.
   *
   * \warning This function uses a simple distance function (no min. image)
   */
  void FAUrdf::update(vector<particle> &p)
  {
    int n=p.size();
    npart=0;
//#pragma omp for
    for (int i=0; i<n-1; i++)
      for (int j=i+1; j<n; j++) 
        if ( (p[i].id==a && p[j].id==b)
            || (p[j].id==a && p[i].id==b) ) {
          npart++;
          add( p[i].dist(p[j]) );
        }
  }

  /*!
   * Calculate shell volume at x
   */
  float FAUrdf::volume(float x) { return 4./3.*acos(-1.)*( pow(x+0.5*xres,3)-pow(x-0.5*xres,3) ); }
  /*!
   *  Get g(x) from histogram according to
   *    \f$ g(x) = \frac{N(r)}{N_{tot}} \frac{ 3 } { 4\pi\left [ (x+xres)^3 - x^3 \right ] }\f$
   */
  float FAUrdf::get(float x) {
    return (*this)(x)/(cnt*volume(x)) * 1660.57;
  }

  /*!
   *  Get g(x) from histogram according to
   *    \f$ g(x) = \frac{N(r)}{N_{tot}} \frac{ 3 } { 4\pi\left [ (x+xres)^3 - x^3 \right ] }\f$
   */

};//namespace
#endif
