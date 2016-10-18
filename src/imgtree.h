#include <cct/metric.h>

#include <opencv2/highgui/highgui.hpp>

struct arg_int * imread_flags = nullptr;
struct arg_file * input_files = nullptr;

template<typename Weight, template<typename, int> class WeightFunctor>
int process()
{
    std::cout << "id,name,width,height,leaves,edges,components,height,roots,degenerates,minTime,maxTime,meanTime\n";
    for(int i = 0; i < input_files->count; ++i)
    {
        cv::Mat image = cv::imread(input_files->filename[i], imread_flags->ival[0]);
        if(!image.data)
        {
            std::cerr << "Can't open \"" << input_files->filename[i] << "\"" << std::endl;
            return EXIT_FAILURE;
        }

        process<Weight, WeightFunctor>(i, input_files->filename[i], image);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char * argv[])
{
    struct arg_lit * info = arg_lit0(NULL, "info", NULL);
    struct arg_int * edge_norm = arg_int0("e", "edge-norm", "", NULL);
    struct arg_file * outname = arg_file0("o", NULL, "<file>", NULL);
    struct arg_end  * end = arg_end(20);
    void * argtable[] = {
        info,
        imread_flags = arg_int0(NULL, "imread-flags", "", NULL),
        edge_norm,
        child_list = arg_lit0(NULL, "child-list", NULL),
        measurements = arg_int0("n", "measurements", "", NULL),
        tile_width   = arg_int0(NULL, "tile-width", "", NULL),
        tile_height  = arg_int0(NULL, "tile-height", "", NULL),
        parallel_depth   = arg_int0("d", "parallel-depth", "", NULL),
        parallel_nomerge = arg_lit0(NULL, "parallel-nomerge", NULL),
        outname,
        input_files = arg_filen(NULL, NULL, "<image>", 1, argc-1, NULL),
        end };
    // defaults
    imread_flags->ival[0] = CV_LOAD_IMAGE_ANYCOLOR;//8UC?
    edge_norm->ival[0] = 0,
    measurements->ival[0] = 1;
    parallel_depth->ival[0] = 0;
    tile_width->ival[0] = 64;
    tile_height->ival[0] = 16;

    int error_count = arg_parse(argc, argv, argtable);

    if(info->count)
    {
        std::cout << "[timing]\n"
            << "period="
                << std::chrono::high_resolution_clock::period::num << '/'
                << std::chrono::high_resolution_clock::period::den << '\n'
            << "steady=" << std::chrono::high_resolution_clock::is_steady << std::endl;
        return EXIT_SUCCESS;
    }

    if(error_count)
    {
        std::cerr << "FUJ" << std::endl;
        return EXIT_FAILURE;
    }

    int retval = EXIT_FAILURE;

    switch(edge_norm->ival[0])
    {
        case 0 :
            retval = process<uint8_t, cct::metric::LInf>();
            break;
        case 1 :
            retval = process<uint16_t, cct::metric::L1>();
            break;
        case 2 :
            retval = process<float, cct::metric::L2>();
            break;
        default :
            std::cerr << "L" << edge_norm->ival[0] << " norm not implemented" << std::endl;
    }

    return retval;
}

namespace boost
{

void assertion_failed(char const * expr, char const * function, char const * file, long line)
{
    std::cerr << "Assertion failed : " << expr << ", " << function << ", " << file << ":" << line << std::endl;
    //__builtin_trap();
    abort();
}

}
