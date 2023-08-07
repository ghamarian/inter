#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <opencv2/opencv.hpp>
#include <string>

namespace bip = boost::interprocess;

int main() {
    // Open the shared memory segment
    bip::managed_shared_memory segment(bip::open_only, "ImageShm");

    // Find the semaphore in the shared memory
    bip::interprocess_semaphore *sem_image_ready = segment.find<bip::interprocess_semaphore>("SemImageReady").first;
    bip::interprocess_semaphore *sem_show_done = segment.find<bip::interprocess_semaphore>("SemShowDone").first;
    bip::interprocess_semaphore *sem_shutdown = segment.find<bip::interprocess_semaphore>("SemShutdown").first;

    // Define an allocator that uses the segment manager
    typedef bip::allocator<bip::offset_ptr<unsigned char>, bip::managed_shared_memory::segment_manager> ShmemAllocator;
    typedef bip::vector<bip::offset_ptr<unsigned char>, ShmemAllocator> ImageVector;

    // Find the vector of image offsets in the shared memory
    auto imageVecPair = segment.find<ImageVector>("ImageVec");
    if(imageVecPair.second == 0) {
        std::cout << "Failed to find ImageVec\n";
        return -1;
    }
    ImageVector* imageVec = imageVecPair.first;

    int index = 0;
    while (true) {
        if (sem_shutdown->try_wait()) {
            break;
        }

        // Wait for the next image to be ready
        sem_image_ready->wait();

        // Retrieve the offset pointer from the vector
        bip::offset_ptr<unsigned char> myImg = (*imageVec)[index];

        // Get the image size information
        std::string idxStr = std::to_string(index);
        auto dimsPair = segment.find<int>(idxStr.c_str());
        if(dimsPair.second == 0) {
            std::cout << "Failed to find dims\n";
            return -1;
        }
        int* dims = dimsPair.first;
        cv::Mat image(dims[0], dims[1], CV_8UC(dims[2]), static_cast<void*>(&*myImg));

        // Show the image
        cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);
        cv::imshow("Display window", image);

        // Wait for a key press
        cv::waitKey(0);

        // Signal that we are done showing the image
        sem_show_done->post();

        // Increment the index
        index++;
    }

    return 0;
}
