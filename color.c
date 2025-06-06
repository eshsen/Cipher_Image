#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma pack(push, 1)
typedef struct
{
    char signature[2];       // Подпись файла (должна быть "BM")
    unsigned int fileSize;   // Размер файла в байтах
    unsigned int reserved;   // Зарезервировано (обычно 0)
    unsigned int dataOffset; // Смещение данных изображения от начала файла
} BMPHeader;

typedef struct
{
    unsigned int headerSize;      // Размер этого заголовка (обычно 40 байт)
    int width;                    // Ширина изображения в пикселях
    int height;                   // Высота изображения в пикселях
    unsigned short planes;        // Количество цветовых плоскостей (должно быть 1)
    unsigned short bitsPerPixel;  // Глубина цвета (например, 24)
    unsigned int compression;     // Тип сжатия (0 — без сжатия)
    unsigned int dataSize;        // Размер данных изображения в байтах
    int xResolution;              // Горизонтальное разрешение
    int yResolution;              // Вертикальное разрешение
    unsigned int colors;          // Количество используемых цветов
    unsigned int importantColors; // Количество важных цветов
} BMPInfoHeader;
#pragma pack(pop)

/**
 * @brief Встраивает текст в изображение, начиная с указанной позиции.
 *
 * Эта функция записывает длину текста (в битах) и сам текст, побитно,
 * начиная с позиции startPos в массив imageData.
 *
 * @param imageData Указатель на массив байтов данных изображения.
 * @param dataSize Размер массива imageData в байтах.
 * @param text Строка текста для скрытия.
 * @param startPos Начальная позиция в массиве imageData для вставки.
 */
void embedText(unsigned char *imageData, int dataSize, const char *text, int startPos)
{
    int textLen = strlen(text);
    for (int i = 0; i < 32; i++)
    {
        int bit = (textLen >> i) & 1;
        if (startPos + i >= dataSize)
        {
            fprintf(stderr, "Error: Out of bounds during length embedding at index %d\n", startPos + i);
            return;
        }
        imageData[startPos + i] = (imageData[startPos + i] & 0xFE) | bit;
    }
    int currentByteIndex = startPos + 32;
    for (int i = 0; i < textLen; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            int bit = (text[i] >> j) & 1;
            if (currentByteIndex >= dataSize)
            {
                fprintf(stderr, "Error: Out of bounds during text embedding at index %d\n", currentByteIndex);
                return;
            }
            imageData[currentByteIndex] = (imageData[currentByteIndex] & 0xFE) | bit;
            currentByteIndex++;
        }
    }
}

/**
 * @brief Основная функция для шифрования текста в BMP изображение.
 *
 * Загружает BMP файл, вставляет скрытый текст и сохраняет результат,
 * а также сохраняет ключ позиции вставки.
 *
 * @return Возвращает 0 при успешном завершении, иначе — код ошибки.
 */
int color()
{
    FILE *file, *keyFile, *outputFile;
    BMPHeader header;
    BMPInfoHeader infoHeader;
    unsigned char *imageData;
    char filename[256], outputFilename[256], text[101];
    int startPos;

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

    if (header.signature[0] != 'B' || header.signature[1] != 'M')
    {
        printf("Error: Not a valid BMP file\n");
        fclose(file);
        return 1;
    }

    if (infoHeader.bitsPerPixel != 24)
    {
        printf("Error: Only 24-bit BMP files are supported\n");
        fclose(file);
        return 1;
    }

    int imageDataSize = infoHeader.width * infoHeader.height * 3;

    imageData = (unsigned char *)malloc(imageDataSize);

    fseek(file, header.dataOffset, SEEK_SET);

    fread(imageData, 1, imageDataSize, file);

    fclose(file);

    printf("Enter text to encrypt (max 100 characters): ");

    scanf(" %[^\n]", text);

    if (strlen(text) > 100)
    {
        printf("Text too long! Maximum 100 characters allowed.\n");
        free(imageData);
        return 1;
    }

    int requiredBits = (strlen(text) * 8) + 32;

    int requiredBytes = (requiredBits + 7) / 8;

    if (requiredBytes > imageDataSize)
    {
        printf("Error: Image too small to hold the text.\n");
        printf("Required %d bytes, available %d bytes in image data.\n", requiredBytes, imageDataSize);
        free(imageData);
        return 1;
    }

    srand(time(NULL));
    int maxPossibleStartPos = imageDataSize - requiredBytes;

    if (maxPossibleStartPos < 0)
    {
        printf("Internal error: Calculated maxPossibleStartPos is negative. Image is too small.\n");
        free(imageData);
        return 1;
    }

    startPos = rand() % (maxPossibleStartPos + 1);

    embedText(imageData, imageDataSize, text, startPos);

    keyFile = fopen("color_key", "w");
    if (keyFile)
    {
        fprintf(keyFile, "%d", startPos);
        fclose(keyFile);
        printf("\nKey saved to 'color_key' file\n");
    }
    else
    {
        printf("Error: Could not open color_key file for writing.\n");
        free(imageData);
        return 1;
    }

    printf("Enter output filename: ");
    scanf("%s", outputFilename);

    outputFile = fopen(outputFilename, "wb");
    if (outputFile)
    {
        fwrite(&header, sizeof(BMPHeader), 1, outputFile);
        fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, outputFile);
        fseek(outputFile, header.dataOffset, SEEK_SET);
        fwrite(imageData, 1, imageDataSize, outputFile);
        fclose(outputFile);
        printf("Encrypted image saved as %s\n", outputFilename);
    }
    else
    {
        printf("Error: Could not open output file %s for writing.\n", outputFilename);
    }

    free(imageData);

    return 0;
}
