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
#include "opendlv-standard-message-set.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{0};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("camera")) || (0 == commandlineArguments.count("cid")) || (0 == commandlineArguments.count("width")) || (0 == commandlineArguments.count("height")) || (0 == commandlineArguments.count("bpp")) || (0 == commandlineArguments.count("freq")) ) {
        std::cerr << argv[0] << " interfaces with the given V4L camera id (i.e., 0 for /dev/video0 or a valid connection string) and publishes it to a running OpenDaVINCI session using the OpenDLV Standard Message Set." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --camera=<V4L id> --cid=<OpenDaVINCI session> --width=<width> --height=<height> --bpp=<bits per pixel> [--name=<unique name for the associated shared memory>] [--id=<Identifier in case of multiple cameras>] [--verbose]" << std::endl;
        std::cerr << "         --freq:    desired bits per pixel of a frame (must be either 8 or 24)" << std::endl;
        std::cerr << "         --width:   desired width of a frame" << std::endl;
        std::cerr << "         --height:  desired height of a frame" << std::endl;
        std::cerr << "         --bpp:     desired bits per pixel of a frame (must be either 8 or 24)" << std::endl;
        std::cerr << "         --name:    when omitted, '/cam0' is chosen" << std::endl;
        std::cerr << "         --verbose: when set, the raw image is displayed" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --camera=/dev/video0 --name=cam0" << std::endl;
        retCode = 1;
    }
    else {
        std::unique_ptr<cv::VideoCapture> videoStream{nullptr};
        try {
            const uint32_t AUTO{(commandlineArguments["camera"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["camera"])) : 0};
            videoStream.reset(new cv::VideoCapture(AUTO));
        }
        catch (...) {
            videoStream.reset(new cv::VideoCapture(commandlineArguments["camera"]));
        }

        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const uint32_t BPP{static_cast<uint32_t>(std::stoi(commandlineArguments["bpp"]))};
        const float FREQ{static_cast<float>(std::stof(commandlineArguments["freq"]))};

        if ( (BPP != 24) && (BPP != 8) ) {
            std::cerr << argv[0] << ": bits per pixel must be either 24 or 8; found " << BPP << "." << std::endl;
            return retCode = 1;
        }
        if ( !(FREQ > 0) ) {
            std::cerr << argv[0] << ": freq must be larger than 0; found " << FREQ << "." << std::endl;
            return retCode = 1;
        }
        const uint32_t SIZE{WIDTH * HEIGHT * BPP/8};
        const std::string NAME{(commandlineArguments["name"].size() != 0) ? commandlineArguments["name"] : "/cam0"};
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        (void)ID;

        if (videoStream && videoStream->isOpened()) {
            videoStream->set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
            videoStream->set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
            videoStream->set(CV_CAP_PROP_FORMAT, (BPP == 24 ? CV_CAP_MODE_RGB : CV_CAP_MODE_GRAY));

            // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME, SIZE});
            if (sharedMemory && sharedMemory->valid()) {
                std::clog << argv[0] << ": Data from camera '" << commandlineArguments["camera"]<< "' available in shared memory '" << sharedMemory->name() << "' (" << sharedMemory->size() << ")." << std::endl;

                auto timeTrigger = [&videoStream, &sharedMemory, &VERBOSE](){
                    cv::Mat frameData;
                    bool retVal = videoStream->read(frameData);
                    if (retVal) {
                        sharedMemory->lock();
                        ::memcpy(sharedMemory->data(), reinterpret_cast<char*>(frameData.data), frameData.step * frameData.rows);
                        sharedMemory->unlock();
                        sharedMemory->notifyAll();
                    }
                    if (retVal && VERBOSE) {
                        cv::imshow(sharedMemory->name(), frameData);
                    }
                    cv::waitKey(10);
                    return retVal;
                };

                od4.timeTrigger(FREQ, timeTrigger);
            }
            else {
                std::cerr << argv[0] << ": Failed to create shared memory '" << NAME << "'." << std::endl;
            }
        }
        else {
            std::cerr << argv[0] << ": Failed to open camera '" << commandlineArguments["camera"] << "'." << std::endl;
        }
    }
    return retCode;
}

