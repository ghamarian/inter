#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <opencv2/opencv.hpp>

namespace bip = boost::interprocess;

int main() {
    struct shm_remove {
        shm_remove() { bip::shared_memory_object::remove("ImageShm"); }
        ~shm_remove() { bip::shared_memory_object::remove("ImageShm"); }
    } remover;

    bip::managed_shared_memory segment(bip::create_only, "ImageShm", 1024 * 1024 * 128); // increase shared memory size

    bip::interprocess_semaphore *sem_image_ready = segment.construct<bip::interprocess_semaphore>("SemImageReady")(0);
    bip::interprocess_semaphore *sem_show_done = segment.construct<bip::interprocess_semaphore>("SemShowDone")(0);
    bip::interprocess_semaphore *sem_shutdown = segment.construct<bip::interprocess_semaphore>("SemShutdown")(0);

// Define a vector of pointers to the images
    typedef bip::allocator<bip::offset_ptr<unsigned char>, bip::managed_shared_memory::segment_manager> ShmemAllocator;
    typedef bip::vector<bip::offset_ptr<unsigned char>, ShmemAllocator> ImageVector;
    ImageVector* imageVec = segment.construct<ImageVector>("ImageVec")(segment.get_segment_manager());

    std::vector<std::string> imagePaths = {"/Users/amir/amirtje.jpeg",
                                           "/Users/amir/Downloads/image_cab3676c510915ad5bbdbac984dbe586.jpg",
                                           "/Users/amir/Downloads/image_5c3aac1c7eaa4261b838618dca2af35a.jpg"};

    for (const auto& imagePath : imagePaths) {
        cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cout << "Could not open or find the image" << std::endl;
            return -1;
        }

        // Allocate memory and construct the image in the shared memory
        int imgSize = image.total() * image.elemSize();
        std::cout << "image size: " << imgSize << std::endl;
        unsigned char* myImg = static_cast<unsigned char*>(segment.allocate(imgSize));
        std::memcpy(myImg, image.data, imgSize);

        // Create an offset pointer and push it onto the vector
        bip::offset_ptr<unsigned char> myImgPtr(myImg);
        imageVec->push_back(myImgPtr);


        // Share image size information
        std::string index = std::to_string(imageVec->size() - 1);
        int* dims = segment.construct<int>(index.c_str())[3]();
        dims[0] = image.rows;
        dims[1] = image.cols;
        dims[2] = image.channels();

        //print image size
        std::cout << "image size: " << dims[0] << " " << dims[1] << " " << dims[2] << std::endl;

        // Signal that the image is ready
        sem_image_ready->post();

        if (&imagePath == &imagePaths.back()) {
            sem_shutdown->post();
        }


        // Wait for the show process to finish showing the image
        sem_show_done->wait();
    }
    if (imagePaths.empty()) {
        sem_shutdown->post();
    }

    return 0;
}
