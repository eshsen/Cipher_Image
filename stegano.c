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
 * Загружает BMP изображение из файла.
 * @param filename Имя файла BMP.
 * @param header Указатель на структуру BMPHeader, куда будет сохранена информация о заголовке.
 * @return Указатель на массив данных изображения в памяти или NULL при ошибке.
 */
unsigned char *load_bmp(const char *filename, BMPHeader *header)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Failed to open file %s\n", filename);
        return NULL;
    }

    fread(header, sizeof(BMPHeader), 1, f);

    if (header->bfType != 0x4D42)
    { // 'BM' в little-endian
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
 * Сохраняет BMP изображение в файл.
 * @param filename Имя файла для сохранения BMP.
 * @param header Указатель на структуру BMPHeader с информацией о изображении.
 * @param data Данные изображения для записи.
 * @return 1 при успехе, 0 при ошибке.
 */
int save_bmp(const char *filename, BMPHeader *header, unsigned char *data)
{
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        printf("Failed to open file for writing %s\n", filename);
        return 0;
    }

    fwrite(header, sizeof(BMPHeader), 1, f);

    int width = header->biWidth;
    int height = header->biHeight;

    int row_padded = (width * 3 + 3) & (~3);

    fwrite(data, 1, row_padded * height, f);

    fclose(f);
    return 1;
}

/**
 * Получает общее количество пикселей изображения.
 * @param header Указатель на структуру BMPHeader.
 * @return Общее число пикселей.
 */
int get_pixel_count(BMPHeader *header)
{
    return header->biWidth * header->biHeight;
}

/**
 * Получает значение определенного бита из символа.
 * @param c Символ-источник.
 * @param pos Позиция бита (от 0 до 7).
 * @return Значение бита (0 или 1).
 */
int get_bit(char c, int pos)
{
    return (c >> pos) & 1;
}

/**
 * Устанавливает бит в компоненте пикселя (использует только последний бит компоненты R).
 * @param pixel Указатель на байт компоненты пикселя (R).
 * @param bit Значение бита для установки (0 или 1).
 */
void set_bit_in_pixel(unsigned char *pixel, int bit)
{
    pixel[2] = (pixel[2] & 0xFE) | bit;
}

/**
 * Основная функция стеганографической вставки текста в BMP изображение.
 * Взаимодействует с пользователем для выбора файлов и параметров шифрования.
 * Не возвращает значения. Выполняет операции по внедрению сообщения и сохранению результата.
 * Включает создание файла ключа с параметрами шифрования.
 */
int stegano()
{
    printf("\nBMP Image Text Encryption\n");
    printf("=========================\n\n");

    char input_filename[256], output_filename[256], key_filename[] = "stegano_key";

    printf("Enter the input BMP filename: ");
    scanf("%255s", input_filename);

    BMPHeader header;

    unsigned char *data = load_bmp(input_filename, &header);
    if (!data)
    {
        return 1;
    }

    int width = header.biWidth;
    int height = header.biHeight;

    int pixel_count = get_pixel_count(&header);
    size_t max_message_size = pixel_count / 8;

    printf("\nImage loaded successfully!\n");
    printf("Maximum message size: %zu bytes\n\n", max_message_size);

    while (getchar() != '\n')
        ;
    char message[1024];

    printf("Enter a message (max %zu symbols): ", max_message_size);
    fgets(message, max_message_size + 1, stdin);

    size_t msg_len = strlen(message);
    if (msg_len > 0 && message[msg_len - 1] == '\n')
    {
        message[msg_len - 1] = '\0';
        msg_len--;
    }

    if (msg_len > max_message_size)
    {
        printf("The message is too long!\n");
        free(data);
        return 1;
    }

    int step;
    printf("Enter the step to advance through the bitmap: ");
    scanf("%d", &step);

    size_t total_bits = msg_len * 8;

    if ((total_bits - 1) / step >= pixel_count)
    {
        printf("The message is too large for the given image and step.\n");
        free(data);
        return 1;
    }

    size_t message_byte_index = 0;
    int bit_in_char = 0;

    size_t pixel_index = 0;

    for (size_t i = 0; i < total_bits; i++)
    {
        if (pixel_index >= pixel_count)
        {
            break;
        }
        unsigned char *pixel = &data[pixel_index * 3]; // B G R

        int bit_to_embed = get_bit(message[message_byte_index], bit_in_char);

        set_bit_in_pixel(pixel, bit_to_embed);

        bit_in_char++;
        if (bit_in_char == 8)
        {
            bit_in_char = 0;
            message_byte_index++;
        }

        pixel_index += step;

        if (pixel_index >= pixel_count && i != total_bits - 1)
        {
            printf("There is not enough space for the entire message.\n");
            free(data);
            return 1;
        }
    }

    printf("Enter the output BMP filename: ");
    scanf("%255s", output_filename);

    if (!save_bmp(output_filename, &header, data))
    {
        printf("Failed to save image.\n");
        free(data);
        return 1;
    }

    printf("\nImage saved as %s\n", output_filename);

    free(data);

    FILE *keyfile = fopen(key_filename, "w");
    if (!keyfile)
    {
        printf("Failed to open key file '%s' for writing.\n", key_filename);
        return 1;
    }
    fprintf(keyfile, "STEP: %d\nLENGTH: %zu\n", step, msg_len);
    fclose(keyfile);
    printf("The key has been saved in the file '%s'.\n", key_filename);

    return 0;
}
