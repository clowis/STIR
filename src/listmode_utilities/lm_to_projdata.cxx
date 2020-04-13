
#include "stir/listmode/LmToTimeDiffs.h"
#include "stir/IO/InputFileFormatRegistry.h"

#ifndef STIR_NO_NAMESPACES
using std::cerr;
using std::endl;
#endif

USING_NAMESPACE_STIR



int main(int argc, char * argv[])
{
  if (argc>1)
    {
      if (strcmp(argv[1], "--help") == 0 ||
          strcmp(argv[1], "-?") == 0) {
        cerr << "\nUsage: " << argv[0] << " [par_file]\n"
          << "Run " << argv[0] << " --input-formats to list the supported input formats\n";
        exit(EXIT_SUCCESS);
      }
      // Display the supported inputs, we need this in order to know
      // which listmode files are supported
      if (strcmp(argv[1], "--input-formats") == 0)
      {
        cerr << endl << "Supported input file formats:\n";
        InputFileFormatRegistry<CListModeData>::default_sptr()->
          list_registered_names(cerr);
        exit(EXIT_SUCCESS);
      }
      if (strcmp(argv[1], "--test_timing_positions") == 0)
      {
        cerr << "A test function for TOF data which I could not fit anywhere else right now:\n"
          "It is going to fill every segment with the index number of the respective TOF position \n"
          "and then stop.\n";
        std::cout << argc << std::endl;
        std::cout << argv[0] << "\n" << argv[1] << "\n" << argv[2] << std::endl;
        LmToProjData application(argc == 3 ? argv[2] : 0);
        application.run_tof_test_function();
        exit(EXIT_SUCCESS);
      }
  }
  LmToTimeDiffs application(argc==2 ? argv[1] : 0);
  application.process_data();

  return EXIT_SUCCESS;
}

