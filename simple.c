#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct
{
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPHeader;

typedef struct
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
} BMPInfoHeader;
#pragma pack(pop)

/**
 * load - Загружает BMP изображение из файла.
 * @param filename: Имя файла изображения.
 * @param imageData: Указатель на указатель, куда будет сохранена выделенная память под данные изображения.
 * @param header: Указатель на структуру BMPHeader для хранения заголовка файла.
 * @param infoHeader: Указатель на структуру BMPInfoHeader для хранения информации о изображении.
 *
 * Возвращает размер изображения в байтах при успешной загрузке, 0 при ошибке.
 */
int load(const char *filename, unsigned char **imageData, BMPHeader *header, BMPInfoHeader *infoHeader)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error: Cannot open image file %s\n", filename);
        return 0;
    }

    fread(header, sizeof(BMPHeader), 1, file);
    fread(infoHeader, sizeof(BMPInfoHeader), 1, file);

    if (header->type != 0x4D42 || infoHeader->bitCount != 24)
    {
        printf("Error: Only 24-bit BMP files are supported\n");
        fclose(file);
        return 0;
    }

    fseek(file, header->offset, SEEK_SET);
    int imageSize = infoHeader->width * abs(infoHeader->height) * 3;
    *imageData = (unsigned char *)malloc(imageSize);
    fread(*imageData, 1, imageSize, file);

    fclose(file);

    return imageSize;
}

/**
 * saveBMP - Сохраняет изображение в файл BMP.
 * @param filename: Имя файла для сохранения изображения.
 * @param imageData: Указатель на данные изображения.
 * @param header: Указатель на структуру BMPHeader с заголовком файла.
 * @param infoHeader: Указатель на структуру BMPInfoHeader с информацией об изображении.
 *
 * Возвращает 1 при успешном сохранении, 0 при ошибке.
 */
int saveBMP(const char *filename, unsigned char *imageData, BMPHeader *header, BMPInfoHeader *infoHeader)
{
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        printf("Error: Cannot create output file %s\n", filename);
        return 0;
    }
    fwrite(header, sizeof(BMPHeader), 1, file);
    fwrite(infoHeader, sizeof(BMPInfoHeader), 1, file);

    fseek(file, header->offset, SEEK_SET);
    int imageSize = infoHeader->width * abs(infoHeader->height) * 3;
    fwrite(imageData, 1, imageSize, file);
    fclose(file);
    return 1;
}

/**
 * encryptText - Встраивает текст в изображение с помощью метода LSB (младших бит).
 * @param imageData: Массив байтов данных изображения, в который будет встроен текст.
 * @param text: Строка текста для скрытия.
 * @param imageSize: Размер данных изображения в байтах.
 */
void encryptText(unsigned char *imageData, const char *text, int imageSize)
{
    int textLen = strlen(text);
    int bitIndex = 0;

    for (int i = 0; i < 32; i++)
    {
        if (bitIndex >= imageSize)
            break;
        imageData[bitIndex] = (imageData[bitIndex] & 0xFE) | ((textLen >> (31 - i)) & 1);
        bitIndex++;
    }

    for (int i = 0; i < textLen; i++)
    {
        for (int j = 7; j >= 0; j--)
        {
            if (bitIndex >= imageSize)
                return;
            imageData[bitIndex] = (imageData[bitIndex] & 0xFE) | ((text[i] >> j) & 1);
            bitIndex++;
        }
    }
}

/**
 * simple - Основная функция для шифрования текста в BMP изображение и сохранения ключа.
 *
 * Выполняет:
 * - Запрос пути к изображению у пользователя
 * - Загрузку изображения
 * - Ввод текста для скрытия
 * - Встраивание текста в изображение
 * - Сохранение измененного изображения
 * - Создание файла ключа с информацией о длине текста и размере изображения
 *
 * Возвращает 0 при успешной работе или 1 при ошибках.
 */
int simple()
{
    FILE *file, *keyFile, *outputFile;
    BMPHeader header;
    BMPInfoHeader infoHeader;
    unsigned char *imageData;
    char filename[256], outputFilename[256], text[1000];

    printf("\nBMP Image Text Encryption\n");
    printf("=========================\n\n");

    printf("Enter BMP filename: ");
    scanf("%s", filename);

    file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error: Cannot open file %s\n", filename);
        return 1;
    }

    fread(&header, sizeof(BMPHeader), 1, file);
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, file);

    if (header.type != 0x4D42) // 'BM'
    {
        printf("Error: Not a valid BMP file\n");
        fclose(file);
        return 1;
    }

    if (infoHeader.bitCount != 24)
    {
        printf("Error: Only 24-bit BMP files are supported\n");
        fclose(file);
        return 1;
    }

    int imageSize = load(filename, &imageData, &header, &infoHeader);

    if (!imageSize)
        return 1;

    int maxChars = (imageSize - 32) / 8;
    printf("\nImage loaded successfully!\n");
    printf("Maximum characters that can be hidden: %d\n\n", maxChars);
    printf("Enter text to encrypt (max %d characters): ", maxChars);

    scanf(" %[^\n]", text);

    if (strlen(text) > maxChars)
    {
        printf("Error: Text too long! Maximum %d characters allowed.\n", maxChars);
        free(imageData);
        return 1;
    }

    printf("Enter output filename: ");
    scanf("%s", outputFilename);

    outputFile = fopen(outputFilename, "wb");

    encryptText(imageData, text, imageSize);

    if (saveBMP(outputFilename, imageData, &header, &infoHeader))
    {
        keyFile = fopen("simple_key", "w");
        if (keyFile)
        {
            fprintf(keyFile, "TEXT_LENGTH: %d\n", (int)strlen(text));
            fprintf(keyFile, "IMAGE_SIZE: %d\n", imageSize);
            fclose(keyFile);
            printf("\nImage saved as %s\n", outputFilename);
            printf("Key information saved to 'simple_key' file.\n");
        }
        else
        {
            printf("Warning: Could not save key file!\n");
        }
    }

    free(imageData);

    return 0;
}
