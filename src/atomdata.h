#pragma once
#include "core.h"

namespace Faunus {

/**
 * @brief General properties for atoms
 */
class AtomData { // has to be a class when a constant reference is used
  public:
    typedef int Tid;
    typedef std::string TPropertyName;

  private:
    std::map<TPropertyName, double> property; //!< arbitrary additional properties
    Tid _id = -1;
    friend void to_json(json &, const AtomData &);
    friend void from_json(const json &, AtomData &);

  public:
    std::string name;         //!< Name
    double charge = 0;        //!< Particle charge [e]
    double mw = 1;            //!< Weight
    double sigma = 0;         //!< Diameter for e.g Lennard-Jones etc. [angstrom]
                              //!< Do not set! Only a temporal class member during the refactorization
    double activity = 0;      //!< Chemical activity [mol/l]
    double alphax = 0;        //!< Excess polarisability (unit-less)
    double dp = 0;            //!< Translational displacement parameter [angstrom]
    double dprot = 0;         //!< Rotational displacement parameter [degrees]
    double mulen = 0;         //!< Dipole moment scalar [eÃ]
    double sclen = 0;         //!< Sphere-cylinder length [angstrom]
    double tension = 0;       //!< Surface tension [kT/Å^2]
    double tfe = 0;           //!< Transfer free energy [J/mol/angstrom^2/M]
    Point mu = {0, 0, 0};     //!< Dipole moment unit vector
    Point scdir = {1, 0, 0};  //!< Sphero-cylinder direction
    bool hydrophobic = false; //!< Is the particle hydrophobic?
    bool implicit = false;    //!< Is the particle implicit (e.g. proton)?

    Tid &id();                //!< Type id
    const Tid &id() const;    //!< Type id
    double getProperty(const TPropertyName name) const;
    double &getProperty(const TPropertyName name);
    void setProperty(const TPropertyName name, const double value);
};

void to_json(json &j, const AtomData &a);
void from_json(const json &j, AtomData &a);

/**
 * @brief Construct vector of atoms from json array
 *
 * Items are added to existing items while if an item
 * already exists, it will be overwritten.
 */
void from_json(const json &j, std::vector<AtomData> &v);

extern std::vector<AtomData> atoms; //!< Global instance of atom list

template <class Trange> auto findName(Trange &rng, const std::string &name) {
    return std::find_if(rng.begin(), rng.end(), [&name](auto &i) { return i.name == name; });
} //!< Returns iterator to first element with member `name` matching input

template <class Trange> std::vector<int> names2ids(Trange &rng, const std::vector<std::string> &names) {
    std::vector<AtomData::Tid> index;
    index.reserve(names.size());
    for (auto &n : names) {
        // wildcard selecting all id's
        if (n == "*") {
            index.resize(rng.size());
            std::iota(index.begin(), index.end(), 0);
            return index;
        }
        auto it = findName(rng, n);
        if (it != rng.end())
            index.push_back(it->id());
        else
            throw std::runtime_error("name '" + n + "' not found");
    }
    return index;
} //!< Convert vector of names into vector of id's from Trange (exception if not found)

} // namespace Faunus
