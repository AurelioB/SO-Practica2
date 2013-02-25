#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <assert.h>
#include <sys/sysinfo.h>

#define DIF 16
#define SIZE 65536

// For some reason these constants are not defined in sched.h, so weŕe adding them here

#define CSIGNAL         0x000000ff      /* signal mask to be sent at exit */
#define CLONE_VM        0x00000100      /* set if VM shared between processes */
#define CLONE_FS        0x00000200      /* set if fs info shared between processes */
#define CLONE_FILES     0x00000400      /* set if open files shared between processes */
#define CLONE_SIGHAND   0x00000800      /* set if signal handlers shared */

// NOMBRE DEL ARCHIVO A PROCESAR
char filename[]="test.bmp";

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

int loadBMP(char *filename, IMAGE *image) {
  	FILE *fin;
   	int i=0;
   	int totpixs=0;
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
		printf("	ERROR: BMP file is in not in 24 bits, uncompressed format (%d bits)\n", image->infoheader.bitsPerPixel);
		//return(-1);
	}
	printf("-- Width: %d\n-- Height: %d\n", image->infoheader.cols, image->infoheader.rows);
	image->pixel = (PIXEL *)malloc(sizeof(PIXEL) * image->infoheader.cols * image->infoheader.rows); 
	totpixs=image->infoheader.rows*image->infoheader.cols;
   	while(i<totpixs) {
		fread(image->pixel+i, sizeof(PIXEL),512, fin); i+=512;
   	}
   	fclose(fin);
   	return 0;
}

int saveBMP(char *filename, IMAGE *image) {
   	FILE *fout;
   	int i,totpixs;
   	fout=fopen(filename,"wb");
   	if (fout == NULL) return(-1); // Error // Escribe encabezado
	fwrite(&image->header, sizeof(HEADER), 1, fout); // Escribe información del encabezado
	fwrite(&image->infoheader, sizeof(INFOHEADER), 1, fout);
	i=0;
	totpixs=image->infoheader.rows*image->infoheader.cols;
	while(i<totpixs) {
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

void processBMP(IMAGE imagenfte, IMAGE *imagedst, int y0, int y1) {
	IMAGE * imagefte = &imagenfte;
	int i, j;
	int count = 0;
	PIXEL *pfte, *pdst;
	PIXEL *v0, *v1, *v2, *v3, *v4, *v5, *v6, *v7;
	int imageRows, imageCols;
	//memcpy(imagedst,imagefte,sizeof(IMAGE)-sizeof(PIXEL *));
	imageRows = imagefte->infoheader.rows;
	imageCols = imagefte->infoheader.cols;
	//imagedst->pixel=(PIXEL *)malloc(sizeof(PIXEL)*imageRows*imageCols);
    //for( i = 1; i < imageRows - 1; i++ )
    printf("Y0: %d, Y1: %d\n", y0, y1);
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
			if(	abs(blackandwhite(*pfte) - blackandwhite(*v0)) > DIF || 
				abs(blackandwhite(*pfte) - blackandwhite(*v1)) > DIF || 
				abs(blackandwhite(*pfte) - blackandwhite(*v2)) > DIF || 
				abs(blackandwhite(*pfte) - blackandwhite(*v3)) > DIF || 
				abs(blackandwhite(*pfte) - blackandwhite(*v4)) > DIF || 
				abs(blackandwhite(*pfte) - blackandwhite(*v5)) > DIF || 
				abs(blackandwhite(*pfte) - blackandwhite(*v6)) > DIF || 
				abs(blackandwhite(*pfte) - blackandwhite(*v7)) > DIF
			) {
				pdst->red = 0;
				pdst->green = 0;
				pdst->blue = 0;
			} else  /*if(i > y0 && i < y1)*/ {
				pdst->red = 255;
				pdst->green = 255;
				pdst->blue = 255;
			}
		}
}

int run(int *argv) {
	printf("\n---\nThread from %d to %d\n---\n", argv[0], argv[1]);
	processBMP(imagenfte, &imagendst, argv[0], argv[1]);
	free(argv);
	return 0;
}

int main() {
	void *child_stack;
	child_stack = (void *) malloc(SIZE);
  	assert(child_stack != NULL);
  	int pid, status, rc;
  	printf("%d cores available\n", get_nprocs());
	int nThreads = get_nprocs();

	int res;
	clock_t t_inicial,t_final;
	char namedest[80];
	t_inicial=clock();
	strcpy(namedest,strtok(filename,"."));
	strcat(filename,".bmp");
	strcat(namedest,"_P.bmp");
	printf("-- Source: %s\n",filename);
	printf("-- Output: %s\n",namedest);
	res = loadBMP(filename, &imagenfte);
	if(res == -1) {
		fprintf(stderr,"	ERROR: Can't open source file\n");
		exit(1);
	}
	initBMP(&imagenfte, &imagendst);
	//printf("Procesando imagen de: Renglones = %d, Columnas = %d\n", imagenfte.infoheader.rows, imagenfte.infoheader.cols);
	/*initBMP(&imagenfte, &imagendst);
	int args[2];
	args[0] = 1;
	args[1] = 101;
	pid = clone(run, child_stack + SIZE/sizeof(void *), 0x00000100, args);
	int args2[2];
	args2[0] = 100;
	args2[1] = 201;

	pid = clone(run, child_stack + (SIZE * 2) /sizeof(void *), 0x00000100, args2);*/
	int n;
	int space = imagenfte.infoheader.rows / nThreads;
	for(n = 1; n <= nThreads; n++) {
		//int args[2];
		int *args = (int*) malloc(2 * sizeof(int));
		args[0] = space * (n - 1);
		args[1] = args[0] + space;
		printf("Thread %d: %d - %d\n", n, args[0], args[1]);
		pid = clone(run, child_stack + (SIZE/sizeof(void *)) * n, 0x00000100, args);
	}

	//assert(pid != -1);
  	status = 0;
  	while(rc != -1) {
		rc = waitpid(-1, &status, __WCLONE);
		printf("THREAD RC: %d\n", rc);
	}

	//assert(rc != -1);
  	//assert(WEXITSTATUS(status) == 0);

	//processBMP(&imagenfte, &imagendst, 1, 50);
	//processBMP(&imagenfte, &imagendst, 100, 150);
	//processBMP(&imagenfte, &imagendst, 300, 400);
	//processBMP(&imagenfte, &imagendst, 200, 300);
	//processBMP(imagenfte, &imagendst, 1, 200);
	res = saveBMP(namedest, &imagendst);
	if(res == -1) {
		fprintf(stderr,"	ERROR: Can't write output file\n");
		exit(1);
	}
	t_final = clock();
	printf("Tiempo %3.6f segundos\n",((float) t_final- (float)t_inicial)/ CLOCKS_PER_SEC);
}
