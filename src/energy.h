#pragma once

#include "core.h"
#include "potentials.h"

namespace Faunus {
    namespace Energy {

        /**
         * @brief Nonbonded energy using a pair-potential
         */
        template<typename Tspace, typename Tpairpot>
        struct Nonbonded {
            Tpairpot pairpot;
            typedef std::vector<int> Tindex;

            template<typename T>
            double i2i(const Tspace &spc, const T &a, const T &b) {
                return pairpot(a, b, spc.geo.vdist(a.pos, b.pos));
            }

            template<typename T>
            double g2g(const Tspace &spc, const T &g1, const T &g2) {
                double u = 0;
                for (auto &i : g1)
                    for (auto &j : g2)
                        u += i2i(spc, i, j);
                return u;
            }

            double index2index(const Tspace &spc, const Tindex &index1, const Tindex &index2) {
                double u = 0;
                for (auto &i : index1)
                    for (auto &j : index2)
                        u += i2i(spc, spc.p[i], spc.p[j]);
                return u;
            }

            std::pair<double,double> energy(Tspace &oldspc, Tspace &newspc, typename Tspace::Tchange &change) {
                if (change.empty())
                    return {0,0};

                double uold=0, unew=0;

                using namespace ranges;
                auto moved = change.touchedGroupIndex(); // index of moved groups
                auto fixed = view::ints( 0, int(oldspc.groups.size()) )
                    | view::remove_if(
                            [&moved](int i){return std::binary_search(moved.begin(), moved.end(), i);}
                            ); // index of static groups

                for (auto &d : change.groups) {

                    // moved<->moved
                    /*
                       for (auto &n : change.groups)
                       if (n.first > m.first) {
                       auto &g1 = spc.groups[m.first];
                       auto &g2 = spc.groups[n.first];
                       u += g2g(spc, g1, g2);
                       }*/

                    // moved<->static
                    for (auto i : fixed) {
                        uold += g2g(oldspc, oldspc.groups[d.index], oldspc.groups[i]);
                        unew += g2g(newspc, newspc.groups[d.index], newspc.groups[i]);
                    }
                }
                return {uold, unew};
            }
        }; // Nonbonded energy before and after change (returns pair w. uold and unew)

    }//namespace
}//namespace
