#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "xnu-definitions.h"

size_t strlcpy(char *dst, const char *src, size_t n);
size_t strlcat(char *dst, const char *src, size_t n);

void hexify(unsigned char *data, uint32_t size)
{
	while (size--)
	{
		printf("%02x", *data++);
	}
}

void fcopy(FILE *f1, FILE *f2)
{
	char buffer[BUFSIZ];
	size_t n;

	while ((n = fread(buffer, sizeof(char), sizeof(buffer), f1)) > 0)
	{
		if (fwrite(buffer, sizeof(char), n, f2) != n)
			puts("error copying backup");
	}
}

size_t strlcpy(char *dst, const char *src, size_t n)
{
	char *p = dst;

	if (n != 0)
	{
		for (; --n != 0; p++, src++)
		{
			if ((*p = *src) == '\0')
				return p - dst;
		}
		*p = '\0';
	}
	return (p - dst) + strlen(src);
}

size_t strlcat(char *dst, const char *src, size_t n)
{
	char *p = dst;

	while (n != 0 && *p != '\0')
	{
		p++;
		n--;
	}
	if (n != 0)
	{
		for (; --n != 0; p++, src++)
		{
			if ((*p = *src) == '\0')
				return p - dst;
		}
		*p = '\0';
	}
	return (p - dst) + strlen(src);
}

int main(int argc, char *argv[])
{
	struct mach_header currentHeader;
	FILE *fp; // edited file pointer
	FILE *fw; // backup file pointer
	char fwName[80];
	char fwPrefix[4] = ".bak";

	if (argc < 1)
	{
		puts("please enter the filename binary: in the format ./removePIE filename");
		return EXIT_FAILURE;
	}

	if ((fp = fopen(argv[1], "rb+")) == NULL)
	{
		puts("error, unable to open file");
		return EXIT_FAILURE;
	}

	if ((fw = fopen(fwName, "wb")) == NULL)
	{
		return EXIT_FAILURE;
	}

	if ((fread(&currentHeader.magic, sizeof(int32_t), 1, fp)) == 0)
	{
		puts("error reading magic constant in file");
		return EXIT_FAILURE;
	}

	if (currentHeader.magic == MH_MAGIC || currentHeader.magic == MH_MAGIC_64)
	{
		// create backup
		strlcpy(fwName, argv[1], strlen(argv[1]) + 1);
		strlcat(fwName, fwPrefix, strlen(fwPrefix) + 1);

		puts("loading header");
		fseek(fp, 0, SEEK_SET);
		if ((fread(&currentHeader, sizeof(currentHeader), 1, fp)) == 0)
		{
			printf("error reading mach-o header");
			return EXIT_FAILURE;
		}

		fseek(fp, 0, SEEK_SET); // set fp back to 0 to get full copy
		puts("backing up application binary...");

		fcopy(fp, fw);
		fclose(fw);

		printf("binary backed up to %s\n", fwName);
		printf("mach_header: ");
		hexify((unsigned char *)&currentHeader, sizeof(currentHeader));
		printf("\noriginal flags:\t");
		hexify((unsigned char *)&currentHeader.flags, sizeof(currentHeader.flags));
		printf("\ndisabling ASLR...\n");
		currentHeader.flags &= ~MH_PIE;
		puts("new flags:\t");
		hexify((unsigned char *)&currentHeader.flags, sizeof(currentHeader.flags));

		fseek(fp, 0, SEEK_SET);
		if ((fwrite(&currentHeader, sizeof(char), 28, fp)) == 0)
		{
			printf("error writing to application file %s\n", fwName);
		}
		printf("\nASLR has been disabled for %s\n", argv[1]);
		// exit and close memory
		fclose(fp);
		return EXIT_SUCCESS;
	}
	else if (currentHeader.magic == MH_CIGAM || currentHeader.magic == MH_CIGAM_64) // big endian
	{
		puts("file is big endian, not an iOS binary!");
		return EXIT_FAILURE;
	}
	else
	{
		puts("file is not a Mach-O binary!");
		return EXIT_FAILURE;
	}

	return EXIT_FAILURE;
}

