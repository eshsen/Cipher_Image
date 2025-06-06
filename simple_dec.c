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
 * loadBMP - Загружает BMP изображение из файла.
 * @param filename: Имя файла изображения.
 * @param imageData: Указатель на указатель, куда будет сохранена выделенная память под данные изображения.
 * @param header: Указатель на структуру BMPHeader для хранения заголовка файла.
 * @param infoHeader: Указатель на структуру BMPInfoHeader для хранения информации об изображении.
 *
 * Возвращает размер изображения в байтах при успешной загрузке, 0 при ошибке.
 */
int loadBMP(const char *filename, unsigned char **imageData, BMPHeader *header, BMPInfoHeader *infoHeader)
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
 * decryptText - Извлекает скрытый текст из данных изображения, закодированный методом LSB.
 * @param imageData: Массив байтов данных изображения с встроенным текстом.
 * @param imageSize: Размер данных изображения в байтах.
 *
 * Возвращает указатель на строку с извлеченным текстом или NULL при ошибке.
 */
char *decryptText(unsigned char *imageData, int imageSize)
{
    int bitIndex = 0;
    int textLen = 0;

    for (int i = 0; i < 32; i++)
    {
        if (bitIndex >= imageSize)
            break;
        textLen = (textLen << 1) | (imageData[bitIndex] & 1);
        bitIndex++;
    }

    if (textLen <= 0 || textLen > 1000)
    {
        printf("Error: Invalid text length detected: %d\n", textLen);
        return NULL;
    }

    char *text = (char *)malloc(textLen + 1);
    text[textLen] = '\0';

    for (int i = 0; i < textLen; i++)
    {
        char ch = 0;
        for (int j = 7; j >= 0; j--)
        {
            if (bitIndex >= imageSize)
            {
                free(text);
                return NULL;
            }
            ch = (ch << 1) | (imageData[bitIndex] & 1);
            bitIndex++;
        }
        text[i] = ch;
    }

    return text;
}

/**
 * readKeyFile - Читает файл ключа и извлекает путь к зашифрованному изображению.
 * @param imagePath: Буфер для хранения пути к изображению (должен быть достаточно большим).
 *
 * Возвращает 1 при успешном чтении файла и извлечении пути, 0 при ошибке.
 */
int readKeyFile(char *imagePath)
{
    FILE *keyFile = fopen("simple_key", "r");
    if (!keyFile)
    {
        printf("Error: Cannot open key file 'simple_key'\n");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), keyFile))
    {
        if (strncmp(line, "ENCRYPTED_IMAGE:", 16) == 0)
        {
            strcpy(imagePath, line + 16);
            imagePath[strcspn(imagePath, "\n")] = 0;
        }
    }

    fclose(keyFile);
    return 1;
}

/**
 * simple_dec - Основная функция для декодирования текста из BMP изображения.
 *
 * Выполняет:
 * - Запрос пути к изображению у пользователя
 * - Загрузку изображения
 * - Извлечение скрытого текста из изображения
 * - Вывод расшифрованного сообщения
 *
 * Возвращает 0 при успешной работе или 1 при ошибках.
 */
int simple_dec()
{
    char imagePath[256];

    printf("\nBMP Image Text Decryption\n");
    printf("=========================\n\n");

    printf("Enter encrypted BMP filename: ");
    scanf("%255s", imagePath);

    BMPHeader header;
    BMPInfoHeader infoHeader;
    unsigned char *imageData;

    int imageSize = loadBMP(imagePath, &imageData, &header, &infoHeader);
    if (!imageSize)
        return 1;

    printf("Image loaded successfully!\n");

    char *decryptedText = decryptText(imageData, imageSize);

    printf("\n==================\n");
    printf("Decrypted message:\n\n");

    if (decryptedText)
    {
        printf("%s\n", decryptedText);
        free(decryptedText);
    }
    else
    {
        printf("Error: Could not extract text from image\n");
    }

    free(imageData);
    return 0;
}
