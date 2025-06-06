#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma pack(push, 1)
typedef struct
{
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct
{
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

typedef struct
{
    unsigned char b, g, r;
} PIXEL;

typedef struct
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    PIXEL *pixels;
} BMP_IMAGE;

/**
 * @brief Загружает BMP-изображение из файла.
 *
 * Открывает файл с изображением, читает заголовки и пиксели,
 * и возвращает указатель на структуру BMP_IMAGE с загруженными данными.
 *
 * @param filename Имя файла BMP для загрузки.
 * @return Указатель на структуру BMP_IMAGE с данными изображения, или NULL при ошибке.
 */
BMP_IMAGE *load_Bmp(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }

    BMP_IMAGE *img = malloc(sizeof(BMP_IMAGE));
    if (!img)
    {
        fclose(file);
        return NULL;
    }

    fread(&img->fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fread(&img->infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    if (img->fileHeader.bfType != 0x4D42 || img->infoHeader.biBitCount != 24)
    {
        printf("Error: Only 24-bit BMP files are supported\n");
        free(img);
        fclose(file);
        return NULL;
    }

    int imageSize = img->infoHeader.biWidth * img->infoHeader.biHeight;
    img->pixels = malloc(imageSize * sizeof(PIXEL));

    fseek(file, img->fileHeader.bfOffBits, SEEK_SET);

    int padding = (4 - (img->infoHeader.biWidth * 3) % 4) % 4;
    for (int y = 0; y < img->infoHeader.biHeight; y++)
    {
        fread(&img->pixels[y * img->infoHeader.biWidth], sizeof(PIXEL), img->infoHeader.biWidth, file);
        fseek(file, padding, SEEK_CUR);
    }

    fclose(file);
    return img;
}

/**
 * @brief Получает значение определенного бита в байте.
 *
 * Возвращает 0 или 1 в зависимости от значения бита по указанной позиции.
 *
 * @param byte Исходный байт.
 * @param bit Позиция бита (от 0 до 7).
 * @return Значение бита (0 или 1).
 */
int getBit(unsigned char byte, int bit)
{
    return (byte >> bit) & 1;
}

/**
 * @brief Извлекает скрытое сообщение из изображения, начиная с заданных координат.
 *
 * Проходит по пикселям изображения, извлекая младшие биты цветовых каналов,
 * и собирает символы сообщения до тех пор, пока не встретит нулевой байт или не достигнет длины messageLen.
 *
 * @param img Указатель на структуру BMP_IMAGE с загруженным изображением.
 * @param startX Координата X начальной точки извлечения сообщения.
 * @param startY Координата Y начальной точки извлечения сообщения.
 * @param messageLen Длина сообщения в символах (в байтах).
 * @return Указатель на строку с извлечённым сообщением. Необходимо освободить память после использования.
 */
char *extract_Message(BMP_IMAGE *img, int startX, int startY, int messageLen)
{
    char *message = malloc(messageLen + 1);
    int startIndex = startY * img->infoHeader.biWidth + startX;
    int bitIndex = 0;

    for (int i = 0; i < messageLen + 1; i++)
    {
        char ch = 0;

        for (int bit = 0; bit < 8; bit++)
        {
            int pixelIndex = startIndex + (bitIndex / 3);
            int colorChannel = bitIndex % 3;

            unsigned char color;
            switch (colorChannel)
            {
            case 0:
                color = img->pixels[pixelIndex].r;
                break;
            case 1:
                color = img->pixels[pixelIndex].g;
                break;
            case 2:
                color = img->pixels[pixelIndex].b;
                break;
            }

            if (getBit(color, 0))
            {
                ch |= (1 << bit);
            }
            bitIndex++;
        }

        message[i] = ch;
        if (ch == '\0')
            break;
    }

    return message;
}

/**
 * @brief Загружает координаты начала скрытого сообщения и его длину из файла "color_key".
 *
 * Читает из файла три целых числа: X, Y и длину сообщения.
 *
 * @param x Указатель на переменную для хранения координаты X.
 * @param y Указатель на переменную для хранения координаты Y.
 * @param messageLen Указатель на переменную для хранения длины сообщения.
 * @return 1 при успешном чтении, 0 при ошибке.
 */
int load_color_key(int *x, int *y, int *messageLen)
{
    FILE *file = fopen("color_key", "r");
    if (!file)
    {
        printf("Error: Cannot open color_key file\n");
        return 0;
    }

    if (fscanf(file, "%d %d %d", x, y, messageLen) != 3)
    {
        printf("Error: Invalid color_key file format\n");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

/**
 * @brief Декодирует скрытое сообщение из BMP-изображения с использованием сохраненного ключа.
 *
 * Эта функция загружает координаты начала сообщения и его длину из файла "color_key",
 * запрашивает у пользователя имя файла BMP, загружает изображение,
 * извлекает скрытое сообщение и выводит его на экран.
 *
 * @return Возвращает 0 при успешном выполнении, или 1 при возникновении ошибок.
 */
int color_dec()
{
    char filename[256];
    int startX, startY, messageLen;

    // Загружаем координаты и длину сообщения из файла "color_key"
    if (!load_color_key(&startX, &startY, &messageLen))
    {
        return 1; // Ошибка при загрузке ключа
    }

    printf("\nBMP Image Text Decryption\n");
    printf("=========================\n\n");

    printf("Enter BMP filename to decode: ");
    scanf("%s", filename);

    printf("Image loaded successfully!\n");

    BMP_IMAGE *img = load_Bmp(filename);
    if (!img)
        return 1; // Ошибка при загрузке изображения

    printf("\n==================\n");
    printf("Decrypted message:\n\n");

    char *message = extract_Message(img, startX, startY, messageLen);
    if (!message)
    {
        free(img->pixels);
        free(img);
        return 1; // Ошибка при извлечении сообщения
    }

    printf("%s\n", message);

    free(message);
    free(img->pixels);
    free(img);

    return 0; // Успешное завершение
}
