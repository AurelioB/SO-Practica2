#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/sysinfo.h>

#define DIF 16
#define SIZE 65536

// For some reason these constants are not defined in sched.h, so weŕe adding them here

#define CSIGNAL         0x000000ff      /* signal mask to be sent at exit */
#define CLONE_VM        0x00000100      /* set if VM shared between processes */
#define CLONE_FS        0x00000200      /* set if fs info shared between processes */
#define CLONE_FILES     0x00000400      /* set if open files shared between processes */
#define CLONE_SIGHAND   0x00000800      /* set if signal handlers shared */



#pragma pack(2)  // Empaquetado de 2 bytes
typedef struct {
	unsigned char magic1;
	unsigned char magic2;
	unsigned int size;
	unsigned short int reserved1, reserved2;
	unsigned int pixelOffset; // offset a la imagen
} HEADER;

#pragma pack() // Empaquetamiento por default
typedef struct {
	unsigned int size; // Tamaño de este encabezado INFOHEADER 
	int cols, rows; // Renglones y columnas de la imagen unsigned short int planes;
	unsigned short int bitsPerPixel; // Bits por pixel
	unsigned int compression;
	unsigned int cmpSize;
	int xScale, yScale;
	unsigned int numColors;
	unsigned int importantColors;
} INFOHEADER;

typedef struct {
      unsigned char red;
      unsigned char green;
      unsigned char blue;
} PIXEL;

typedef struct {
      HEADER header;
      INFOHEADER infoheader;
      PIXEL *pixel;
} IMAGE;

IMAGE imagenfte, imagendst;
int imageRows, imageCols;

int loadBMP(char *filename, IMAGE *image) {
  	FILE *fin;
   	int i = 0;
   	int totpixs = 0;
   	fin = fopen(filename, "rb+");
   	// Si el archivo no existe
   	if (fin == NULL) {
   		printf("	ERROR: File does not exist\n");
   		return(-1);
   	}
   	// Leer encabezado
	fread(&image->header, sizeof(HEADER), 1, fin);
   	// Probar si es un archivo BMP
	if (!((image->header.magic1 == 'B') && (image->header.magic2 == 'M'))) {
		printf("	ERROR: Not a BMP file\n");
		return(-1);
	}
	fread(&image->infoheader, sizeof(INFOHEADER), 1, fin);
   	// Probar si es un archivo BMP 24 bits no compactado
	if (!((image->infoheader.bitsPerPixel == 24) && !image->infoheader.compression)) {
		printf("	WARNING: BMP file is not 24 bits, uncompressed (%d bits)\n", image->infoheader.bitsPerPixel);
		//return(-1);
	}
	printf("-- Width: %d\n-- Height: %d\n", image->infoheader.cols, image->infoheader.rows);
	image->pixel = (PIXEL *)malloc(sizeof(PIXEL) * image->infoheader.cols * image->infoheader.rows); 
	totpixs = image->infoheader.rows * image->infoheader.cols;
   	while(i < totpixs) {
		fread(image->pixel + i, sizeof(PIXEL), 512, fin); i+=512;
   	}

   	imageRows = image->infoheader.rows;
	imageCols = image->infoheader.cols;

   	fclose(fin);
   	return 0;
}

int saveBMP(char *filename, IMAGE *image) {
   	FILE *fout;
   	int i, totpixs;
   	fout=fopen(filename, "wb");
   	if (fout == NULL) return(-1); // Error // Escribe encabezado
	fwrite(&image->header, sizeof(HEADER), 1, fout); // Escribe información del encabezado
	fwrite(&image->infoheader, sizeof(INFOHEADER), 1, fout);
	i = 0;
	totpixs = image->infoheader.rows*image->infoheader.cols;
	while(i < totpixs) {
		fwrite(image->pixel+i,sizeof(PIXEL), 512, fout); i+=512;
	}
   	fclose(fout);
}

unsigned char blackandwhite(PIXEL p) {
	return((unsigned char) (0.3 * ((float)p.red) + 0.59 * ((float)p.green) + 0.11 * ((float)p.blue)));
}

void initBMP(IMAGE *imagefte, IMAGE *imagedst) {
	memcpy(imagedst, imagefte, sizeof(IMAGE) - sizeof(PIXEL *));
	imagedst->pixel = (PIXEL *) malloc(sizeof(PIXEL) * imagefte->infoheader.rows * imagefte->infoheader.cols);
}

void processBMP(IMAGE *imagefte, IMAGE *imagedst, int y0, int y1) {
	//IMAGE * imagefte = &imagenfte;
	int i, j;
	int count = 0;
	PIXEL *pfte, *pdst;
	PIXEL *v0, *v1, *v2, *v3, *v4, *v5, *v6, *v7;
	
	//memcpy(imagedst,imagefte,sizeof(IMAGE)-sizeof(PIXEL *));
	
	//imagedst->pixel=(PIXEL *)malloc(sizeof(PIXEL)*imageRows*imageCols);
    //for( i = 1; i < imageRows - 1; i++ )
    //printf("Y0: %d, Y1: %d\n", y0, y1);
    for( i = y0; i < (y1 - 1); i++ )
		for( j = 1; j < (imageCols - 1); j++ ){
			pfte = imagefte->pixel + imageCols * i + j;
			v0 = pfte - imageCols - 1;
			v1 = pfte - imageCols;
			v2 = pfte - imageCols + 1;
      		v3 = pfte - 1;
      		v4 = pfte + 1;
      		v5 = pfte + imageCols - 1;
      		v6 = pfte + imageCols;
      		v7 = pfte + imageCols + 1;

      		pdst = imagedst->pixel + imageCols * i + j;
			if(	abs( blackandwhite( *pfte ) - blackandwhite( *v0 ) ) > DIF || 
				abs( blackandwhite( *pfte ) - blackandwhite( *v1 ) ) > DIF || 
				abs( blackandwhite( *pfte ) - blackandwhite( *v2 ) ) > DIF || 
				abs( blackandwhite( *pfte ) - blackandwhite( *v3 ) ) > DIF || 
				abs( blackandwhite( *pfte ) - blackandwhite( *v4 ) ) > DIF || 
				abs( blackandwhite( *pfte ) - blackandwhite( *v5 ) ) > DIF || 
				abs( blackandwhite( *pfte ) - blackandwhite( *v6 ) ) > DIF || 
				abs( blackandwhite( *pfte ) - blackandwhite( *v7 ) ) > DIF
			) {
				pdst->red = 0;
				pdst->green = 0;
				pdst->blue = 0;
			} else  {
				pdst->red = 255;
				pdst->green = 255;
				pdst->blue = 255;
			}
		}
}

int run(int *argv) {
	//printf("\n---\nThread from %d to %d\n---\n", argv[0], argv[1]);
	processBMP(&imagenfte, &imagendst, argv[0], argv[1]);
	free(argv);
	return 0;
}

int main(int argc, char **argv) {
	void *child_stack;
	child_stack = (void *) malloc(SIZE);
  	int pid, status, rc;
  	int opt;
	char *filename = "input.bmp";
	char *destination = "output.bmp";

	int nThreads = get_nprocs();	// Number of threads. By default equal to the number of available cores

  	while ((opt = getopt (argc, argv, "n:i:o:")) != -1) {
    	switch (opt) {
           	case 'n':
            	nThreads = strtol(optarg, (char **)NULL, 10);
             	break;
           	case 'i':
             	filename = optarg;
             	break;
           	case 'o':
             	destination = optarg;
             	break;
           	case '?':
             	if (optopt == 'n' || optopt == 'i' || optopt == 'o')
               		fprintf (stderr, "Option -%c requires an argument.\n", optopt);
             	else if (isprint (optopt))
               		fprintf (stderr, "Unknown option `-%c'.\n", optopt);
             	else
               		fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
             	return 1;
           	default:
            	abort ();
		}
	}

	if(argc == 1) {
		printf("\nBMP border is a multithreaded tool to detect edges in BMP images.\n\n");
		printf("The following options are available:\n\n");
		printf("	-n	<number>	Number of threads to create\n");
		printf("				DEFAULT: Number of available cores\n\n");
		printf("	-i	<file>		Input file to read\n");
		printf("				DEFAULT: input.bmp\n\n");
		printf("	-o	<file>		Output file to write\n");
		printf("				DEFAULT: output.bmp\n\n");
	}
  	
	int res;
	clock_t t_inicial,t_final;
	

	t_inicial=clock();




	printf("-- Input File: %s\n",filename);
	
	res = loadBMP(filename, &imagenfte);
	if(res == -1) {
		fprintf(stderr,"	ERROR: Can't open source file\n");
		exit(1);
	}
	initBMP(&imagenfte, &imagendst);

	int n;
	int space = imagenfte.infoheader.rows / nThreads;
	printf("-- Cores available: %d\n", get_nprocs());
	printf("-- Thread execution (%d Total):\n", nThreads);
	for(n = 1; n <= nThreads; n++) {
		int *args = (int*) malloc(2 * sizeof(int));	// malloc space for clone arguments
		args[0] = space * (n - 1);
		args[1] = args[0] + space + 1;
		pid = clone(run, child_stack + (SIZE/sizeof(void *)) * n, CLONE_VM, args);
	}

  	status = 0;

  	do {
		rc = waitpid(-1, &status, __WCLONE);
		if(rc == -1)
			break;
		printf("	Thread with PID %d finished execution\n", rc);
	} while(1);
	printf("-- Output File: %s\n",destination);
	res = saveBMP(destination, &imagendst);
	if(res == -1) {
		fprintf(stderr,"	ERROR: Can't write output file\n");
		exit(1);
	}
	t_final = clock();
	printf("-- Time %3.6f seconds\n",((float) t_final- (float)t_inicial)/ CLOCKS_PER_SEC);
}
