/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"

#include <libyuv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " interfaces with the given OpenCV-encapsulated camera (e.g., a V4L identifier like 0 or a stream address) and provides the captured image in two shared memory areas: one in I420 format and one in ARGB format." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --camera=<V4L dev node> --width=<width> --height=<height> [--name.i420=<unique name for the shared memory in I420 format>] [--name.argb=<unique name for the shared memory in ARGB format>] [--yuyv422] [--verbose]" << std::endl;
        std::cerr << "         --name.i420: name of the shared memory for the I420 formatted image; when omitted, video0.i420 is chosen" << std::endl;
        std::cerr << "         --name.argb: name of the shared memory for the I420 formatted image; when omitted, video0.argb is chosen" << std::endl;
        std::cerr << "         --width:     desired width of a frame" << std::endl;
        std::cerr << "         --height:    desired height of a frame" << std::endl;
        std::cerr << "         --verbose:   display captured image" << std::endl;
        std::cerr << "Example: " << argv[0] << " --width=640 --height=480 --verbose" << std::endl;
    } else {
        const std::string NAME_I420{(commandlineArguments["name.i420"].size() != 0) ? commandlineArguments["name.i420"] : "video0.i420"};
        const std::string NAME_ARGB{(commandlineArguments["name.argb"].size() != 0) ? commandlineArguments["name.argb"] : "video0.argb"};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};

        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        std::unique_ptr<cluon::SharedMemory> sharedMemoryARGB(new cluon::SharedMemory{NAME_ARGB});
        if (sharedMemoryARGB && sharedMemoryARGB->valid()) {
            std::clog << argv[0] << ": Attached to '" << sharedMemoryARGB->name() << "' (" << sharedMemoryARGB->size() << " bytes)." << std::endl;

            cv::Mat ARGB(HEIGHT, WIDTH, CV_8UC4, sharedMemoryARGB->data());

            while (!cluon::TerminateHandler::instance().isTerminated.load()) {
                sharedMemoryARGB->wait();
                sharedMemoryARGB->lock();
                if (VERBOSE) {
                  cv::imshow(sharedMemoryARGB->name(), ARGB);
                  cv::waitKey(10); // Necessary to actually display the image.
                }
                sharedMemoryARGB->unlock();
            }
        }
        retCode = 0;
    }

    return retCode;
}

