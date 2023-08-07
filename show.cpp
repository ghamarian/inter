#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <opencv2/opencv.hpp>

namespace bip = boost::interprocess;

int main() {
    // Open the shared memory object
    bip::managed_shared_memory segment(bip::open_only, "ImageShm");

    // Find the semaphore in the shared memory
    bip::interprocess_semaphore *sem_image_ready = segment.find<bip::interprocess_semaphore>("SemImageReady").first;
    bip::interprocess_semaphore *sem_show_done = segment.find<bip::interprocess_semaphore>("SemShowDone").first;
    bip::interprocess_semaphore *sem_shutdown = segment.find<bip::interprocess_semaphore>("SemShutdown").first;


    while (true) {
        if (!sem_image_ready->try_wait()) {
            // If the image is not ready, check if a shutdown signal has been sent
            if (sem_shutdown->try_wait()) {
                // If shutdown signal is received, break the loop
                break;
            }
            continue;
        }  // Wait for a signal that the image is ready

        // Get the image from the shared memory
        int* dims = segment.find<int>("ImageSize").first;
        unsigned char* myImg = segment.find<unsigned char>("RawImage").first;
        cv::Mat image(dims[0], dims[1], CV_8UC(dims[2]), myImg);

        // Show the image
        cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);
        cv::imshow("Display window", image);

        // Wait for a key press
        cv::waitKey(0);

        sem_show_done->post();

    }

    return 0;
}
