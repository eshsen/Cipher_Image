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
 * Эта функция открывает указанный BMP-файл, считывает заголовки файла и изображения,
 * а также загружает пиксели изображения в динамически выделенную память.
 *
 * @param filename Путь к файлу BMP, который необходимо загрузить.
 * @return Указатель на структуру BMP_IMAGE, содержащую загруженное изображение,
 *         или NULL в случае ошибки.
 */
BMP_IMAGE *load__bmp(const char *filename)
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
 * @brief Сохраняет BMP-изображение в файл.
 *
 * Эта функция создает новый BMP-файл и записывает в него заголовки и пиксели,
 * представляющие изображение.
 *
 * @param filename Путь к файлу, в который необходимо сохранить изображение.
 * @param img Указатель на структуру BMP_IMAGE, содержащую изображение для сохранения.
 * @return 1 в случае успешного сохранения, 0 в случае ошибки.
 */
int save__bmp(const char *filename, BMP_IMAGE *img)
{
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        printf("Error: Cannot create file %s\n", filename);
        return 0;
    }

    fwrite(&img->fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&img->infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    int padding = (4 - (img->infoHeader.biWidth * 3) % 4) % 4;
    unsigned char paddingBytes[3] = {0, 0, 0};

    for (int y = 0; y < img->infoHeader.biHeight; y++)
    {
        fwrite(&img->pixels[y * img->infoHeader.biWidth], sizeof(PIXEL), img->infoHeader.biWidth, file);
        if (padding > 0)
        {
            fwrite(paddingBytes, 1, padding, file);
        }
    }

    fclose(file);
    return 1;
}

/**
 * @brief Устанавливает значение бита в байте.
 *
 * Эта функция устанавливает указанный бит в байте в заданное значение (0 или 1).
 *
 * @param byte Указатель на байт, в котором нужно установить бит.
 * @param bit Индекс бита, который нужно установить (0 - младший бит).
 * @param value Значение, которым нужно установить бит (0 или 1).
 */
void set__bit(unsigned char *byte, int bit, int value)
{
    if (value)
    {
        *byte |= (1 << bit);
    }
    else
    {
        *byte &= ~(1 << bit);
    }
}

/**
 * @brief Получает значение бита из байта.
 *
 * Эта функция возвращает значение указанного бита в байте.
 *
 * @param byte Байтовое значение, из которого нужно получить бит.
 * @param bit Индекс бита, который нужно получить (0 - младший бит).
 * @return Значение бита (0 или 1).
 */
int get__bit(unsigned char byte, int bit)
{
    return (byte >> bit) & 1;
}

/**
 * @brief Скрывает сообщение в изображении BMP.
 *
 * Эта функция встраивает заданное сообщение в пиксели изображения, изменяя
 * младший бит каждого цветового канала пикселей для кодирования символов
 * сообщения. Сообщение представляет собой строку символов и заканчивается
 * нулевым символом.
 *
 * @param img Указатель на структуру BMP_IMAGE, в которую будет встроено сообщение.
 * @param message Указатель на строку символов, содержащую сообщение для скрытия.
 * @param startX Координата X начала встраивания сообщения в изображение.
 * @param startY Координата Y начала встраивания сообщения в изображение.
 *
 * @note Если сообщение слишком длинное для изображения, начиная с указанной позиции,
 * функция выведет сообщение об ошибке и завершит выполнение.
 */
void hideMessage(BMP_IMAGE *img, const char *message, int startX, int startY)
{
    int messageLen = strlen(message);
    int totalBits = (messageLen + 1) * 8; // +1 for null terminator
    int imageSize = img->infoHeader.biWidth * img->infoHeader.biHeight;
    int startIndex = startY * img->infoHeader.biWidth + startX;

    if (startIndex + (totalBits / 3) >= imageSize)
    {
        printf("Error: Message too long for image starting at this position\n");
        return;
    }

    int bitIndex = 0;
    for (int i = 0; i < messageLen + 1; i++)
    {
        char ch = (i < messageLen) ? message[i] : '\0';

        for (int bit = 0; bit < 8; bit++)
        {
            int pixelIndex = startIndex + (bitIndex / 3);
            int colorChannel = bitIndex % 3;

            unsigned char *color;
            switch (colorChannel)
            {
            case 0:
                color = &img->pixels[pixelIndex].r;
                break;
            case 1:
                color = &img->pixels[pixelIndex].g;
                break;
            case 2:
                color = &img->pixels[pixelIndex].b;
                break;
            }

            set__bit(color, 0, (ch >> bit) & 1);
            bitIndex++;
        }
    }
}

/**
 * @brief Сохраняет координаты и длину скрытого сообщения в файл "color_key".
 *
 * Эта функция записывает информацию о позиции начала скрытого сообщения и его длине,
 * чтобы позже можно было восстановить сообщение из изображения.
 *
 * @param x Координата X начальной точки скрытия сообщения.
 * @param y Координата Y начальной точки скрытия сообщения.
 * @param messageLen Длина скрытого сообщения в символах.
 */
void saveColorKey(int x, int y, int messageLen)
{
    FILE *file = fopen("color_key", "w");
    if (!file)
    {
        printf("Error: Cannot create color_key file\n");
        return;
    }

    fprintf(file, "%d %d %d\n", x, y, messageLen);
    fclose(file);
}

/**
 * @brief Выполняет процесс шифрования сообщения в BMP-изображение с сохранением ключа.
 *
 * Эта функция запрашивает у пользователя исходное BMP-изображение, сообщение для скрытия,
 * генерирует случайную стартовую позицию для вставки сообщения, шифрует сообщение,
 * сохраняет ключ (координаты и длину сообщения) в файл "color_key",
 * сохраняет полученное изображение с внедренным сообщением в указанный файл.
 *
 * @return Возвращает 0 при успешном выполнении, или 1 при возникновении ошибок.
 */
int color()
{
    char filename[256], message[1000], outputFileName[256];

    printf("\nBMP Image Text Encryption\n");
    printf("=========================\n\n");

    printf("Enter BMP filename: ");
    scanf("%s", filename);

    BMP_IMAGE *img = load__bmp(filename);
    if (!img)
        return 1; // Ошибка при загрузке изображения

    printf("\nImage loaded successfully!\n");
    printf("Enter message to hide: ");
    getchar(); // consume leftover newline
    fgets(message, sizeof(message), stdin);
    message[strcspn(message, "\n")] = '\0'; // удаление символа новой строки

    srand(time(NULL));
    int maxX = img->infoHeader.biWidth - 1;
    int maxY = img->infoHeader.biHeight - 1;
    int startX = rand() % maxX;
    int startY = rand() % maxY;

    int messageLen = strlen(message);
    int requiredPixels = ((messageLen + 1) * 8 + 2) / 3;

    // Проверка, чтобы сообщение поместилось в изображение
    if (startY * img->infoHeader.biWidth + startX + requiredPixels >=
        img->infoHeader.biWidth * img->infoHeader.biHeight)
    {
        startX = 0;
        startY = 0;
    }

    hideMessage(img, message, startX, startY);
    saveColorKey(startX, startY, messageLen);

    printf("Enter output filename: ");
    scanf("%s", outputFileName);

    if (save__bmp(outputFileName, img))
    {
        printf("\nImage saved as %s\n", outputFileName);
        printf("Key information saved to 'color_key' file.\n");
        free(img->pixels);
        free(img);
        return 0; // Успешное завершение
    }
    else
    {
        printf("Failed to save the image.\n");
        free(img->pixels);
        free(img);
        return 1; // Ошибка при сохранении файла
    }
}
