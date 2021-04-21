#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#define PI 3.14159
#define FIR_size 60 // points for FIR coefficient
#define sample_size 80 // every sample's size
struct header{
	char ChunkID[4];
	int ChunkSize;
	char Format[4];
	char SubChunk1ID[4];
	int SubChunk1Size;
	short AudioFormat;
	short NumChannels;
	int SampleRate;
	int ByteRate;
	short BlockAlign;
	short BitsPerSample;
	char SubChunk2ID[4];
	int SubChunk2Size;
}Wav_Header;

double samp[sample_size + FIR_size -1];

void fir_F( double *in, double *out,double *coe,int fil ,int len )
{
    double *co_pointer;
    double *in_pointer;
    double val;
    memcpy( &samp[fil - 1], in, len * sizeof(double) ); // you will have to shift the samples toward the right end so the left end can store the value 0 and perform convolution
    for ( int n = 0; n < len; n++ ) {        // now we will perform convolution one by one
        co_pointer = coe;
        in_pointer = &samp[fil - 1 + n];      // it points to the start of the sample in the beginning and it will be moving
        val = 0;   // initilize val every time the convolution moves
        for ( int k = 0; k < fil; k++ ) {     // main part of the convolution
            val = val + (*co_pointer++) * (*in_pointer--);   //coefficient moves toward right, sample moves toward left
        }
        out[n] = val; //store the value
    }
    memmove( &samp[0], &samp[len], (fil - 1) * sizeof(double) ); //initilizeation

}




void floatize( double* out, short *in, int len){
	for ( int i = 0 ; i < len; i++){
		out[i] = (double)in[i];
	}
}

void intize(short *out,double * in , int len){
	for (int i = 0; i < len; i++){
		if(in[i] >32767.0){
			in[i] = 32767.0;
		}
		else if(in[i] < -32768){
			in[i] = -32768;
		}
		out[i] = (short) in[i];
	}
}

void init(void){
	memset(samp, 0, sizeof(samp));   //initialize
}



double *allocating_memory_double(unsigned int Total_Sample){
	return (double*)malloc(sizeof(double)* Total_Sample);
}


short * allocating_memory(unsigned int Total_Sample){
	return (short*)malloc(sizeof(short) * Total_Sample);
}



void co_hamming(float cut_freq, float Sampling_Rate, double * coe, short n){
	float Wn =(cut_freq * 2)/ Sampling_Rate;
	short mid = n/2;
		for (int i = 0 ; i < n ; i++){
			if(i != mid){
				coe[i] = sin(Wn * (i-mid))/ (PI * (i-mid)) * (0.5 * (1 - cos((2*PI*i)/ (n-1))));   //apply hann window to sinc function
			}
			else{
				coe[i] = Wn/PI; // l'hospital rule
			}
		}
}


int main(int argc, char *argv[]){
	char* WavIn_Name = argv[1];//input.wav
	char* WavOut_Name = argv[2];//output.wav
	short d = 0;
	int size;
	short inp[sample_size];
	short outp[sample_size];
	double floatIn[sample_size];
	double floatOut[sample_size];

	FILE* fp_WavIn;
	FILE* fp_WavOut;

	fp_WavIn = fopen(WavIn_Name,"rb");
	fp_WavOut = fopen(WavOut_Name,"wb");
	fread(&Wav_Header, 44, 1, fp_WavIn);
	int N = Wav_Header.SubChunk2Size/Wav_Header.BlockAlign; // total sample
	short *buffer = NULL;
	buffer = allocating_memory(N);
	short *del = NULL;
	del = allocating_memory(4801); // as i analyze the data 0.1 sec is equivalent to funcdamental freq
	double *FIR = NULL;
	FIR = allocating_memory_double(FIR_size);

	fseek(fp_WavIn, 44, SEEK_SET);
	fread(buffer, sizeof(short), N, fp_WavIn);
	for (int i = 0; i < 4801; i ++){
		del[i] = buffer[i];
	}

	for (int i = 0; i <N; i++, d++){
		buffer[i] = buffer[i] - del[d]; //this will create a noise signal that need to be canceled
		if(d == 4800)
			d = 0;
	}
	co_hamming(20000, 48000, FIR, 60); // the first argument takes in the cut off frequency of low pass filter
	// second argument takes in the sampling rate // third argument is the coefficient of FIR process // fourth argument is total sample
	fwrite(&Wav_Header, 44, 1, fp_WavOut);

	fseek(fp_WavIn, 44, SEEK_SET); // seek to the point where real data is located
	do{
			size = fread(inp, sizeof(short), sample_size, fp_WavIn); // first we need to read the size of the file to determine whether it's the end
			floatize(floatIn,inp , size);   //the data need to be float to be manipulated
			fir_F( floatIn, floatOut,FIR,FIR_size, size); // doing the convolution and things
			intize(outp, floatOut, size); //  convert back to int so it can be stored into wave file
			fwrite(outp, sizeof(short), size, fp_WavOut);
		}while (size!= 0);



	fclose(fp_WavOut);

}
