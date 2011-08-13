
#include <stdio.h>
#include <math.h>

#include "match_template.h"

int MatchTemplate(const IplImage* pTemplate,
				  const IplImage* pSource,
				  double dAngle,
				  double dScore,
				  ShiftValue& iResult)
{
	// check the input params
	if (pTemplate == NULL || pSource == NULL)
	{
		printf("MatchTemplate input params is NULL!\n");
		return -1;
	}
	
	const CvSize iTemplateSize = cvGetSize(pTemplate);
	const CvSize iSrcSize = cvGetSize(pSource);
	if (iTemplateSize.height > iSrcSize.height
		|| iTemplateSize.width > iSrcSize.width)
	{
		printf("MatchTemplate Template size is larger than Source Image!\n");
		return -1;
	}

	if (dAngle < 0 || dAngle > 180)
	{
		printf("MatchTemplate input angle is out of range!\n");
		return -1;
	}
	// check the input params ends
	
	// pyrdown the source image
	// pyrdown times
	// get nPyrdownCount self-adaptively
	int nPyrdownCount = GetPyrdownTime(iTemplateSize, iSrcSize, 20);
	printf("PyrDown count is %d\n", nPyrdownCount);
	// pyrdowned source image
	IplImage* pPyrDownedSource = GetPyrDownImage(pSource, nPyrdownCount);
	// pyrdowned template image
	IplImage* pPyrDownedTemplate = GetPyrDownImage(pTemplate, nPyrdownCount);
	// pyrdown the source image ends
	
	// roughly match template
	double dFirstStep = 0.5;
	MatchWithAngle(pPyrDownedTemplate,
				   pPyrDownedSource,
				   dAngle,
				   dFirstStep,
				   iResult);
	// roughly match template ends
	
	// convert to init scale 
	// rotate source image with iResult.dA
	IplImage* pRotatedImage = RotateImage(pSource, iResult.dAngle);
	
	// set image ROI
	iResult.dX *= pow(2, nPyrdownCount);
	iResult.dY *= pow(2, nPyrdownCount);
	// 
	ShiftValue iRoughlyResult = iResult;
	
	printf("roughly matched result:\n");
	printf("X: %f\nY: %f\nAngle: %f\n",
		   iRoughlyResult.dX,
		   iRoughlyResult.dY,
		   iRoughlyResult.dAngle);
	
	// The top-left point of the ROI
	CvPoint iTopLeftPoint;
	double dExpand = 0.2;
	
	iResult.dX += iSrcSize.width / 2;
	iResult.dY += iSrcSize.height / 2;
	SetImageROIWithCenterPoint(pRotatedImage,
							   iTemplateSize,
							   cvPoint(iResult.dX, iResult.dY),
							   dExpand,
							   iTopLeftPoint);
	// convert to init scale ends
	
	IplImage* pCanniedTemplate = cvCloneImage(pTemplate);
	IplImage* pCanniedSource = cvCloneImage(pRotatedImage);
	ExpandEdge(pTemplate, pCanniedTemplate, 2);
	ExpandEdge(pRotatedImage, pCanniedSource, 2);
	
	// precisly match template
	MatchWithAngle(pCanniedTemplate,
				   pCanniedSource,
				   dFirstStep * 1.5,
				   dFirstStep * 0.1,
				   iResult);
	ShiftValue iTmpResult = iResult;
	
	printf("precisly matched result:\n");
	printf("X: %f\nY: %f\nAngle: %f\n",
		   iTmpResult.dX,
		   iTmpResult.dY,
		   iTmpResult.dAngle);	
	
	// add roughly match result and precisly match result to final result
	AddShiftParams(iRoughlyResult, iTmpResult, iResult);
	
	printf("final matched result:\n");
	printf("X: %f\nY: %f\nAngle: %f\n",
		   iResult.dX,
		   iResult.dY,
		   iResult.dAngle);	
	
	iResult.dX += iSrcSize.width / 2;
	iResult.dY += iSrcSize.height / 2;
	
	/*
	iTmpResult.dAngle += iRoughlyResult.dAngle;
	TranferCoordinate(iSrcSize.width / 2,
					  iSrcSize.height / 2,
					  iTmpResult,
					  iResult);
	*/
	// precisly match template ends
	
	cvReleaseImage(&pRotatedImage);
	cvReleaseImage(&pPyrDownedSource);
	cvReleaseImage(&pPyrDownedTemplate);
	cvReleaseImage(&pCanniedSource);
	cvReleaseImage(&pCanniedTemplate);
	return 1;
}
	
IplImage* GetPyrDownImage(const IplImage* pSource, int nTime)
{
	// check input params
	if (pSource == NULL)
	{
		printf("GetPyrDownImage Source Image is NULL:\n");
		return NULL;
	}
	if (nTime < 1)
	{
		printf("GetPyrDownImage pyrdown time is error!\n");
		return NULL;
	}
	// check input params ends
	
	// source image size
	CvSize iSourceSize = cvGetSize(pSource);
	CvSize iPyrDownSize = cvSize(iSourceSize.width / 2, iSourceSize.height / 2);
	
	// temp pyrdowned image
	IplImage* pPyrDownedImage = cvCreateImage(iPyrDownSize, IPL_DEPTH_8U, 1);
	cvPyrDown(pSource, pPyrDownedImage, CV_GAUSSIAN_5x5);
	// result image
	IplImage* pResult;
	
	if (nTime == 1)
	{
		pResult =  cvCloneImage(pPyrDownedImage);
	}
	else
	{
		pResult = GetPyrDownImage(pPyrDownedImage, --nTime);
	}
	
	cvReleaseImage(&pPyrDownedImage);
	return pResult;
}

int MatchWithAngle(const IplImage* pTemplate,
				   const IplImage* pSource,
				   double dAngle,
				   double dStep,
				   ShiftValue& iShift)
{
	// check input params
	if (pTemplate == NULL || pSource == NULL)
	{
		printf("MatchWithAngle input image is NULL!\n");
		return -1;
	}
	if (dAngle < 0 || dStep < 0 || dStep > dAngle)
	{
		printf("MatchWithAngle input angle and step is error!\n");
	}
	CvSize iTemplateSize = cvGetSize(pTemplate);
	CvSize iSourceSize = cvGetSize(pSource);
	if (iTemplateSize.width > iSourceSize.width ||
		iTemplateSize.height > iSourceSize.height)
	{
		printf("MatchWithAngle input image size is error!\n");
		return -1;
	}
	// check input params ends
	
	IplImage* pRotateImage = cvCloneImage(pSource);
	// ratete mat used in cvGetQuadrangleSubPix()
	CvMat* pRotateMat = cvCreateMat(2, 3, CV_32FC1);
	
	// cvMatchTemplate result mat
	CvMat* pMatchedResult 
			= cvCreateMat(iSourceSize.height - iTemplateSize.height + 1,
						  iSourceSize.width - iTemplateSize.width + 1,
					  	  CV_32FC1);
									 
	CvPoint iMinPos, iMaxPos;
	double dMinValue, dMaxValue;
	double dCmpValue = -1;
	// get the best matching result in this loop
	for (double dA = -dAngle; dA <dAngle + dStep; dA += dStep)
	{
		// make sure the angle is not larger than dAngle
		if (dA > dAngle)
		{
			dA = dAngle;
		}
		
		// get the rotate mat
		double dCos = cos(dA * CV_PI / 180.0);
		double dSin = sin(dA * CV_PI / 180.0);
		cvmSet(pRotateMat, 0, 0, dCos);
		cvmSet(pRotateMat, 0, 1, dSin);
		cvmSet(pRotateMat, 0, 2, iSourceSize.width / 2);
		cvmSet(pRotateMat, 1, 0, -dSin);
		cvmSet(pRotateMat, 1, 1, dCos);
		cvmSet(pRotateMat, 1, 2, iSourceSize.height / 2);
		
		// ratete the source image whit angle dA
		cvGetQuadrangleSubPix(pSource, pRotateImage, pRotateMat);
		
		// get the best match
		cvMatchTemplate(pRotateImage,
						pTemplate,
						pMatchedResult,
						CV_TM_CCOEFF_NORMED);
		cvMinMaxLoc(pMatchedResult,
					&dMinValue,
					&dMaxValue,
					&iMinPos,
					&iMaxPos,
					NULL);
		
		if (dCmpValue < dMaxValue)
		{
			dCmpValue = dMaxValue;
			iShift.dX = iMaxPos.x;
			iShift.dY = iMaxPos.y;
			iShift.dAngle = dA;
		}
		
		/*
		if (dCmpValue > dMinValue)
		{
			dCmpValue = dMinValue;
			iShift.dX = iMinPos.x;
			iShift.dY = iMinPos.y;
			iShift.dAngle = dA;
		}
		*/
		
	}
	
	iShift.dX += iTemplateSize.width / 2 - iSourceSize.width / 2;
	iShift.dY += iTemplateSize.height / 2 - iSourceSize.height / 2;
	
	cvReleaseImage(&pRotateImage);
	cvReleaseMat(&pRotateMat);
	cvReleaseMat(&pRotateMat);
	
	return 1;
}

IplImage* RotateImage(const IplImage* pSource, double dAngle)
{
	if (pSource == NULL)
	{
		printf("RotateImage input image is NULL!\n");
		return NULL;
	}
	
	if (dAngle == 0)
	{
		return cvCloneImage(pSource);
	}
	
	CvSize iSize = cvGetSize(pSource);
	double dCos = cos(dAngle * CV_PI / 180.0);
	double dSin = sin(dAngle * CV_PI / 180.0);
	
	CvMat* pRotateMat = cvCreateMat(2, 3, CV_32FC1);
	cvmSet(pRotateMat, 0, 0, dCos);
	cvmSet(pRotateMat, 0, 1, dSin);
	cvmSet(pRotateMat, 0, 2, iSize.width / 2);
	cvmSet(pRotateMat, 1, 0, -dSin);
	cvmSet(pRotateMat, 1, 1, dCos);
	cvmSet(pRotateMat, 1, 2, iSize.height / 2);
	
	IplImage* pRotatedImage = cvCloneImage(pSource);
	cvGetQuadrangleSubPix(pSource, pRotatedImage, pRotateMat);
	
	cvReleaseMat(&pRotateMat);
	
	return pRotatedImage;
}

void TranferCoordinate(double dX, double dY,
					   const ShiftValue& iSrcShiftParam,
					   ShiftValue& iDstShiftParam)
{
	double dOriginX = iSrcShiftParam.dX - dX;
	double dOriginY = iSrcShiftParam.dY - dY;
	// get dOriginX and dOriginY
	CvPoint iTmpPoint;
	GetRotatedPoint(cvPoint(dOriginX, dOriginY),
					iSrcShiftParam.dAngle,
					iTmpPoint);
	dOriginX = iTmpPoint.x + dX;
	dOriginY = iTmpPoint.y + dY;
	
	double dShiftX = iSrcShiftParam.dX;
	double dShiftY = iSrcShiftParam.dY;
	// get dShiftX and dShiftY
	GetRotatedPoint(cvPoint(dShiftX, dShiftY),
					iSrcShiftParam.dAngle,
					iTmpPoint);
	
	// iDstShiftParam.dX = dOriginX + iTmpPoint.x;
	// iDstShiftParam.dY = dOriginY + iTmpPoint.y;
	iDstShiftParam.dX = dOriginX;
	iDstShiftParam.dY = dOriginY;
	iDstShiftParam.dAngle = iSrcShiftParam.dAngle;
}

int GetRotatedPoint(const CvPoint& iSrcPoint,
					double dAngle,
					CvPoint& iDstPoint)
{
	if (dAngle > 180 || dAngle < -180)
	{
		printf("GetRotatedPoint input dAngle is out of range!\n");
		return -1;
	}
	if (dAngle == 0)
	{
		iDstPoint.x = iSrcPoint.x;
		iDstPoint.y = iSrcPoint.y;
		return 1;
	}
	
	double dCos = cos(dAngle * CV_PI / 180.0);
	double dSin = sin(dAngle * CV_PI / 180.0);
	
	iDstPoint.x = dCos * iSrcPoint.x + dSin * iSrcPoint.y;
	iDstPoint.y = -dSin * iSrcPoint.x + dCos * iSrcPoint.y;
	
	return 1;
}

int SetImageROIWithCenterPoint(IplImage* pSourceImage,
							   CvSize iTemplateSize,
							   CvPoint iCenterPoint,
							   double dExpand,
							   CvPoint& iTopLeftPoint)
{
	// check input param
	if (pSourceImage == NULL)
	{
		printf("SetImageROIWithCenterPoint input source image is NULL!\n");
		return -1;
	}
	
	if (dExpand < 0)
	{
		printf("SetImageROIWithCenterPoint expand is error!\n");
		return -1;
	}
	
	CvSize iSize = cvGetSize(pSourceImage);
	if (iCenterPoint.x < 0
		|| iCenterPoint.y < 0
		|| iCenterPoint.x > iSize.width
		|| iCenterPoint.y > iSize.height)
	{
		printf("SetImageROIWithCenterPoint left-top Point is error!\n");
		return -1;
	}
	// check input param ends
	
	CvRect iRect;
	iRect.x = iCenterPoint.x - iTemplateSize.width * (dExpand + 0.5);
	iRect.y = iCenterPoint.y - iTemplateSize.height * (dExpand + 0.5);
	iRect.height = iTemplateSize.height * (1 + 2 * dExpand);
	iRect.width = iTemplateSize.width * (1 + 2 * dExpand);
	
	if (iRect.x < 0)
	{
		iRect.x = 0;
	}
	if (iRect.y < 0)
	{
		iRect.y = 0;
	}
	if (iRect.x + iRect.width > iSize.width)
	{
		iRect.x = iSize.width - iRect.width;
	}
	if (iRect.y + iRect.height > iSize.height)
	{
		iRect.y = iSize.height - iRect.height;
	}
	
	cvSetImageROI(pSourceImage, iRect);
	
	iTopLeftPoint.x = iRect.x;
	iTopLeftPoint.y = iRect.y;
	
	return 1;
}

void DrawResult(IplImage* pSourceImage,
				CvSize iTemplateSize,
				ShiftValue iShiftParam,
				double dColor)
{
	if (pSourceImage == NULL)
	{
		printf("DrawResult input SourceImage is NULL");
		return;
	}
	
	double dX = iShiftParam.dX;
	double dY = iShiftParam.dY;
	double dWidth = iTemplateSize.width;
	double dHeight = iTemplateSize.height;
	
	CvPoint iTopLeft, iTopRight, iBottomLeft, iBottomRight;
	iTopLeft = cvPoint(-dWidth / 2, -dHeight / 2);
	iTopRight = cvPoint(dWidth / 2,  -dHeight / 2);
	iBottomLeft = cvPoint(-dWidth / 2, dHeight / 2);
	iBottomRight = cvPoint(dWidth / 2, dHeight / 2);
	
	CvPoint iTmpPoint;
	GetRotatedPoint(iTopLeft, iShiftParam.dAngle, iTmpPoint);
	iTopLeft = iTmpPoint;
	GetRotatedPoint(iTopRight, iShiftParam.dAngle, iTmpPoint);
	iTopRight = iTmpPoint;
	GetRotatedPoint(iBottomLeft, iShiftParam.dAngle, iTmpPoint);
	iBottomLeft = iTmpPoint;
	GetRotatedPoint(iBottomRight, iShiftParam.dAngle, iTmpPoint);
	iBottomRight = iTmpPoint;
	
	iTopLeft.x += dX;
	iTopRight.x += dX;
	iBottomLeft.x += dX;
	iBottomRight.x += dX;
	
	iTopLeft.y += dY;
	iTopRight.y += dY;
	iBottomLeft.y += dY;
	iBottomRight.y += dY;
	
	cvCircle(pSourceImage, cvPoint(dX, dY), 2, cvScalarAll(dColor));
	cvCircle(pSourceImage, iTopLeft, 2, cvScalarAll(dColor));
	cvCircle(pSourceImage, iTopRight, 2, cvScalarAll(dColor));
	cvCircle(pSourceImage, iBottomLeft, 2, cvScalarAll(dColor));
	cvCircle(pSourceImage, iBottomRight, 2, cvScalarAll(dColor));
	
	cvLine(pSourceImage, iTopLeft, iTopRight, cvScalarAll(dColor));
	cvLine(pSourceImage, iTopRight, iBottomRight, cvScalarAll(dColor));
	cvLine(pSourceImage, iBottomRight, iBottomLeft, cvScalarAll(dColor));
	cvLine(pSourceImage, iBottomLeft, iTopLeft, cvScalarAll(dColor));
}

void AddShiftParams(ShiftValue& iA, ShiftValue& iB, ShiftValue& iC)
{
	CvPoint iPointA, iPointB, iTmpPoint;
	
	iPointA = cvPoint(iA.dX, iA.dY);
	GetRotatedPoint(iPointA, iA.dAngle, iTmpPoint);
	iC.dAngle = iA.dAngle;
	iC.dX = iTmpPoint.x;
	iC.dY = iTmpPoint.y;
	
	iPointB = cvPoint(iB.dX, iB.dY);
	GetRotatedPoint(iPointB, iB.dAngle + iA.dAngle, iTmpPoint);
	iC.dAngle += iB.dAngle;
	iC.dX += iTmpPoint.x;
	iC.dY += iTmpPoint.y;
}

int ExpandEdge(const IplImage* pSourceImage, IplImage* pEdge, int nWidth)
{
	if (pSourceImage == NULL || pEdge == NULL)
	{
		printf("ExpandEdge input image is NULL!\n");
		return -1;
	}
	
	CvSize iSourceSize = cvGetSize(pSourceImage);
	CvSize iEdgeSize = cvGetSize(pEdge);
	if (iSourceSize.height != iEdgeSize.height
		|| iSourceSize.width != iEdgeSize.width)
	{
		printf("ExpandEdge input image size error!\n");
		return -1;
	}
	
	cvCanny(pSourceImage, pEdge, 50, 150, 3);
	cvSmooth(pEdge, pEdge, CV_BLUR, nWidth, nWidth, 0, 0);
	cvThreshold(pEdge, pEdge, 0, 200, CV_THRESH_BINARY);

	return 1;
}

int GetPyrdownTime(CvSize iTemplateSize,
				   CvSize iSourceSize,
				   int nMinLength)
{
	if (iTemplateSize.height > iSourceSize.height
	  || iTemplateSize.width > iSourceSize.width)
	{
		printf("GetPyrdownTime input size error!\n");
		return -1;
	}
	
	if (iTemplateSize.height == iSourceSize.height
	  || iTemplateSize.width == iSourceSize.width
	  || iTemplateSize.height <= nMinLength
	  || iTemplateSize.width <= nMinLength)
	{
		return 0;
	}
	
	int nRet = 0;
	while (iTemplateSize.height < iSourceSize.height
		&& iTemplateSize.width < iSourceSize.width
		&& iTemplateSize.height > nMinLength
		&& iTemplateSize.width > nMinLength)
	{
		iTemplateSize.height /= 2;
		iTemplateSize.width /= 2;
		++nRet;
	}
	--nRet;
	
	return nRet;
}
