#include <stdio.h>
#include <stdlib.h>
#include "stegano.h"
#include "stegano_dec.h"
#include "color.h"
#include "color_dec.h"
#include "simple.h"
#include "simple_dec.h"

int main()
{
    int choice, method;

    printf("Welcome! If you want to encrypt the message enter 1, otherwise 0: ");
    scanf("%d", &choice);

    if (choice == 1)
    {
        printf("\nWhat method do you want to use?\n");
        printf("1 - steganography\n");
        printf("2 - color substitution\n");
        printf("3 - direct encryption\n");
        printf("Enter your choice: ");
        scanf("%d", &method);

        switch (method)
        {
        case 1:
            return stegano();
        case 2:
            return color();
        case 3:
            return simple();
        default:
            printf("Invalid choice!\n");
            return 1;
        }
    }
    else if (choice == 0)
    {
        printf("\nWhat method did you use?\n");
        printf("1 - steganography\n");
        printf("2 - color substitution\n");
        printf("3 - direct encryption\n");
        printf("Enter your choice: ");
        scanf("%d", &method);

        switch (method)
        {
        case 1:
            return stegano_dec();
        case 2:
            return color_dec();
        case 3:
            return simple_dec();
        default:
            printf("Invalid choice!\n");
            return 1;
        }
    }
    else
    {
        printf("Invalid choice! Please enter 1 for encryption or 0 for decryption.\n");
        return 1;
    }
}
