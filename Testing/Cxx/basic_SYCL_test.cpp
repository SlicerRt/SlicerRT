/**
 * @file
 * @brief Basic test to check that SYCL is running correctly on the selected device ("gpu", "cpu")
 * @author Declan Garvey (IFIC, CSIC-UV)
 */

#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <sycl/sycl.hpp>

/**
 * @brief Basic function to free memory that was allocated during the test
 * @param q The sycl queue used
 * @param testArrayOnDevice The unsigned int array used for testing
 * @param submitFlag Error flag to signal unsuccessful submiting
 * @param copyFlag Error flag to signal unsuccessful copying
 * @param arithmeticFlag Error flag to signal unsuccessful arithmetics
 */
void cleanup(sycl::queue &q, unsigned *testArrayOnDevice, bool *submitFlag, bool *copyFlag,
             bool *arithmeticFlag)
{
  if (testArrayOnDevice)
    sycl::free(testArrayOnDevice, q);
  if (submitFlag)
    sycl::free(submitFlag, q);
  if (copyFlag)
    sycl::free(copyFlag, q);
  if (arithmeticFlag)
    sycl::free(arithmeticFlag, q);
}

/**
 * @brief main
 * @param argc Argument counts
 * @param argv Variables
 * @return EXIT_FAILURE if error, EXIT_SUCCESS if ok
 */
int main(int argc, char **argv)
{
  CLI::App app{"Simple ctest to check if SYCL is functioning correctly"};
  std::string deviceName = "gpu";
  app.add_option<std::string>("-d,-D,--DeviceName", deviceName,
                              "The name of the device to run test on, Accepts: gpu or cpu");
  CLI11_PARSE(app, argc, argv);

  // List available platforms
  bool gpuAvailable = false;
  auto platforms = sycl::platform::get_platforms();
  for (auto &platform : platforms) {
    std::cout << "Platform: " << platform.get_info<sycl::info::platform::name>() << std::endl;
    auto devices = platform.get_devices();
    for (auto &device : devices) {
      std::cout << "  Device: " << device.get_info<sycl::info::device::name>() << std::endl;
    }
    gpuAvailable |= !platform.get_devices(sycl::info::device_type::gpu).empty();
  }
  if (deviceName == "gpu" && !gpuAvailable) {
    std::cerr << deviceName << " SYCL fail: No GPUs are available on this system" << std::endl;
    return 255;
  }

  // Create SYCL queue
  sycl::queue q;
  if (deviceName == "cpu") {
    q = sycl::queue{sycl::cpu_selector_v};

    if (q.get_device().is_cpu() == false) {
      std::cerr << deviceName << " SYCL fail: Unable to create sycl queue" << std::endl;
      return EXIT_FAILURE;
    }
  } else if (deviceName == "gpu") {
    q = sycl::queue{sycl::gpu_selector_v};

    if (q.get_device().is_gpu() == false) {
      std::cerr << deviceName << " SYCL fail: Unable to create sycl queue" << std::endl;
      return EXIT_FAILURE;
    }
  } else {
    std::cerr << "Unknown device name: " << deviceName << ", Accepted values: cpu, gpu"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Define a simple incrementing array that will be used for testing
  const unsigned arraySize = 10000;
  unsigned testArray[arraySize];
  for (unsigned i = 0; i < arraySize; i++)
    testArray[i] = i;

  // Copy to Device
  unsigned *testArrayOnDevice = sycl::malloc_device<unsigned>(arraySize, q);
  q.memcpy(testArrayOnDevice, testArray, sizeof(unsigned) * arraySize);

  // A flag that remains false until the queue is submitted successful
  bool *submitFlag = sycl::malloc_shared<bool>(1, q);
  submitFlag[0] = false; // Assumed fail until proven pass

  // A flag that becomes false if copying to device fails
  bool *copyFlag = sycl::malloc_shared<bool>(1, q);
  copyFlag[0] = true; // Assumed pass until proven fail

  // A flag that becomes false if basic arithmetic test on device fails
  bool *arithmeticFlag = sycl::malloc_shared<bool>(1, q);
  arithmeticFlag[0] = true; // Assumed passed until proven fail

  q.wait();

  q.submit([&](sycl::handler &h) {
     h.parallel_for(sycl::range<1>(arraySize), [=](sycl::id<1> idx) {
       submitFlag[0] = true;
       if (testArrayOnDevice[idx[0]] != idx[0])
         copyFlag[0] = false;

       // Test very basic arithmetic equation x = 2 * (x + 3)
       testArrayOnDevice[idx[0]] += 3;
       testArrayOnDevice[idx[0]] *= 2;
       if (testArrayOnDevice[idx[0]] != 2 * (idx[0] + 3))
         arithmeticFlag[0] = false;
     });
   }).wait();

  if (submitFlag[0] == false) {
    std::cerr << deviceName << " SYCL fail: Unsuccessfully submitted to device" << std::endl;
    cleanup(q, testArrayOnDevice, submitFlag, copyFlag, arithmeticFlag);
    return EXIT_FAILURE;
  }
  if (copyFlag[0] == false) {
    std::cerr << deviceName << " SYCL fail: Copy from host to device unsuccessful" << std::endl;
    cleanup(q, testArrayOnDevice, submitFlag, copyFlag, arithmeticFlag);
    return EXIT_FAILURE;
  }
  if (arithmeticFlag[0] == false) {
    std::cerr << deviceName << " SYCL fail: basic arithmetics on device unsuccessful" << std::endl;
    cleanup(q, testArrayOnDevice, submitFlag, copyFlag, arithmeticFlag);
    return EXIT_FAILURE;
  }

  q.memcpy(testArray, testArrayOnDevice, sizeof(unsigned) * arraySize).wait();
  for (unsigned i = 1; i < arraySize; i++) {
    if (testArray[i] != 2 * (i + 3)) {
      std::cerr << deviceName << " SYCL fail: Copy from device to host unsuccessful" << std::endl;
      cleanup(q, testArrayOnDevice, submitFlag, copyFlag, arithmeticFlag);
      return EXIT_FAILURE;
    }
  }

  cleanup(q, testArrayOnDevice, submitFlag, copyFlag, arithmeticFlag);
  return EXIT_SUCCESS;
}
