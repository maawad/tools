#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <thread>

struct cpu_timer
{
public:
    cpu_timer() : start_time(), end_time() {}

    void start()
    {
        start_time = std::chrono::high_resolution_clock::now();
    }

    void stop()
    {
        end_time = std::chrono::high_resolution_clock::now();
    }

    double elapsed_seconds() const
    {
        std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        return elapsed_seconds.count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
};

struct file
{
    file(std::string filename, uint64_t bytes) : filename{filename}, file_size{bytes}, data{} {}
    file() : filename{"test.bin"}, file_size{1 << 30}, data{} {}

    void write() const
    {
        std::ofstream output_file(filename, std::ios::binary);
        if (!output_file)
        {
            std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
            return;
        }
        output_file.write(reinterpret_cast<const char *>(data.data()), file_size);
    }
    bool read()
    {
        std::ifstream input_file(filename, std::ios::binary);
        if (!input_file)
        {
            std::cerr << "Error: Unable to open file " << filename << " for reading." << std::endl;
            return false;
        }
        input_file.read(reinterpret_cast<char *>(data.data()), file_size);
        return input_file.good();
    }

    bool read_parallel(const uint32_t num_threads)
    {
        std::vector<std::thread> threads(num_threads);
        const uint64_t chunk_size = file_size / num_threads;

        for (uint32_t i = 0; i < num_threads; i++)
        {
            threads[i] = std::thread([this, i, chunk_size, num_threads]()
                                     {
                const uint64_t offset = i * chunk_size;
                const uint64_t size = (i == (num_threads - uint32_t{1})) ? file_size - offset : chunk_size;
                std::ifstream input_file(filename, std::ios::binary);
                if (!input_file)
                {
                    std::cerr << "Error: Unable to open file " << filename << " for reading." << std::endl;
                    return;
                }
                input_file.seekg(offset);
                input_file.read(reinterpret_cast<char *>(data.data() + offset), size); });
        }

        for (auto &thread : threads)
        {
            thread.join();
        }

        return true;
    }
    void generate_random_data()
    {
        data.resize(file_size);
        reference_data.resize(file_size);
        for (uint32_t i = 0; i < file_size; i++)
        {
            uint32_t value = i % 128;
            data[i] = static_cast<std::byte>(value);
            reference_data[i] = data[i];
        }
    }
    void clear()
    {
        std::fill(data.begin(), data.end(), std::byte{0x00});
    }
    void display() const
    {
        for (const auto b : data)
        {
            std::cout << static_cast<char>(b) << " ";
        }
        std::cout << std::endl;
    }
    bool state_match_reference() const
    {
        auto match_entries = std::transform_reduce(data.begin(), data.end(), reference_data.begin(), uint64_t{0}, std::plus<uint64_t>(), [](auto a, auto b)
                                                   { return a == b; });
        return match_entries == data.size();
    }
    std::string filename;
    uint64_t file_size;
    std::vector<std::byte> data;
    std::vector<std::byte> reference_data;
};

int main()
{
    const uint64_t size{1ull << 31};
    file test_file("test.bin", size);

    test_file.generate_random_data();

    const uint64_t num_experiments = 10;
    for (uint32_t i = 0; i < num_experiments; i++)
    {
        std::cout << "------------ TEST: " << i << " ------------ " << std::endl;
        cpu_timer write_timer;
        write_timer.start();
        test_file.write();
        write_timer.stop();
        double write_seconds = write_timer.elapsed_seconds();
        std::cout << double{size} / write_seconds / double{1 << 30} << " GB/s [Write]" << std::endl;

        cpu_timer read_timer;
        test_file.clear();
        read_timer.start();
        test_file.read();
        read_timer.stop();
        double read_seconds = read_timer.elapsed_seconds();
        if (!test_file.state_match_reference())
        {
            std::cout << "Validation failed" << std::endl;
            return 0;
        }
        std::cout << double{size} / read_seconds / double{1 << 30} << " GB/s [Read]" << std::endl;
        const uint32_t num_threads = 8;
        for (uint32_t threads = 2; threads <= num_threads; threads++)
        {
            cpu_timer par_read_timer;
            test_file.clear();
            par_read_timer.start();
            test_file.read_parallel(threads);
            par_read_timer.stop();
            double par_read_seconds = par_read_timer.elapsed_seconds();
            if (!test_file.state_match_reference())
            {
                std::cout << "Validation failed" << std::endl;
                return 0;
            }
            std::cout << double{size} / par_read_seconds / double{1 << 30} << " GB/s [Read] " << threads << " threads" << std::endl;
        }
    }

    return 0;
}
