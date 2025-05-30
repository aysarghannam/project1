#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <sys/time.h>

using namespace std;

typedef struct {
    int start_row;
    int end_row;
    int width;
    int height;
    int** image;
    int** output;
} thread_data_t;

void* blur_thread(void* arg) {
    thread_data_t* data = (thread_data_t*) arg;
    int start = data->start_row;
    int end = data->end_row;
    int width = data->width;
    int height = data->height;
    int** image = data->image;
    int** output = data->output;

    for (int i = start; i < end; i++) {
        if (i == 0 || i == height - 1) continue;
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
    pthread_exit(NULL);
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

                pthread_t* threads = (pthread_t*) malloc(NUM_THREADS * sizeof(pthread_t));
                thread_data_t* thread_data = (thread_data_t*) malloc(NUM_THREADS * sizeof(thread_data_t));

                int rows_per_thread = height / NUM_THREADS;

                gettimeofday(&start, NULL);

                for (int i = 0; i < NUM_THREADS; i++) {
                    thread_data[i].start_row = i * rows_per_thread;
                    thread_data[i].end_row = (i == NUM_THREADS - 1) ? height : (i + 1) * rows_per_thread;
                    thread_data[i].width = width;
                    thread_data[i].height = height;
                    thread_data[i].image = image;
                    thread_data[i].output = output;

                    pthread_create(&threads[i], NULL, blur_thread, (void*) &thread_data[i]);
                }

                for (int i = 0; i < NUM_THREADS; i++) {
                    pthread_join(threads[i], NULL);
                }

                gettimeofday(&end, NULL);

                elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
                double speedup = sequential_time / elapsed;

                cout << "متوازي (" << NUM_THREADS << " threads): " << elapsed << " ثانية\n";
                cout << "⏩ Speedup (" << NUM_THREADS << " threads): " << speedup << "x\n";

                free(threads);
                free(thread_data);
            }
        }

        free_2d(image, height);
        free_2d(output, height);
    }

    return 0;
}
