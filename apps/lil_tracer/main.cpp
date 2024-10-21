#include <iostream>
#include <lt/lt.h>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void print_progress(double percentage, float t) {
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    printf("\t%f (ms)", t);
    fflush(stdout);
}

int main(int argc, char* argv[])
{   
    lt::State::log_level = lt::logWarning;
    for (int a = 1; a < argc; a++) {

        lt::Renderer ren;
        lt::Scene scn;

        lt::generate_from_path(argv[a], scn, ren);

        float time = 0.;

        for (int s = 0; s < ren.max_sample;  s++) {
            float t = ren.render(scn);

            time += t;

            print_progress(double(s) / double(ren.max_sample - 1.0), t);
        }

        std::cout << "\nTime elapsed : " << time << " (ms) " << std::endl;

        lt::save_sensor_exr(*ren.sensor, std::string(argv[a]) + ".exr");
        
    }

    return 0;
}