#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#define DEFAULT_NUM_READERS 5
#define DEFAULT_NUM_WRITERS 2
#define DEFAULT_DB_SIZE 10

// Глобальные переменные
int* database;
int db_size;
int reader_count = 0;
int num_readers;
int num_writers;
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER; // Мьютекс для доступа к базе данных
pthread_mutex_t reader_count_mutex = PTHREAD_MUTEX_INITIALIZER; // Мьютекс для счетчика читателей

FILE* log_file;

// Функция для инициализации базы данных
void init_database() {
    database = (int*)malloc(db_size * sizeof(int));
    for (int i = 0; i < db_size; i++) {
        database[i] = i + 1;
    }
}

// Функция для сортировки базы данных
void sort_database() {
    for (int i = 0; i < db_size - 1; i++) {
        for (int j = 0; j < db_size - i - 1; j++) {
            if (database[j] > database[j + 1]) {
                int temp = database[j];
                database[j] = database[j + 1];
                database[j + 1] = temp;
            }
        }
    }
}

// Логирование событий
void log_event(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    vfprintf(stdout, format, args);
    va_end(args);
}

// Функция читателя
void* reader(void* arg) {
    int id = *(int*)arg;
    free(arg);

    while (1) {
        pthread_mutex_lock(&reader_count_mutex);
        reader_count++;
        if (reader_count == 1) {
            pthread_mutex_lock(&db_mutex); // Первый читатель блокирует доступ писателям
        }
        pthread_mutex_unlock(&reader_count_mutex);

        // Чтение из базы данных
        int index = rand() % db_size;
        log_event("[Reader %d] Read index %d, value %d\n", id, index, database[index]);

        pthread_mutex_lock(&reader_count_mutex);
        reader_count--;
        if (reader_count == 0) {
            pthread_mutex_unlock(&db_mutex); // Последний читатель освобождает доступ
        }
        pthread_mutex_unlock(&reader_count_mutex);

        usleep(rand() % 1000000); // Задержка
    }

    return NULL;
}

// Функция писателя
void* writer(void* arg) {
    int id = *(int*)arg;
    free(arg);

    while (1) {
        pthread_mutex_lock(&db_mutex); // Писатель блокирует доступ

        // Запись в базу данных
        int index = rand() % db_size;
        int old_value = database[index];
        int new_value = rand() % 100;
        database[index] = new_value;
        sort_database();
        log_event("[Writer %d] Wrote index %d, old value %d, new value %d\n", id, index, old_value, new_value);

        pthread_mutex_unlock(&db_mutex); // Освобождает доступ

        usleep(rand() % 2000000); // Задержка
    }

    return NULL;
}

void usage(const char* prog_name) {
    printf("Usage: %s [-r NUM_READERS] [-w NUM_WRITERS] [-d DB_SIZE] [-o OUTPUT_FILE]\n", prog_name);
    exit(1);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    // Параметры по умолчанию
    num_readers = DEFAULT_NUM_READERS;
    num_writers = DEFAULT_NUM_WRITERS;
    db_size = DEFAULT_DB_SIZE;
    log_file = stdout;

    // Парсинг аргументов командной строки
    int opt;
    while ((opt = getopt(argc, argv, "r:w:d:o:")) != -1) {
        switch (opt) {
        case 'r':
            num_readers = atoi(optarg);
            break;
        case 'w':
            num_writers = atoi(optarg);
            break;
        case 'd':
            db_size = atoi(optarg);
            break;
        case 'o':
            log_file = fopen(optarg, "w");
            if (!log_file) {
                perror("Failed to open output file");
                return 1;
            }
            break;
        default:
            usage(argv[0]);
        }
    }

    pthread_t* readers = malloc(num_readers * sizeof(pthread_t));
    pthread_t* writers = malloc(num_writers * sizeof(pthread_t));

    init_database();

    // Создание потоков читателей
    for (int i = 0; i < num_readers; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&readers[i], NULL, reader, id) != 0) {
            perror("Failed to create reader thread");
            return 1;
        }
    }

    // Создание потоков писателей
    for (int i = 0; i < num_writers; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&writers[i], NULL, writer, id) != 0) {
            perror("Failed to create writer thread");
            return 1;
        }
    }

    // Ожидание завершения потоков (в данном случае не используется, так как программа работает бесконечно)
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    fclose(log_file);
    free(database);
    free(readers);
    free(writers);

    return 0;
}
