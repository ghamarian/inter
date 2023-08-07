#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <opencv2/opencv.hpp>

namespace bip = boost::interprocess;

int main() {

    // Define image paths
    std::vector<std::string> imagePaths = {"/Users/amir/amirtje.jpeg",
                                           "/Users/amir/Downloads/image_cab3676c510915ad5bbdbac984dbe586.jpg",
                                           "/Users/amir/Downloads/image_5c3aac1c7eaa4261b838618dca2af35a.jpg"};

    // Determine the largest image size
    int maxSize = 0;
    for (const auto& imagePath : imagePaths) {
        cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cout << "Could not open or find the image" << std::endl;
            return -1;
        }

        int imgSize = image.total() * image.elemSize();
        if (imgSize > maxSize) {
            maxSize = imgSize;
        }
    }

    // Remove shared memory on construction and destruction
    struct shm_remove {
        shm_remove() { bip::shared_memory_object::remove("ImageShm"); }
        ~shm_remove() { bip::shared_memory_object::remove("ImageShm"); }
    } remover;

    // Create semaphores in shared memory
    bip::managed_shared_memory segment(bip::create_only, "ImageShm", maxSize + 1024);
    bip::interprocess_semaphore *sem_image_ready = segment.construct<bip::interprocess_semaphore>("SemImageReady")(0);
    bip::interprocess_semaphore *sem_show_done = segment.construct<bip::interprocess_semaphore>("SemShowDone")(0);
    bip::interprocess_semaphore *sem_shutdown = segment.construct<bip::interprocess_semaphore>("SemShutdown")(0);

    for (const auto& imagePath : imagePaths) {
        cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cout << "Could not open or find the image" << std::endl;
            return -1;
        }

        // Construct the image in the shared memory
        int imgSize = image.total() * image.elemSize();
        unsigned char* myImg = segment.construct<unsigned char>("RawImage")[imgSize](0);
        std::memcpy(myImg, image.data, imgSize);

        // Share image size information
        int* dims = segment.construct<int>("ImageSize")[3]();
        dims[0] = image.rows;
        dims[1] = image.cols;
        dims[2] = image.channels();

        // Signal that the image is ready
        sem_image_ready->post();

        // Wait for the show process to finish showing the image
        sem_show_done->wait();

        // Destroy the objects
        segment.destroy_ptr(myImg);
        segment.destroy_ptr(dims);
    }

    // Signal that we are done
    sem_shutdown->post();

    return 0;
}
