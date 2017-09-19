//set logfile [open "C:\\HLS\\log.txt" "w"]
//puts $logfile [mrd 0x1300000 76800 b]

#include <stdio.h>
#include "platform.h"
#include <xparameters.h>
#include "xaxidma.h"
#include "xdohist.h"
#include "xdohiststrech.h"
#include "AxiTimerHelper.h"
#include "LenaOnCode.h"

#define SIZE_ARR (320*240) //Resolution of our Image

//DMA Addresses - Memory used by DMA
#define MEM_BASE_ADDR 0x01000000
#define TX_BUFFER_BASE (MEM_BASE_ADDR + 0x00100000)
#define RX_BUFFER_BASE (MEM_BASE_ADDR + 0x00300000)

//Get a pointer to the TX and RX dma buffer (Configure DMA)
//The Pointers are for 8-bit memory but their addresses are 32bit (u32)
unsigned char *m_dma_buffer_TX = (unsigned char*) TX_BUFFER_BASE;
unsigned char *m_dma_buffer_RX = (unsigned char*) RX_BUFFER_BASE;

unsigned int hist_sw[256];
unsigned char imgOut[SIZE_ARR];
unsigned char imgIn_HW[SIZE_ARR];

unsigned int *hist_hw = (unsigned int *) 0x40000000;

XAxiDma axiDma;
void initDMA()
{

	XAxiDma_Config *CfgPtr;
	CfgPtr = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_DEVICE_ID);
	XAxiDma_CfgInitialize(&axiDma,CfgPtr);

	// Disable interrupts
	XAxiDma_IntrDisable(&axiDma,XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&axiDma,XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DMA_TO_DEVICE);

	//return XST_SUCCESS;
}

XDohist doHist;
XDohiststrech doHistStrech;

void initDoHist()
{

	XDohist_Config *doHist_cfg;
	printf("Initializing doHist\n");
	doHist_cfg = XDohist_LookupConfig(XPAR_DOHIST_0_DEVICE_ID);

	if (!doHist_cfg)
	{
		print("Error Loading config for doHist_cfg\n");
	}
	int status = XDohist_CfgInitialize(&doHist,doHist_cfg);
	if(status != XST_SUCCESS)
	{
		printf("Error Initializing doHist core\n");
	}
	//return status;
}

void initDoHistStrech()
{

	XDohiststrech_Config *doHistStrech_cfg;
	printf("Initializing doHist\n");
	doHistStrech_cfg = XDohiststrech_LookupConfig(XPAR_DOHISTSTRECH_0_DEVICE_ID);

	if (!doHistStrech_cfg)
	{
		print("Error Loading config for doHistStretch_cfg\n");
	}
	int status = XDohiststrech_CfgInitialize(&doHistStrech,doHistStrech_cfg);
	if(status != XST_SUCCESS)
	{
		printf("Error Initializing doHist core\n");
	}

}

void doHistSW(unsigned char *img, unsigned int *hist)
{
	//Reset Histogram
	for (int idx = 0; idx < 256; idx++)
	{
		hist[idx] = 0;
	}

	//Calculate Histogram
	for (int idxImg = 0; idxImg < (320*240); idxImg++)
	{
		hist[img[idxImg]] = hist[img[idxImg]] +1;
	}

}

void doHistStechSW(unsigned char *imgIn,unsigned char *imgOut, unsigned char xMin, unsigned char xMax)
{
	float xMax_minus_xMin = xMax-xMin;
	for (int idxImg =0 ; idxImg <(320*240);idxImg++)
	{
		float y_t_float = ((imgIn[idxImg] - xMin) / (xMax_minus_xMin))*255;
		//Convert from Float to Char
		imgOut[idxImg] = y_t_float;  //Char 0 to 255
	}
}

int main()
{
	initDMA();
	initDoHist();
	initDoHistStrech();
	AxiTimerHelper axiTimer;


//==============================SOFTWARE=IMPLEMENTATION===========================
	printf("Doing Histogram on SW\n");
	axiTimer.startTimer();
	doHistSW(img, hist_sw);
	axiTimer.stopTimer();
	double hist_SW_elapsed = axiTimer.getElapsedTimerInSeconds();
	printf("Histogram SW execution time: %f sec\n", hist_SW_elapsed);

	//Get min value from Histogram
	unsigned char xMin;
	for(int idxMin = 0; idxMin <256; idxMin++)
	{
		xMin = idxMin;
		if (hist_sw[idxMin])
			break;
	}

	//Get min value from Histogram
	unsigned char xMax;
	for(int idxMax = 255; idxMax >=0; idxMax--)
	{
		xMax = idxMax;
		if (hist_sw[idxMax])
			break;
	}

	printf("SW xMin = %d xMax = %d\n", xMin, xMax);

	print("Doing histogram Stretch SW\n");
	axiTimer.startTimer();
	doHistStechSW(img, imgOut,xMin,xMax);
	axiTimer.stopTimer();
	double hist_Stretch_SW_elapse = axiTimer.getElapsedTimerInSeconds();
	printf("Histogram Stretch SW execution time: %f sec\n", hist_Stretch_SW_elapse);


//==============================HARDWARE=IMPLEMENTATION===========================
	//Populate data(Get Image from Header and put into memory
	for (int idx = 0; idx < SIZE_ARR; idx++)
	{
		imgIn_HW[idx] = img[idx];
	}

	XDohist_Start(&doHist);
	//Dont care about result
	XDohiststrech_Set_xMax(&doHistStrech,255);
	XDohiststrech_Set_xMin(&doHistStrech,0);
	XDohiststrech_Start(&doHistStrech);


	axiTimer.startTimer();
	//Flush Buffers
	Xil_DCacheFlushRange((u32)imgIn_HW, SIZE_ARR*sizeof(unsigned char));
	Xil_DCacheFlushRange((u32)m_dma_buffer_RX, SIZE_ARR*sizeof(unsigned char));

	XAxiDma_SimpleTransfer(&axiDma,(u32)imgIn_HW,SIZE_ARR*sizeof(unsigned char),XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_SimpleTransfer(&axiDma,(u32)m_dma_buffer_RX,SIZE_ARR*sizeof(unsigned char),XAXIDMA_DEVICE_TO_DMA);

	//Wait for Transfers to Finish
	while(XAxiDma_Busy(&axiDma,XAXIDMA_DMA_TO_DEVICE));
	while(XAxiDma_Busy(&axiDma,XAXIDMA_DEVICE_TO_DMA));

	Xil_DCacheInvalidateRange((u32)m_dma_buffer_RX,SIZE_ARR*sizeof(unsigned char));
	axiTimer.stopTimer();
	double hist_HW_elapsed = axiTimer.getElapsedTimerInSeconds();
	printf("Histogram Stretch HW execution time: %f sec\n", hist_HW_elapsed);

	//Get Min Value
	for(int idxMin = 0; idxMin<256; idxMin++)
	{
		xMin = idxMin;
		if (hist_hw[idxMin])
			break;
	}

	//Get Max Value
	for(int idxMax = 256; idxMax>=0; idxMax--)
	{
		xMax = idxMax;
		if (hist_hw[idxMax])
			break;
	}
	printf("(HW) xMin =%d and xMax = %d\n", xMin, xMax);


	//Now we do the Histogram Stretch
	XDohist_Start(&doHist);
	//Dont care about result
	XDohiststrech_Set_xMax(&doHistStrech,xMax);
	XDohiststrech_Set_xMin(&doHistStrech,xMin);
	XDohiststrech_Start(&doHistStrech);

	axiTimer.startTimer();
		//Flush Buffers
	Xil_DCacheFlushRange((u32)imgIn_HW, SIZE_ARR*sizeof(unsigned char));
	Xil_DCacheFlushRange((u32)m_dma_buffer_RX, SIZE_ARR*sizeof(unsigned char));

	XAxiDma_SimpleTransfer(&axiDma,(u32)imgIn_HW,SIZE_ARR*sizeof(unsigned char),XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_SimpleTransfer(&axiDma,(u32)m_dma_buffer_RX,SIZE_ARR*sizeof(unsigned char),XAXIDMA_DEVICE_TO_DMA);

	//Wait for Transfers to Finish
	while(XAxiDma_Busy(&axiDma,XAXIDMA_DMA_TO_DEVICE));
	while(XAxiDma_Busy(&axiDma,XAXIDMA_DEVICE_TO_DMA));

	Xil_DCacheInvalidateRange((u32)m_dma_buffer_RX,SIZE_ARR*sizeof(unsigned char));
	axiTimer.stopTimer();
	double histStretch_HW_elapsed = axiTimer.getElapsedTimerInSeconds();
	printf("Histogram Stretch HW execution time: %f sec\n", histStretch_HW_elapsed);

	double TotalSW = (hist_SW_elapsed +hist_Stretch_SW_elapse);
	double TotalHW = (hist_HW_elapsed + histStretch_HW_elapsed);
	printf("Time Summary SW: %f HW: %f ratio:%f\n", TotalSW,TotalHW, TotalHW/TotalSW);

	printf("DMA out address: 0x%X\n", m_dma_buffer_RX);
/*	int imgMistmatch = 0;
	for(int idxComp = 0;idxComp<SIZE_ARR; idxComp++)
	{
		if (imgOut[idxComp] != m_dma_buffer_RX[idxComp])
		{
			printf("Invalid Response \n");
			imgMistmatch = 1;
		}
	}
	if (!imgMistmatch)
		printf("SW and HW images are the same\n");
*/
	return 0;
	//Xil_DCacheDisable();
	//Flush the cache of the buffers


	return 0;
}
