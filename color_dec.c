#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * @brief Извлекает скрытый текст из массива данных изображения, начиная с указанной позиции.
 *
 * Эта функция читает длину текста (в битах), закодированную в первых 32 битах,
 * затем извлекает сам текст побитно.
 *
 * @param imageData Указатель на массив байтов данных изображения.
 * @param dataSize Размер массива imageData в байтах.
 * @param startPos Начальная позиция в массиве для чтения скрытого текста.
 * @return Указатель на строку с извлечённым текстом. Необходимо освободить память после использования.
 *         Возвращает NULL, если длина текста некорректна или возникла ошибка.
 */
char *extractText(unsigned char *imageData, int dataSize, int startPos)
{
    int textLen = 0;
    for (int i = 0; i < 32; i++)
    {
        int bit = imageData[startPos + i] & 1;
        textLen |= (bit << i);
    }

    if (textLen <= 0 || textLen > 1000)
    {
        return NULL;
    }

    char *text = (char *)malloc(textLen + 1);
    if (!text)
        return NULL;

    int pos = startPos + 32;

    for (int i = 0; i < textLen; i++)
    {
        char ch = 0;
        for (int j = 0; j < 8; j++)
        {
            if (pos >= dataSize)
            {
                free(text);
                return NULL;
            }
            int bit = imageData[pos] & 1;
            ch |= (bit << j);
            pos++;
        }
        text[i] = ch;
    }

    text[textLen] = '\0';
    return text;
}

/**
 * @brief Основная функция для дешифровки скрытого текста из BMP изображения.
 *
 * Загружает ключ из файла "color_key", затем читает изображение,
 * извлекает скрытый текст и выводит его на экран.
 *
 * @return Возвращает 0 при успешном завершении, иначе — код ошибки.
 */
int color_dec()
{
    printf("\nBMP Image Text Decryption\n");
    printf("=========================\n\n");

    FILE *file, *keyFile;
    BMPHeader header;
    BMPInfoHeader infoHeader;
    unsigned char *imageData;
    char filename[256];
    int startPos;

    keyFile = fopen("color_key", "r");
    if (!keyFile)
    {
        printf("Error: Cannot open color_key file\n");
        return 1;
    }

    fscanf(keyFile, "%d", &startPos);
    fclose(keyFile);

    printf("Enter encrypted BMP filename: ");
    scanf("%s", filename);
    printf("Image loaded successfully!\n");

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

    int imageDataSize = infoHeader.width * infoHeader.height * 3;

    imageData = (unsigned char *)malloc(imageDataSize);
    if (!imageData)
    {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }

    fseek(file, header.dataOffset, SEEK_SET);
    fread(imageData, 1, imageDataSize, file);
    fclose(file);

    char *extractedText = extractText(imageData, imageDataSize, startPos);

    printf("\n==================\n");
    printf("Decrypted message:\n\n");

    if (extractedText)
    {
        printf("%s\n", extractedText);
        free(extractedText);
    }
    else
    {
        printf("Error: Could not extract text from image\n");
    }

    free(imageData);

    return 0;
}