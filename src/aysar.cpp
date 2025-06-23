#include <iostream>
#include <cstdlib>
#include <omp.h>
#include <sys/time.h>

using namespace std;

void blur_parallel(int** image, int** output, int width, int height, int num_threads) {
    #pragma omp parallel num_threads(num_threads)
    {
        int i_start, i_end;
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();
        int rows_per_thread = height / nthreads;

        i_start = tid * rows_per_thread;
        i_end = (tid == nthreads - 1) ? height - 1 : (i_start + rows_per_thread);

        for (int i = max(1, i_start); i < min(i_end, height - 1); i++) {
            for (int j = 1; j < width - 1; j++) {
                int sum = 0;
                for (int di = -1; di <= 1; di++) {
                    for (int dj = -1; dj <= 1; dj++) {
                        sum += image[i + di][j + dj];
                    }
                }
                output[i][j] = sum / 9;
            }
        }
    }
}

void blur_sequential(int** image, int** output, int width, int height) {
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int sum = 0;
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    sum += image[i + di][j + dj];
                }
            }
            output[i][j] = sum / 9;
        }
    }
}

int** allocate_2d(int height, int width) {
    int** arr = (int**) malloc(height * sizeof(int*));
    if (arr == NULL) {
        cout << "فشل في تخصيص الصفوف!" << endl;
        exit(1);
    }
    for (int i = 0; i < height; i++) {
        arr[i] = (int*) malloc(width * sizeof(int));
        if (arr[i] == NULL) {
            cout << "فشل في تخصيص الأعمدة للصف " << i << endl;
            exit(1);
        }
    }
    return arr;
}

void free_2d(int** arr, int height) {
    for (int i = 0; i < height; i++) free(arr[i]);
    free(arr);
}

void init_image(int** image, int width, int height) {
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            image[i][j] = rand() % 256;
}

int main() {
    int sizes[][2] = {{500, 500}, {1000, 1000}, {2000, 2000}};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    int thread_counts[] = {1, 2, 4, 8};
    int num_thread_counts = sizeof(thread_counts) / sizeof(thread_counts[0]);

    struct timeval start, end;
    double elapsed;

    for (int s = 0; s < num_sizes; s++) {
        int height = sizes[s][0];
        int width = sizes[s][1];

        int** image = allocate_2d(height, width);
        int** output = allocate_2d(height, width);

        init_image(image, width, height);

        cout << "\n====== اختبار صورة بحجم " << height << " x " << width << " ======\n";

        double sequential_time = 0.0;

        for (int t = 0; t < num_thread_counts; t++) {
            int NUM_THREADS = thread_counts[t];

            if (NUM_THREADS == 1) {
                gettimeofday(&start, NULL);
                blur_sequential(image, output, width, height);
                gettimeofday(&end, NULL);

                elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
                sequential_time = elapsed;

                cout << "تسلسلي (1 thread): " << elapsed << " ثانية\n";
            } else {
                for (int i = 0; i < height; i++)
                    for (int j = 0; j < width; j++)
                        output[i][j] = 0;

                gettimeofday(&start, NULL);
                blur_parallel(image, output, width, height, NUM_THREADS);
                gettimeofday(&end, NULL);

                elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
                double speedup = sequential_time / elapsed;

                cout << "متوازي (" << NUM_THREADS << " threads): " << elapsed << " ثانية\n";
                cout << "⏩ Speedup (" << NUM_THREADS << " threads): " << speedup << "x\n";
            }
        }

        free_2d(image, height);
        free_2d(output, height);
    }

    return 0;
}
