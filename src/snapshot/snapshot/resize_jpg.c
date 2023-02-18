#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <jpeglib.h>

void swapJsampRow(unsigned char *src, unsigned char *dest) {
    unsigned char *temp;
    temp = dest;
    dest = src;
    src = temp;
}

/*
 * resizeJpegFile
 */
int resizeJpegFile(char *inFileName, char *outFileName, float factor) {
    // init decompress struct
    struct jpeg_decompress_struct in;
    struct jpeg_error_mgr jInErr;

    JSAMPROW inRowPointer[1];

    FILE *inFile = fopen(inFileName, "rb");
    if (!inFile) {
        printf("Error opening jpeg file %s\n!", inFileName);
        return -1;
    }

    in.err = jpeg_std_error(&jInErr);
    jpeg_create_decompress(&in);
    jpeg_stdio_src(&in, inFile);
    jpeg_read_header(&in, TRUE);

    jpeg_start_decompress(&in);

    // init compress struct
    struct jpeg_compress_struct out;
    struct jpeg_error_mgr jOutErr;

    JSAMPROW outRowPointer[1];

    FILE *outFile = fopen(outFileName, "wb");
    if (!outFile) {
        printf("Error opening file %s\n", outFileName);
        return -1;
    }

    out.err = jpeg_std_error( &jOutErr);
    jpeg_create_compress(&out);
    jpeg_stdio_dest(&out, outFile);

    int width = in.output_width;
    int height = in.output_height;
    int bytesPerPixel = in.num_components;

    int destWidth = (int) (width * factor);
    int destHeight = (int) (height * factor);

    out.image_width = destWidth;
    out.image_height = destHeight;
    out.input_components = bytesPerPixel;
    out.in_color_space = JCS_RGB;

    jpeg_set_defaults(&out);
    jpeg_start_compress(&out, TRUE);

    // Process RGB data.
    int outRowStride = destWidth * bytesPerPixel;
    int inRowStride = width * bytesPerPixel;
    outRowPointer[0] = (unsigned char *) malloc(outRowStride);
    inRowPointer[0] = (unsigned char *) malloc(inRowStride);

    JSAMPROW baseInRowPointer[1];
    baseInRowPointer[0] = (unsigned char *) malloc(inRowStride);

    unsigned char bUpLeft, bUpRight, bDownLeft, bDownRight;
    unsigned char gUpLeft, gUpRight, gDownLeft, gDownRight;
    unsigned char rUpLeft, rUpRight, rDownLeft, rDownRight;
    unsigned char b, g, r;

    float fX, fY;
    int iX, iY;
    int i, j;

    int currentBaseLocation = -1;

    int count = 0;

    // Process the first line.
    jpeg_read_scanlines(&in, inRowPointer, 1);
    for (j = 0; j < destWidth; j++) {
        fX = ((float) j) / factor;
        iX = (int) fX;

        bUpLeft = inRowPointer[0][iX * 3 + 0];
        bUpRight = inRowPointer[0][(iX + 1) * 3 + 0];

        gUpLeft = inRowPointer[0][iX * 3 + 1];
        gUpRight = inRowPointer[0][(iX + 1) * 3 + 1];

        rUpLeft = inRowPointer[0][iX * 3 + 2];
        rUpRight = inRowPointer[0][(iX + 1) * 3 + 2];

        b = bUpLeft * (iX + 1 - fX) + bUpRight * (fX - iX);
        g = gUpLeft * (iX + 1 - fX) + gUpRight * (fX - iX);
        r = rUpLeft * (iX + 1 - fX) + rUpRight * (fX - iX);

        outRowPointer[0][j * 3 + 0] = b;
        outRowPointer[0][j * 3 + 1] = g;
        outRowPointer[0][j * 3 + 2] = r;
    }
    jpeg_write_scanlines(&out, outRowPointer, 1);

    currentBaseLocation = 0;

    // Process the other lines between the first and last.
    for (i = 1; i < destHeight - 1; i++) {
        fY = ((float) i) / factor;
        iY = (int) fY;

        if (iY == currentBaseLocation) {
            in.output_scanline = iY;
            swapJsampRow(inRowPointer[0], baseInRowPointer[0]);
            jpeg_read_scanlines(&in, baseInRowPointer, 1);
        } else {
            in.output_scanline = iY - 1;
            jpeg_read_scanlines(&in, inRowPointer, 1);
            jpeg_read_scanlines(&in, baseInRowPointer, 1);
        }

        currentBaseLocation = iY + 1;

        for (j = 0; j < destWidth; j++) {
            fX = ((float) j) / factor;
            iX = (int) fX;

            bUpLeft = inRowPointer[0][iX * 3 + 0];
            bUpRight = inRowPointer[0][(iX + 1) * 3 + 0];
            bDownLeft = baseInRowPointer[0][iX * 3 + 0];
            bDownRight = baseInRowPointer[0][(iX + 1) * 3 + 0];

            gUpLeft = inRowPointer[0][iX * 3 + 1];
            gUpRight = inRowPointer[0][(iX + 1) * 3 + 1];
            gDownLeft = baseInRowPointer[0][iX * 3 + 1];
            gDownRight = baseInRowPointer[0][(iX + 1) * 3 + 1];

            rUpLeft = inRowPointer[0][iX * 3 + 2];
            rUpRight = inRowPointer[0][(iX + 1) * 3 + 2];
            rDownLeft = baseInRowPointer[0][iX * 3 + 2];
            rDownRight = baseInRowPointer[0][(iX + 1) * 3 + 2];

            b = bUpLeft * (iX + 1 - fX) * (iY + 1 - fY) + bUpRight * (fX - iX) * (iY + 1 - fY) + bDownLeft * (iX + 1 - fX) * (fY - iY) + bDownRight * (fX - iX) * (fY - iY);
            g = gUpLeft * (iX + 1 - fX) * (iY + 1 - fY) + gUpRight * (fX - iX) * (iY + 1 - fY) + gDownLeft * (iX + 1 - fX) * (fY - iY) + gDownRight * (fX - iX) * (fY - iY);
            r = rUpLeft * (iX + 1 - fX) * (iY + 1 - fY) + rUpRight * (fX - iX) * (iY + 1 - fY) + rDownLeft * (iX + 1 - fX) * (fY - iY) + rDownRight * (fX - iX) * (fY - iY);

            outRowPointer[0][j * 3 + 0] = b;
            outRowPointer[0][j * 3 + 1] = g;
            outRowPointer[0][j * 3 + 2] = r;
        }

        jpeg_write_scanlines(&out, outRowPointer, 1);
    }

    //Process the last line.
    in.output_scanline = height - 1;
    jpeg_read_scanlines(&in, inRowPointer, 1);
    for (j = 0; j < destWidth; j++) {
        fX = ((float) j) / factor;
        iX = (int) fX;

        bUpLeft = inRowPointer[0][iX * 3 + 0];
        bUpRight = inRowPointer[0][(iX + 1) * 3 + 0];

        gUpLeft = inRowPointer[0][iX * 3 + 1];
        gUpRight = inRowPointer[0][(iX + 1) * 3 + 1];

        rUpLeft = inRowPointer[0][iX * 3 + 2];
        rUpRight = inRowPointer[0][(iX + 1) * 3 + 2];

        b = bUpLeft * (iX + 1 - fX) + bUpRight * (fX - iX);
        g = gUpLeft * (iX + 1 - fX) + gUpRight * (fX - iX);
        r = rUpLeft * (iX + 1 - fX) + rUpRight * (fX - iX);

        outRowPointer[0][j * 3 + 0] = b;
        outRowPointer[0][j * 3 + 1] = g;
        outRowPointer[0][j * 3 + 2] = r;
    }
    jpeg_write_scanlines(&out, outRowPointer, 1);

    //free memory
    free(inRowPointer[0]);
    free(baseInRowPointer[0]);
    free(outRowPointer[0]);

    // close resource
    jpeg_finish_decompress(&in);
    jpeg_destroy_decompress(&in);
    fclose(inFile);

    jpeg_finish_compress(&out);
    jpeg_destroy_compress(&out);
    fclose(outFile);

    return 0;
}

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s -i input_filename -o output_filename [-h]\n\n", progname);
    fprintf(stderr, "\t-i input_filename, --input_file input_filename\n");
    fprintf(stderr, "\t\tfile to resize\n");
    fprintf(stderr, "\t-o output_filename, --output_file output_filename\n");
    fprintf(stderr, "\t\tresized file\n");
    fprintf(stderr, "\t-d,     --debug\n");
    fprintf(stderr, "\t\tenable debug\n");
    fprintf(stderr, "\t-h,     --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

int main(int argc, char **argv) {
    char input_filename[1024];
    char output_filename[1024];
    int debug;
    int c;
    int errno;
    char *endptr;

    input_filename[0] = '\0';
    output_filename[0] = '\0';
    debug = 0;

    while (1) {
        static struct option long_options[] =
        {
            {"input_filename",  required_argument, 0, 'i'},
            {"output_filename",  required_argument, 0, 'o'},
            {"debug",  no_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "i:o:dh",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

        case 'i':
            memset(input_filename, '\0', 1024);
            if (strlen(optarg) < 1024) {
                strncpy(input_filename, optarg, strlen(optarg));
            } else {
                fprintf (stderr, "Error, file name too long\n");
                return -1;
            }
            break;

        case 'o':
            memset(output_filename, '\0', 1024);
            if (strlen(optarg) < 1024) {
                strncpy(output_filename, optarg, strlen(optarg));
            } else {
                fprintf (stderr, "Error, file name too long\n");
                return -1;
            }
            break;

        case 'd':
            fprintf (stderr, "debug on\n");
            debug = 1;
            break;

        case 'h':
            print_usage(argv[0]);
            return -1;
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    if (debug) fprintf(stderr, "Starting program\n");
    if (input_filename[0] == '\0') {
        fprintf(stderr, "No input file provided\n");
        return -2;
    }
    if (output_filename[0] == '\0') {
        fprintf(stderr, "No output file provided\n");
        return -2;
    }
    if (debug) fprintf(stderr, "Reading from %s\n", input_filename);
    if (debug) fprintf(stderr, "Writing to %s\n", output_filename);

    int result = resizeJpegFile(input_filename, output_filename, 1.0f / 3);
    if (result == 0) {
        if (debug) fprintf(stderr, "Program completed successfully\n");
    } else {
        fprintf(stderr, "Error resizing image\n");
    }

    return 0;
}
