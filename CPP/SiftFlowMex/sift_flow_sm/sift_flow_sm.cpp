/************************************************************************************
*									   Includes										*
************************************************************************************/
#include <SiftFlow.h>
#include <ScaleMaps.h>
#include <vector>
#include <mex.h>
#include "MxArray.hpp"
#include <opencv2\imgproc.hpp>

/************************************************************************************
*									  Namespaces									*
************************************************************************************/
using std::vector;

/************************************************************************************
*									 Declarations									*
************************************************************************************/
#define printfFnc(...) { mexPrintf(__VA_ARGS__); mexEvalString("drawnow;");}

/************************************************************************************
*									Implementation									*
************************************************************************************/

void convertMatlabFeatures(const mxArray* arr, vector<dc::FeatureType>& features)
{
	cv::Mat featMat = MxArray(arr).toMat(CV_32F, false);
	features.resize(featMat.rows);
	memcpy(features.data(), featMat.data, featMat.rows*sizeof(dc::FeatureType));

	for (size_t i = 0; i < features.size(); ++i)
	{
		features[i].x -= 1;
		features[i].y -= 1;
	}
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	// Load images
	cv::Mat img1 = MxArray(prhs[0]).toMat(CV_32F);	// Convert to single floating point grayscale image
	cv::Mat img2 = MxArray(prhs[1]).toMat(CV_32F);	// Convert to single floating point grayscale image

	// Load matched features
	vector<dc::FeatureType> feat1, feat2;
	convertMatlabFeatures(prhs[2], feat1);
	convertMatlabFeatures(prhs[3], feat2);

	// define parameters
	double alpha = 0.01;
	double d = 1;
	double gamma = 0.001;
	int nlevels = 4;
	int topwsize = 10;
	int wsize = 5;
	int nTopIterations = 60;
	int nIterations = 40;
	int nHierarchy = 2;

	// Load the parameters for matching
	if (nrhs >= 3)
	{
		alpha = mxGetPr(mxGetField(prhs[4], 0, "alpha"))[0];
		d = mxGetPr(mxGetField(prhs[4], 0, "d"))[0];
		gamma = mxGetPr(mxGetField(prhs[4], 0, "gamma"))[0];
		nlevels = mxGetPr(mxGetField(prhs[4], 0, "nlevels"))[0];
		topwsize = mxGetPr(mxGetField(prhs[4], 0, "topwsize"))[0];
		wsize = mxGetPr(mxGetField(prhs[4], 0, "wsize"))[0];
		nTopIterations = mxGetPr(mxGetField(prhs[4], 0, "nTopIterations"))[0];
		nIterations = mxGetPr(mxGetField(prhs[4], 0, "nIterations"))[0];
	}
	
	// Initialize
	dc::SiftFlow siftFlow(d, nlevels, alpha, gamma, nIterations, nTopIterations,
		wsize, topwsize);

	// Create scale maps
	dc::ScaleMaps sm;
	sm.setWeightFunc(true);
	std::vector<float> scaleMap1, scaleMap2;
	printfFnc("Calculating the scale map for the first image...\n");
	sm.run(img1, feat1, scaleMap1);
	printfFnc("Calculating the scale map for the second image...\n");
	sm.run(img2, feat2, scaleMap2);

	// Create SIFT images
	cv::Mat siftImg1, siftImg2;
	printfFnc("Creating the first SIFT image...\n");
	siftFlow.createSIFTImage(img1, siftImg1, scaleMap1);
	printfFnc("Creating the second SIFT image...\n");
	siftFlow.createSIFTImage(img2, siftImg2, scaleMap2);

	// Do SIFT flow
	cv::Mat vx, vy;
	printfFnc("Doing course to fine SIFT flow...\n");
	siftFlow.coarse2fineFlow(siftImg1, siftImg2, vx, vy);

	// Output to Matlab
	plhs[0] = MxArray(vx);
	plhs[1] = MxArray(vy);

	/// Debug ///
	//FImage out(siftImg1.cols, siftImg1.rows, siftImg1.channels());
	//memcpy(out.pData, siftImg1.data, siftImg1.cols*siftImg1.rows*siftImg1.channels()*sizeof(float));
	//out.OutputToMatlab(plhs[2]);
	/////////////
}