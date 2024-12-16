#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <time.h>

#define DEFAULT_NUM_READERS 5
#define DEFAULT_NUM_WRITERS 2
#define DEFAULT_DB_SIZE 10

// Глобальные переменные
int* database;
int db_size;
omp_lock_t db_lock; // OpenMP блок для синхронизации
int reader_count = 0;
omp_lock_t reader_count_lock;

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

// Функция читателя
void reader(int id) {
    while (1) {
        // Увеличиваем счетчик читателей
        omp_set_lock(&reader_count_lock);
        reader_count++;
        if (reader_count == 1) {
            omp_set_lock(&db_lock); // Первый читатель блокирует доступ писателям
        }
        omp_unset_lock(&reader_count_lock);

        // Чтение из базы данных
        int index = rand() % db_size;
        printf("[Reader %d] Read index %d, value %d\n", id, index, database[index]);

        // Уменьшаем счетчик читателей
        omp_set_lock(&reader_count_lock);
        reader_count--;
        if (reader_count == 0) {
            omp_unset_lock(&db_lock); // Последний читатель освобождает доступ
        }
        omp_unset_lock(&reader_count_lock);

        usleep(rand() % 1000000); // Задержка
    }
}

// Функция писателя
void writer(int id) {
    while (1) {
        omp_set_lock(&db_lock); // Писатель блокирует доступ

        // Запись в базу данных
        int index = rand() % db_size;
        int old_value = database[index];
        int new_value = rand() % 100;
        database[index] = new_value;
        sort_database();
        printf("[Writer %d] Wrote index %d, old value %d, new value %d\n", id, index, old_value, new_value);

        omp_unset_lock(&db_lock); // Освобождает доступ

        usleep(rand() % 2000000); // Задержка
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    // Параметры
    int num_readers = DEFAULT_NUM_READERS;
    int num_writers = DEFAULT_NUM_WRITERS;
    db_size = DEFAULT_DB_SIZE;

    // Инициализация базы данных
    init_database();

    // Инициализация OpenMP блоков
    omp_init_lock(&db_lock);
    omp_init_lock(&reader_count_lock);

    // Создание читателей и писателей с OpenMP
    #pragma omp parallel num_threads(num_readers + num_writers)
    {
        int thread_id = omp_get_thread_num();
        if (thread_id < num_readers) {
            reader(thread_id + 1);
        } else {
            writer(thread_id - num_readers + 1);
        }
    }

    // Удаление блокировок и очистка памяти
    omp_destroy_lock(&db_lock);
    omp_destroy_lock(&reader_count_lock);
    free(database);

    return 0;
}
