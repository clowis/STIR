/*
    Copyright (C) 2004 - 2009 Hammersmith Imanet Ltd
    Copyright (C) 2013 - 2016, 2019 University College London
    Copyright (C) 2018-2019, University of Hull
    This file is part of STIR.

    This file is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This file is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    See STIR/LICENSE.txt for details
*/
/*!
  \file
  \ingroup scatter
  \brief Definition of class stir::ScatterSimulation.

  \author Nikos Efthimiou
  \author Kris Thielemans
*/
#include "stir/scatter/ScatterSimulation.h"
#include "stir/ViewSegmentNumbers.h"
#include "stir/Bin.h"

#include "stir/Viewgram.h"
#include "stir/is_null_ptr.h"
#include "stir/IO/read_from_file.h"
#include "stir/IO/write_to_file.h"
#include "stir/info.h"
#include "stir/error.h"
#include <fstream>
#include <boost/format.hpp>

#include "stir/zoom.h"
#include "stir/SSRB.h"

#include "stir/stir_math.h"
#include "stir/zoom.h"
#include "stir/ZoomOptions.h"
#include "stir/NumericInfo.h"

START_NAMESPACE_STIR

ScatterSimulation::
ScatterSimulation()
{
    this->set_defaults();
}

ScatterSimulation::
~ScatterSimulation()
{
    // Sometimes I get a segfault without this line.
    scatt_points_vector.clear();
}

Succeeded
ScatterSimulation::
process_data()
{
    // this is useful in the scatter estimation process.
    this->output_proj_data_sptr->fill(0.f);
    info("ScatterSimulator: Running Scatter Simulation ...");
    info("ScatterSimulator: Initialising ...");
    // The activity image might have been changed, during the estimation process.
//    this->remove_cache_for_integrals_over_activity();
//    this->remove_cache_for_integrals_over_attenuation();
    this->sample_scatter_points();
    this->initialise_cache_for_scattpoint_det_integrals_over_attenuation();
    this->initialise_cache_for_scattpoint_det_integrals_over_activity();

    ViewSegmentNumbers vs_num;

    int bin_counter = 0;
    int axial_bins = 0 ;

    for (vs_num.segment_num() = this->proj_data_info_cyl_noarc_cor_sptr->get_min_segment_num();
         vs_num.segment_num() <= this->proj_data_info_cyl_noarc_cor_sptr->get_max_segment_num();
         ++vs_num.segment_num())
        axial_bins += this->proj_data_info_cyl_noarc_cor_sptr->get_num_axial_poss(vs_num.segment_num());

    const int total_bins =
            this->proj_data_info_cyl_noarc_cor_sptr->get_num_views() * axial_bins *
            this->proj_data_info_cyl_noarc_cor_sptr->get_num_tangential_poss();
    /* Currently, proj_data_info.find_cartesian_coordinates_of_detection() returns
     coordinate in a coordinate system where z=0 in the first ring of the scanner.
     We want to shift this to a coordinate system where z=0 in the middle
     of the scanner.
     We can use get_m() as that uses the 'middle of the scanner' system.
     (sorry)
  */
#ifndef NDEBUG
    {
        CartesianCoordinate3D<float> detector_coord_A, detector_coord_B;
        // check above statement
        this->proj_data_info_cyl_noarc_cor_sptr->find_cartesian_coordinates_of_detection(
                    detector_coord_A, detector_coord_B, Bin(0, 0, 0, 0));
        assert(detector_coord_A.z() == 0);
        assert(detector_coord_B.z() == 0);
        // check that get_m refers to the middle of the scanner
        const float m_first =
                this->proj_data_info_cyl_noarc_cor_sptr->get_m(Bin(0, 0, this->proj_data_info_cyl_noarc_cor_sptr->get_min_axial_pos_num(0), 0));
        const float m_last =
                this->proj_data_info_cyl_noarc_cor_sptr->get_m(Bin(0, 0, this->proj_data_info_cyl_noarc_cor_sptr->get_max_axial_pos_num(0), 0));
        assert(fabs(m_last + m_first) < m_last * 10E-4);
    }
#endif
    this->shift_detector_coordinates_to_origin =
            CartesianCoordinate3D<float>(this->proj_data_info_cyl_noarc_cor_sptr->get_m(Bin(0, 0, 0, 0)), 0, 0);
    float total_scatter = 0 ;

    info("ScatterSimulator: Initialization finished ...");

    for (vs_num.segment_num() = this->proj_data_info_cyl_noarc_cor_sptr->get_min_segment_num();
         vs_num.segment_num() <= this->proj_data_info_cyl_noarc_cor_sptr->get_max_segment_num();
         ++vs_num.segment_num())
    {
        for (vs_num.view_num() = this->proj_data_info_cyl_noarc_cor_sptr->get_min_view_num();
             vs_num.view_num() <= this->proj_data_info_cyl_noarc_cor_sptr->get_max_view_num();
             ++vs_num.view_num())
        {
            info(boost::format("ScatterSimulator: %d / %d") % bin_counter% total_bins);
            total_scatter += this->process_data_for_view_segment_num(vs_num);
            bin_counter +=
                    this->proj_data_info_cyl_noarc_cor_sptr->get_num_axial_poss(vs_num.segment_num()) *
                    this->proj_data_info_cyl_noarc_cor_sptr->get_num_tangential_poss();
            info(boost::format("ScatterSimulator: %d / %d") % bin_counter% total_bins);

        }
    }

    if (detection_points_vector.size() != static_cast<unsigned int>(total_detectors))
    {
        warning("Expected num detectors: %d, but found %d\n",
                total_detectors, detection_points_vector.size());
        return Succeeded::no;
    }

    info(boost::format("TOTAL SCATTER:= %g") % total_scatter);
    return Succeeded::yes;
}

//xxx double
double
ScatterSimulation::
process_data_for_view_segment_num(const ViewSegmentNumbers& vs_num)
{
    // First construct a vector of all bins that we'll process.
    // The reason for making this list before the actual calculation is that we can then parallelise over all bins
    // without having to think about double loops.
    std::vector<Bin> all_bins;
    {
        Bin bin(vs_num.segment_num(), vs_num.view_num(), 0, 0);

        for (bin.axial_pos_num() = this->proj_data_info_cyl_noarc_cor_sptr->get_min_axial_pos_num(bin.segment_num());
             bin.axial_pos_num() <= this->proj_data_info_cyl_noarc_cor_sptr->get_max_axial_pos_num(bin.segment_num());
             ++bin.axial_pos_num())
        {
            for (bin.tangential_pos_num() = this->proj_data_info_cyl_noarc_cor_sptr->get_min_tangential_pos_num();
                 bin.tangential_pos_num() <= this->proj_data_info_cyl_noarc_cor_sptr->get_max_tangential_pos_num();
                 ++bin.tangential_pos_num())
            {
                all_bins.push_back(bin);
            }
        }
    }
    // now compute scatter for all bins
    double total_scatter = 0;
    Viewgram<float> viewgram =
            this->output_proj_data_sptr->get_empty_viewgram(vs_num.view_num(), vs_num.segment_num());
#ifdef STIR_OPENMP
#pragma omp parallel for reduction(+:total_scatter) schedule(dynamic)
#endif

    for (int i = 0; i < static_cast<int>(all_bins.size()); ++i)
    {
        const Bin bin = all_bins[i];
        unsigned det_num_A = 0; // initialise to avoid compiler warnings
        unsigned det_num_B = 0;
        this->find_detectors(det_num_A, det_num_B, bin);
        // TODO: Not thread-safe. See issue  #417 on Github.
        const double scatter_ratio =
                scatter_estimate(det_num_A, det_num_B);
        viewgram[bin.axial_pos_num()][bin.tangential_pos_num()] =
                static_cast<float>(scatter_ratio);
        total_scatter += scatter_ratio;
    } // end loop over bins

    if (this->output_proj_data_sptr->set_viewgram(viewgram) == Succeeded::no)
        error("ScatterEstimationByBin: error writing viewgram");

    return static_cast<double>(viewgram.sum());
}

void
ScatterSimulation::set_defaults()
{
    this->attenuation_threshold =  0.01f ;
    this->random = true;
    this->use_cache = true;
    this->zoom_xy = -1.f;
    this->zoom_z = -1.f;
    this->zoom_size_xy = -1;
    this->zoom_size_z = -1;
    this->use_default_downsampling = false;
    this->downsample_scanner_dets = 1;
    this->downsample_scanner_rings = 1;
    this->density_image_filename = "";
    this->activity_image_filename = "";
    this->density_image_for_scatter_points_output_filename ="";
    this->density_image_for_scatter_points_filename = "";
    this->template_proj_data_filename = "";
    this->remove_cache_for_integrals_over_activity();
    this->remove_cache_for_integrals_over_attenuation();
}

void
ScatterSimulation::
ask_parameters()
{
    this->attenuation_threshold = ask_num("attenuation threshold(cm^-1)",0.0f, 5.0f, 0.01f);
    this->random = ask_num("random points?",0, 1, 1);
    this->use_cache =  ask_num(" Use cache?",0, 1, 1);
    this->density_image_filename = ask_string("density image filename", "");
    this->activity_image_filename = ask_string("activity image filename", "");
    this->density_image_for_scatter_points_filename = ask_string("density image for scatter points filename", "");
    this->template_proj_data_filename = ask_string("Scanner ProjData filename", "");
}

void
ScatterSimulation::initialise_keymap()
{

    this->parser.add_start_key("Scatter Simulation Parameters");
    this->parser.add_stop_key("end Scatter Simulation Parameters");
    this->parser.add_key("template projdata filename",
                         &this->template_proj_data_filename);
    this->parser.add_key("attenuation image filename",
                         &this->density_image_filename);
    this->parser.add_key("attenuation image for scatter points filename",
                         &this->density_image_for_scatter_points_filename);
    this->parser.add_key("zoom XY for attenuation image for scatter points",
                         &this->zoom_xy);
    this->parser.add_key("zoom Z for attenuation image for scatter points",
                         &this->zoom_z);
    this->parser.add_key("XY size of downsampled image for scatter points",
                         &this->zoom_size_xy);
    this->parser.add_key("Z size of downsampled image for scatter points",
                         &this->zoom_size_z);
    this->parser.add_key("attenuation image for scatter points output filename",
                         &this->density_image_for_scatter_points_output_filename);
    this->parser.add_key("downsampled scanner number of detectors per ring",
                         &this->downsample_scanner_dets);
    this->parser.add_key("downsampled scanner number of rings",
                         &this->downsample_scanner_rings);
    this->parser.add_key("activity image filename",
                         &this->activity_image_filename);
    this->parser.add_key("attenuation threshold",
                         &this->attenuation_threshold);
    this->parser.add_key("output filename prefix",
                         &this->output_proj_data_filename);
    this->parser.add_key("use default downsampling",
                         &this->use_default_downsampling);
    this->parser.add_key("random", &this->random);
    this->parser.add_key("use cache", &this->use_cache);
}


bool
ScatterSimulation::
post_processing()
{

    if (this->template_proj_data_filename.size() > 0)
        this->set_template_proj_data_info(this->template_proj_data_filename);

    if (this->activity_image_filename.size() > 0)
        this->set_activity_image(this->activity_image_filename);

    if (this->density_image_filename.size() > 0)
        this->set_density_image(this->density_image_filename);

    if(this->density_image_for_scatter_points_filename.size() > 0)
        this->set_density_image_for_scatter_points(this->density_image_for_scatter_points_filename);

    if ((zoom_xy!=1 || zoom_z != 1) &&
            !is_null_ptr(density_image_for_scatter_points_sptr))
    {
        downsample_density_image_for_scatter_points(zoom_xy, zoom_z);

        if(this->density_image_for_scatter_points_output_filename.size()>0)
            OutputFileFormat<DiscretisedDensity<3,float> >::default_sptr()->
                    write_to_file(density_image_for_scatter_points_output_filename,
                                  *this->density_image_for_scatter_points_sptr);
    }

    if (this->output_proj_data_filename.size() > 0)
        this->set_output_proj_data(this->output_proj_data_filename);

    if(this->use_default_downsampling)
        default_downsampling();

    return false;
}

Succeeded
ScatterSimulation::
set_up()
{
    if (is_null_ptr(proj_data_info_cyl_noarc_cor_sptr))
        error("ScatterSimulation: projection data info not set. Aborting.");

    if (!proj_data_info_cyl_noarc_cor_sptr->has_energy_information())
        error("ScatterSimulation: scanner energy resolution information not set. Aborting.");

    if(!template_exam_info_sptr->has_energy_information())
        error("ScatterSimulation: template energy window information not set. Aborting.");

    if(is_null_ptr(activity_image_sptr))
        error("ScatterSimulation: activity image not set. Aborting.");

    if(is_null_ptr(density_image_sptr))
        error("ScatterSimulation: density image not set. Aborting.");

    if(is_null_ptr(density_image_for_scatter_points_sptr))
    {
        if(!is_null_ptr(density_image_sptr))
        {
           set_density_image_for_scatter_points_sptr(density_image_sptr);
        }
        else
        {
            error("ScatterSimulation: Cannot find appropriate image for scatter points. Aborting.");
        }
    }

//    if(is_null_ptr(output_proj_data_sptr))
//    {
//        this->output_proj_data_sptr.reset(new ProjDataInMemory(this->template_exam_info_sptr,
//                                                               this->proj_data_info_cyl_noarc_cor_sptr->create_shared_clone()));
//        this->output_proj_data_sptr->fill(0.0);
//        info("ScatterSimulation: output projection data created.");
//    }

    return Succeeded::yes;
}

void
ScatterSimulation::
set_activity_image_sptr(const shared_ptr<DiscretisedDensity<3,float> >& arg)
{
    if (is_null_ptr(arg) )
        error("ScatterSimulation: Unable to set the activity image");

    this->activity_image_sptr = arg;
    this->remove_cache_for_integrals_over_activity();
}

void
ScatterSimulation::
set_activity_image(const std::string& filename)
{
    this->activity_image_filename = filename;
    this->activity_image_sptr=
            read_from_file<DiscretisedDensity<3,float> >(filename);

    if (is_null_ptr(this->activity_image_sptr))
    {
        error(boost::format("ScatterSimulation: Error reading activity image %s") %
              this->activity_image_filename);
    }
    this->remove_cache_for_integrals_over_activity();
}

void
ScatterSimulation::
set_density_image_sptr(const shared_ptr<DiscretisedDensity<3,float> >& arg)
{
    if (is_null_ptr(arg) )
        error("ScatterSimulation: Unable to set the density image");
    this->density_image_sptr=arg;
    this->remove_cache_for_integrals_over_attenuation();
}

void
ScatterSimulation::
set_density_image(const std::string& filename)
{
    this->density_image_filename=filename;
    this->density_image_sptr=
            read_from_file<DiscretisedDensity<3,float> >(filename);
    if (is_null_ptr(this->density_image_sptr))
    {
        error(boost::format("Error reading density image %s") %
              this->density_image_filename);
    }
    this->remove_cache_for_integrals_over_attenuation();
}

void
ScatterSimulation::
set_density_image_for_scatter_points_sptr(shared_ptr<DiscretisedDensity<3,float> > arg)
{
    if (is_null_ptr(arg) )
        error("ScatterSimulation: Unable to set the density image for scatter points.");
    this->density_image_for_scatter_points_sptr.reset(
                new VoxelsOnCartesianGrid<float>(*dynamic_cast<VoxelsOnCartesianGrid<float> *>(arg.get())));
    this->sample_scatter_points();
    this->remove_cache_for_integrals_over_attenuation();
}

shared_ptr<DiscretisedDensity<3,float> >
ScatterSimulation::
get_density_image_for_scatter_points_sptr() const
{
    return density_image_for_scatter_points_sptr;
}

void
ScatterSimulation::
set_density_image_for_scatter_points(const std::string& filename)
{
    this->density_image_for_scatter_points_filename=filename;
    this->density_image_for_scatter_points_sptr=
            read_from_file<DiscretisedDensity<3,float> >(filename);

    if (is_null_ptr(this->density_image_for_scatter_points_sptr))
    {
        error(boost::format("Error reading density_for_scatter_points image %s") %
              this->density_image_for_scatter_points_filename);
    }
    this->sample_scatter_points();
    this->remove_cache_for_integrals_over_attenuation();
}

void
ScatterSimulation::
set_image_downsample_factors(float _zoom_xy, float _zoom_z,
                             int _size_zoom_xy, int _size_zoom_z)
{
    zoom_xy = _zoom_xy;
    zoom_z = _zoom_z;
    zoom_size_xy = _size_zoom_xy;
    zoom_size_z = _size_zoom_z;
}

void
ScatterSimulation::
downsample_density_image_for_scatter_points(float _zoom_xy, float _zoom_z,
                 int _size_xy, int _size_z)
{
    zoom_xy = _zoom_xy;
    zoom_z = _zoom_z;
    zoom_size_xy = _size_xy;
    zoom_size_z = _size_z;

    int old_x = dynamic_cast<VoxelsOnCartesianGrid<float> *>(density_image_for_scatter_points_sptr.get())->get_x_size();
    int old_y = dynamic_cast<VoxelsOnCartesianGrid<float> *>(density_image_for_scatter_points_sptr.get())->get_y_size();
    int old_z = dynamic_cast<VoxelsOnCartesianGrid<float> *>(density_image_for_scatter_points_sptr.get())->get_z_size();

    int new_x = _size_xy == -1 ? static_cast<int>(old_x * zoom_xy) : _size_xy;
    int new_y = _size_xy == -1 ? static_cast<int>(old_y * zoom_xy) : _size_xy;
    int new_z = _size_z == -1 ? static_cast<int>(old_z * zoom_z) : _size_z;

    if (new_x%2 == 0 || new_y%2 == 0 )
    {
        warning("ScatterSimulation: The x and y size of the downsampled attenuation image "
                "for scatter points has an even number of voxels. This might be less than ideal."
                "You might want to adjust the zoom factors");
    }

    CartesianCoordinate3D<float> offset_in_mm = dynamic_cast<VoxelsOnCartesianGrid<float> *>(density_image_for_scatter_points_sptr.get())->get_origin();

    ZoomOptions scaling(ZoomOptions::preserve_values);
    zoom_image_in_place( *dynamic_cast<VoxelsOnCartesianGrid<float> *>(density_image_for_scatter_points_sptr.get()),
                         CartesianCoordinate3D<float>(zoom_z, zoom_xy, zoom_xy),
                         offset_in_mm,
                         CartesianCoordinate3D<int>(new_z, new_y, new_x),
                         scaling);

    {
        float image_plane_spacing = dynamic_cast<VoxelsOnCartesianGrid<float> *>(density_image_for_scatter_points_sptr.get())->get_grid_spacing()[1];
        const float num_planes_per_scanner_ring_float =
                proj_data_info_cyl_noarc_cor_sptr->get_ring_spacing() / image_plane_spacing;

        int num_planes_per_scanner_ring = round(num_planes_per_scanner_ring_float);

        if (fabs(num_planes_per_scanner_ring_float - num_planes_per_scanner_ring) > 1.E-2)
            warning(boost::format("ScatterSimulation: DataSymmetriesForBins_PET_CartesianGrid can currently only support z-grid spacing "
                                  "equal to the ring spacing of the scanner divided by an integer. This is not a problem here."
                                  "However, if you are planning to use this in an Scatter Estimation loop it might create problems."
                                  "Reconsider your z-axis downsampling."
                                  "(Image z-spacing is %1% and ring spacing is %2%)") % image_plane_spacing % proj_data_info_cyl_noarc_cor_sptr->get_ring_spacing());
    }

    this->sample_scatter_points();
    this->remove_cache_for_integrals_over_attenuation();
}


void
ScatterSimulation::
set_output_proj_data_sptr(const shared_ptr<ExamInfo>& _exam,
                          const shared_ptr<ProjDataInfo>& _info,
                          const std::string & filename)
{
    if (filename.size() > 0 )
        this->output_proj_data_sptr.reset(new ProjDataInterfile(_exam,
                                                                _info,
                                                                filename,
                                                                std::ios::in | std::ios::out | std::ios::trunc));
    else
        this->output_proj_data_sptr.reset( new ProjDataInMemory(_exam,
                                                                _info));
}

shared_ptr<ProjData>
ScatterSimulation::
get_output_proj_data_sptr() const
{

    if(is_null_ptr(this->output_proj_data_sptr))
    {
        error("ScatterSimulation: No output ProjData set. Aborting.");
    }

    return this->output_proj_data_sptr;
}

void
ScatterSimulation::
set_output_proj_data(const std::string& filename)
{

    if(is_null_ptr(this->proj_data_info_cyl_noarc_cor_sptr))
    {
        error("ScatterSimulation: Template ProjData has not been set. Aborting.");
    }

    this->output_proj_data_filename = filename;
    shared_ptr<ProjData> tmp_sptr;

    if (is_null_ptr(this->template_exam_info_sptr))
    {
        shared_ptr<ExamInfo> exam_info_sptr(new ExamInfo);
        tmp_sptr.reset(new ProjDataInterfile(exam_info_sptr,
                                                       this->proj_data_info_cyl_noarc_cor_sptr->create_shared_clone(),
                                                       this->output_proj_data_filename,std::ios::in | std::ios::out | std::ios::trunc));
    }
    else
    {
        tmp_sptr.reset(new ProjDataInterfile(this->template_exam_info_sptr,
                                             this->proj_data_info_cyl_noarc_cor_sptr->create_shared_clone(),
                                             this->output_proj_data_filename,std::ios::in | std::ios::out | std::ios::trunc));

    }

    set_output_proj_data_sptr(tmp_sptr);
}


void
ScatterSimulation::
set_output_proj_data_sptr(shared_ptr<ProjData> arg)
{
    this->output_proj_data_sptr = arg;
}

void
ScatterSimulation::
set_template_proj_data_info_sptr(shared_ptr<ProjDataInfo> arg)
{
    this->proj_data_info_cyl_noarc_cor_sptr.reset(dynamic_cast<ProjDataInfoCylindricalNoArcCorr* >(arg->clone()));

    if (is_null_ptr(this->proj_data_info_cyl_noarc_cor_sptr))
        error("ScatterSimulation: Can only handle non-arccorrected data");

    // find final size of detection_points_vector
    this->total_detectors =
            this->proj_data_info_cyl_noarc_cor_sptr->get_scanner_ptr()->get_num_rings()*
            this->proj_data_info_cyl_noarc_cor_sptr->get_scanner_ptr()->get_num_detectors_per_ring ();

    // reserve space to avoid reallocation, but the actual size will grow dynamically
    this->detection_points_vector.reserve(static_cast<std::size_t>(this->total_detectors));

    // remove any cached values as they'd be incorrect if the sizes changes
    this->remove_cache_for_integrals_over_attenuation();
    this->remove_cache_for_integrals_over_activity();
}

shared_ptr<ProjDataInfoCylindricalNoArcCorr>
ScatterSimulation::
get_template_proj_data_info_sptr() const
{
    return this->proj_data_info_cyl_noarc_cor_sptr;
}

shared_ptr<ExamInfo>
ScatterSimulation::get_ExamInfo_sptr() const
{
    return this->template_exam_info_sptr;
}

void
ScatterSimulation::
set_template_proj_data_info(const std::string& filename)
{
    this->template_proj_data_filename = filename;
    shared_ptr<ProjData> template_proj_data_sptr =
            ProjData::read_from_file(this->template_proj_data_filename);

    this->set_exam_info_sptr(template_proj_data_sptr->get_exam_info_sptr());

    shared_ptr<ProjDataInfo> tmp_proj_data_info_sptr =
            template_proj_data_sptr->get_proj_data_info_sptr();

    this->set_template_proj_data_info_sptr(tmp_proj_data_info_sptr);

    if (downsample_scanner_dets > 1 || downsample_scanner_rings > 1)
        downsample_scanner();
}

void
ScatterSimulation::set_template_proj_data_info(const ProjDataInfo& arg)
{
    this->proj_data_info_cyl_noarc_cor_sptr.reset(dynamic_cast<ProjDataInfoCylindricalNoArcCorr* >(arg.clone()));

    if (is_null_ptr(this->proj_data_info_cyl_noarc_cor_sptr))
        error("ScatterSimulation: Can only handle non-arccorrected data");

    // find final size of detection_points_vector
    this->total_detectors =
            this->proj_data_info_cyl_noarc_cor_sptr->get_scanner_ptr()->get_num_rings()*
            this->proj_data_info_cyl_noarc_cor_sptr->get_scanner_ptr()->get_num_detectors_per_ring ();

    // reserve space to avoid reallocation, but the actual size will grow dynamically
    this->detection_points_vector.reserve(static_cast<std::size_t>(this->total_detectors));

    // remove any cached values as they'd be incorrect if the sizes changes
    this->remove_cache_for_integrals_over_attenuation();
    this->remove_cache_for_integrals_over_activity();
}

void
ScatterSimulation::
set_exam_info_sptr(const shared_ptr<ExamInfo>& arg)
{
    this->template_exam_info_sptr = arg;
}

Succeeded
ScatterSimulation::downsample_scanner(int new_num_rings, int new_num_dets)
{
    if (new_num_rings == -1)
        if(downsample_scanner_rings > -1)
            new_num_rings = downsample_scanner_rings;
        else
            return Succeeded::no;

    if (new_num_dets == -1)
        if(downsample_scanner_dets > -1)
            new_num_dets = downsample_scanner_dets;
        else
            return Succeeded::no;


    info("ScatterSimulator: Downsampling scanner of template info ...");
    // copy localy.
    shared_ptr<Scanner> new_scanner_sptr( new Scanner(*this->proj_data_info_cyl_noarc_cor_sptr->get_scanner_ptr()));

    // preserve the length of the scanner
    float scanner_length = new_scanner_sptr->get_num_rings()* new_scanner_sptr->get_ring_spacing();

    new_scanner_sptr->set_num_rings(new_num_rings);
    new_scanner_sptr->set_num_detectors_per_ring(new_num_dets);
    new_scanner_sptr->set_ring_spacing(static_cast<float>(scanner_length/new_scanner_sptr->get_num_rings()));
    new_scanner_sptr->set_max_num_non_arccorrected_bins(new_num_dets-1);
    new_scanner_sptr->set_default_bin_size(new_scanner_sptr->get_default_bin_size()*2);

    // Find how much is the delta ring
    // If the previous projdatainfo had max segment == 1 then should be from SSRB
    // in ScatterEstimation. Otherwise use the max possible.
    int delta_ring = proj_data_info_cyl_noarc_cor_sptr->get_num_segments() == 1 ? delta_ring = 0 :
            new_scanner_sptr->get_num_rings()-1;

    this->proj_data_info_cyl_noarc_cor_sptr.reset(dynamic_cast<ProjDataInfoCylindricalNoArcCorr* >(
                                                      ProjDataInfo::ProjDataInfoCTI(new_scanner_sptr,
                                                                                    1, delta_ring,
                                                                                    new_scanner_sptr->get_num_detectors_per_ring()/2,
                                                                                    new_scanner_sptr->get_max_num_non_arccorrected_bins(),
                                                                                    false)->clone()));

    if(!is_null_ptr(output_proj_data_sptr))
    {
        this->output_proj_data_sptr.reset(new ProjDataInMemory(this->template_exam_info_sptr,
                                                               this->proj_data_info_cyl_noarc_cor_sptr->create_shared_clone()));
        this->output_proj_data_sptr->fill(0.0);
        info("ScatterSimulation: output projection data created.");
    }

    // find final size of detection_points_vector
    this->total_detectors =
            this->proj_data_info_cyl_noarc_cor_sptr->get_scanner_ptr()->get_num_rings()*
            this->proj_data_info_cyl_noarc_cor_sptr->get_scanner_ptr()->get_num_detectors_per_ring ();

    // reserve space to avoid reallocation, but the actual size will grow dynamically
    this->detection_points_vector.reserve(static_cast<std::size_t>(this->total_detectors));

    // remove any cached values as they'd be incorrect if the sizes changes
    this->remove_cache_for_integrals_over_attenuation();
    this->remove_cache_for_integrals_over_activity();

    return Succeeded::yes;
}

Succeeded ScatterSimulation::default_downsampling(bool all_images)
{
    if(is_null_ptr(proj_data_info_cyl_noarc_cor_sptr))
            return Succeeded::no;

    float total_axial_length = proj_data_info_cyl_noarc_cor_sptr->get_scanner_sptr()->get_num_rings() *
            proj_data_info_cyl_noarc_cor_sptr->get_scanner_sptr()->get_ring_spacing();

    int new_num_rings = round(total_axial_length / 20.F + 0.5F);

    if(downsample_scanner(new_num_rings, 32) == Succeeded::no)
        return Succeeded::no;


    ZoomOptions scaling(ZoomOptions::preserve_values);

    // Downsample the activity and attanuation images
    shared_ptr<VoxelsOnCartesianGrid<float> > tmpl_density( new VoxelsOnCartesianGrid<float>(*proj_data_info_cyl_noarc_cor_sptr));

    if(!is_null_ptr(activity_image_sptr) && all_images)
    {
        VoxelsOnCartesianGrid<float>* tmp_act = dynamic_cast<VoxelsOnCartesianGrid<float>* >(activity_image_sptr.get());
        float _zoom_xy =
                tmp_act->get_voxel_size().x() / tmpl_density->get_voxel_size().x();
        float _zoom_z =
                tmp_act->get_voxel_size().z() / tmpl_density->get_voxel_size().z();

        BasicCoordinate<3,int> new_size = make_coordinate(tmp_act->get_z_size(),
                                                          tmp_act->get_y_size(),
                                                          tmp_act->get_x_size());

        zoom_image_in_place(*tmp_act ,
                            CartesianCoordinate3D<float>(_zoom_z, _zoom_xy, _zoom_xy),
                            CartesianCoordinate3D<float>(0,0,0),
                            new_size,
                            scaling);

        this->remove_cache_for_integrals_over_activity();
    }

    if(!is_null_ptr(density_image_sptr) && all_images)
    {
        VoxelsOnCartesianGrid<float>* tmp_att = dynamic_cast<VoxelsOnCartesianGrid<float>* >(density_image_sptr.get());
        float _zoom_xy =
                tmp_att->get_voxel_size().x() / tmpl_density->get_voxel_size().x();
        float _zoom_z =
                tmp_att->get_voxel_size().z() / tmpl_density->get_voxel_size().z();

        BasicCoordinate<3,int> new_size = make_coordinate(tmp_att->get_z_size(),
                                                          tmp_att->get_y_size(),
                                                          tmp_att->get_x_size());

        zoom_image_in_place(*tmp_att ,
                            CartesianCoordinate3D<float>(_zoom_z, _zoom_xy, _zoom_xy),
                            CartesianCoordinate3D<float>(0,0,0),
                            new_size, scaling);

        this->remove_cache_for_integrals_over_attenuation();
    }

    if(!is_null_ptr(density_image_for_scatter_points_sptr))
    {
        VoxelsOnCartesianGrid<float>* tmp_att = dynamic_cast<VoxelsOnCartesianGrid<float>* >(density_image_for_scatter_points_sptr.get());
        float _zoom_xy =
                tmp_att->get_voxel_size().x() / tmpl_density->get_voxel_size().x();
        float _zoom_z =
                tmp_att->get_voxel_size().z() / tmpl_density->get_voxel_size().z();

        BasicCoordinate<3,int> new_size = make_coordinate(tmpl_density->get_z_size(),
                                                          tmpl_density->get_y_size(),
                                                          tmpl_density->get_x_size());

        downsample_density_image_for_scatter_points(_zoom_xy, _zoom_z,tmpl_density->get_x_size(), tmpl_density->get_z_size());

        this->sample_scatter_points();
        this->remove_cache_for_integrals_over_attenuation();
    }

    return Succeeded::yes;
}

void
ScatterSimulation::
set_attenuation_threshold(const float arg)
{
    attenuation_threshold = arg;
}

void
ScatterSimulation::
set_random_point(const bool arg)
{
    random = arg;
}

void
ScatterSimulation::
set_cache_enabled(const bool arg)
{
    use_cache = arg;
}

END_NAMESPACE_STIR
