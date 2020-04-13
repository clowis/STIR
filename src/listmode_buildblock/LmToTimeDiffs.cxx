#include "stir/listmode/LmToTimeDiffs.h"
#include "stir/listmode/CListRecordROOT.h"
#include "stir/CPUTimer.h"
#include <iostream>
#include <fstream>
#include "stir/is_null_ptr.h"

START_NAMESPACE_STIR


LmToTimeDiffs::
LmToTimeDiffs()
{
  set_defaults();
}

LmToTimeDiffs::
LmToTimeDiffs(const char * const par_filename):
LmToProjData(par_filename)
{

}


void
LmToTimeDiffs::
process_data()
{
    assert(!is_null_ptr(template_proj_data_info_ptr));

    CPUTimer timer;
    timer.start();

    shared_ptr <CListRecord> record_sptr = lm_data_ptr->get_empty_record_sptr();
    CListRecord& record = *record_sptr;


    unsigned long int more_events =
            do_time_frame? 1 : num_events_to_store;

    std::ofstream outfile;
    outfile.open(output_filename_prefix);

    while(more_events)
    {
        if (lm_data_ptr->get_next_record(record) == Succeeded::no)
        {
            // no more events in file for some reason
            break; //get out of while loop
        }

        if(record.is_event())
        {
            Bin bin;
            get_bin_from_event(bin, record.event());

            CListRecordROOT* tmp = dynamic_cast<CListRecordROOT*>(record_sptr.get());

            if(is_null_ptr(tmp))
                error("Nikos: This should not have happened...");

            double dt = tmp->event().get_delta_time();

            bin.set_bin_value(1);

            if (bin.timing_pos_num() < 0.0)
                dt *= -1;

            outfile << dt << std::endl;
        }
    }
    outfile.close();
    timer.stop();

}

END_NAMESPACE_STIR
