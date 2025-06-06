#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Структура BMP
#pragma pack(push, 1)
typedef struct
{
    unsigned short bfType;      // Тип файла ("BM")
    unsigned int bfSize;        // Размер файла в байтах
    unsigned short bfReserved1; // Зарезервировано
    unsigned short bfReserved2; // Зарезервировано
    unsigned int bfOffBits;     // Смещение до данных изображения

    unsigned int biSize;         // Размер заголовка BITMAPINFOHEADER (40)
    int biWidth;                 // Ширина изображения
    int biHeight;                // Высота изображения
    unsigned short biPlanes;     // Количество плоскостей (должно быть 1)
    unsigned short biBitCount;   // Битность (24)
    unsigned int biCompression;  // Сжатие (0 - без сжатия)
    unsigned int biSizeImage;    // Размер изображения в байтах
    int biXPelsPerMeter;         // Горизонтальное разрешение
    int biYPelsPerMeter;         // Вертикальное разрешение
    unsigned int biClrUsed;      // Количество используемых цветов
    unsigned int biClrImportant; // Важность цветов
} BMPHeader;
#pragma pack(pop)

/**
 * @brief Загружает BMP изображение из файла.
 *
 * Открывает файл, читает заголовок BMP и данные пикселей.
 * Поддерживаются только 24-битные BMP файлы без сжатия.
 *
 * @param filename Имя файла BMP.
 * @param header Указатель на структуру BMPHeader, куда будет сохранена информация о файле.
 * @return Указатель на массив данных пикселей (BGR), или NULL при ошибке.
 */
unsigned char *loadbmp(const char *filename, BMPHeader *header)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Failed to open file %s\n", filename);
        return NULL;
    }

    fread(header, sizeof(BMPHeader), 1, f);

    if (header->bfType != 0x4D42)
    {
        printf("This is not a BMP file.\n");
        fclose(f);
        return NULL;
    }

    if (header->biBitCount != 24)
    {
        printf("Only 24-bit BMPs are supported.\n");
        fclose(f);
        return NULL;
    }

    fseek(f, header->bfOffBits, SEEK_SET);

    int width = header->biWidth;
    int height = header->biHeight;

    int row_padded = (width * 3 + 3) & (~3);

    unsigned char *data = malloc(row_padded * height);
    if (!data)
    {
        printf("Failed to allocate memory.\n");
        fclose(f);
        return NULL;
    }

    fread(data, 1, row_padded * height, f);

    fclose(f);
    return data;
}

/**
 * @brief Получает значение бита из символа по позиции.
 *
 * @param c Символ-источник.
 * @param pos Позиция бита (0 - младший бит).
 * @return Значение бита (0 или 1).
 */
int getbit(char c, int pos)
{
    return (c >> pos) & 1;
}

/**
 * @brief Расшифровывает скрытое сообщение из BMP изображения по ключу.
 *
 * Загружает изображение и ключевой файл. Извлекает закодированное сообщение,
 * основываясь на параметрах шага и длины сообщения из ключа.
 *
 * @param image_filename Имя файла BMP с скрытым сообщением.
 * @param key_filename Имя файла ключа, содержащего параметры шага и длины сообщения.
 */
void decode_message(const char *image_filename, const char *key_filename)
{
    BMPHeader header;

    unsigned char *data = loadbmp(image_filename, &header);

    if (!data)
        return;

    int width = header.biWidth;
    int height = header.biHeight;

    int pixel_count = width * height;

    FILE *keyfile = fopen(key_filename, "r");
    if (!keyfile)
    {
        printf("Failed to open key file.\n");
        free(data);
        return;
    }

    int step;
    size_t msg_len;

    char line[100];
    while (fgets(line, sizeof(line), keyfile))
    {
        if (sscanf(line, "STEP: %d", &step) == 1)
            continue;
        if (sscanf(line, "LENGTH: %zu", &msg_len) == 1)
            continue;
    }
    fclose(keyfile);

    size_t total_bits = msg_len * 8;

    if ((total_bits - 1) / step >= pixel_count)
    {
        printf("The message length exceeds the capacity of the image with the given step.\n");
        free(data);
        return;
    }

    char decoded_message[msg_len + 1];
    memset(decoded_message, 0, sizeof(decoded_message));

    size_t message_byte_index = 0;
    int bit_in_char = 0;

    size_t pixel_index = 0;

    for (size_t i = 0; i < total_bits; i++)
    {
        if (pixel_index >= pixel_count)
            break;

        unsigned char *pixel = &data[pixel_index * 3]; // B G R

        int bit_extracted = pixel[2] & 1;

        decoded_message[message_byte_index] |= (bit_extracted << bit_in_char);

        bit_in_char++;
        if (bit_in_char == 8)
        {
            bit_in_char = 0;
            message_byte_index++;
        }

        pixel_index += step;

        if (pixel_index >= pixel_count && i != total_bits - 1)
        {
            printf("Reached end of image before decoding full message.\n");
            break;
        }
    }

    printf("\n==================\n");
    printf("Decrypted message:\n\n");

    decoded_message[msg_len] = '\0';

    printf("%s\n", decoded_message);

    free(data);
}

/**
 * @brief Основная функция декодирования стеганографического сообщения из BMP файла.
 *
 * Запрашивает у пользователя имя файла изображения и вызывает функцию декодирования.
 *
 * @return Код завершения программы.
 */
int stegano_dec()
{
    char image_filename[256];
    const char *key_filename = "stegano_key";

    printf("\nBMP Image Text Decryption\n");
    printf("=========================\n\n");

    printf("Enter encrypted BMP filename: ");
    scanf("%255s", image_filename);
    printf("Image loaded successfully!\n");

    decode_message(image_filename, key_filename);

    return 0;
}
