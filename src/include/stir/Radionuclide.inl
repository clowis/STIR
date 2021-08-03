//
//
/*!

  \file
  \brief Implementations of inline functions of class stir::Radionuclide

  \author Daniel Deidda
  \author Kris Thielemans
*/
/*
    Copyright (C) 2021 National Physical Laboratory
    Copyright (C) 2021 University College London
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0

    See STIR/LICENSE.txt for details
*/
START_NAMESPACE_STIR

std::string
Radionuclide::parameter_info() const
{
#ifdef BOOST_NO_STRINGSTREAM
  // dangerous for out-of-range, but 'old-style' ostrstream seems to need this
  char str[30000];
  ostrstream s(str, 30000);
#else
  std::ostringstream s;
#endif
  s << "Modality: " << this->modality.get_name() << '\n';
  s << "Radionuclide: " << this->name << '\n';
  s << "Energy " << std::fixed << std::setprecision(12) << this->energy << std::setprecision(5) << '\n';
  s << "Half-life: " << std::fixed << std::setprecision(12) << this->half_life << std::setprecision(5) << '\n';
  s << "Branching ratio: " << std::fixed << std::setprecision(12) <<this->branching_ratio << std::setprecision(5)<<'\n';
  return s.str();
}

Radionuclide::Radionuclide()
    :name("Unknown"),energy(-1),
     branching_ratio(-1),
     half_life(-1),modality("Unknown")
{
    info(this->parameter_info());
}


Radionuclide::Radionuclide(const std::string &rname, float renergy, float rbranching_ratio, float rhalf_life,
                           ImagingModality rmodality)
	 :name(rname),energy(renergy),
      branching_ratio(rbranching_ratio),
      half_life(rhalf_life),modality(rmodality)
     {}

     
std::string
 Radionuclide:: get_name()const
{ 
    if (name=="Unknown")
        error("Radionuclide is Unknown, If you want to use it, it needs to be defined!");
    return name;
}

float
 Radionuclide::get_energy()const 
{
    if (energy<=0)
        error("Radionuclide energy peak is unset, If you want to use it, it needs to be set!");
    return energy;
}

float
 Radionuclide::get_branching_ratio()  const
{ 
    if (energy<=0)
        error("Radionuclide Branching ratio is unset, If you want to use it, it needs to be set!");
    return branching_ratio;
}

float
 Radionuclide::get_half_life() const
{
    if (half_life<=0)
        error("Radionuclide half life is unset, If you want to use it, it needs to be set!");
    return half_life;
}

ImagingModality
 Radionuclide::get_modality() const
{ 
    if (modality.get_name()=="Unknown")
        error("Radionuclide::modality is Unknown, If you want to use it, it needs to be defined!");
    return modality;
}

bool  
Radionuclide::operator==(const Radionuclide& r) const{ 
        return (abs(energy - r.energy)<= 1E-1 || r.energy<0) &&
           (abs(branching_ratio-r.branching_ratio)<= 1E-1 || r.branching_ratio<0) &&
           (abs(half_life-r.half_life)<=1 || r.half_life<0) &&
           (modality.get_name()==r.modality.get_name() || r.modality.get_name()=="Unknown");
}

END_NAMESPACE_STIR
