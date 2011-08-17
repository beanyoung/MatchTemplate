
#ifndef MATCH_TEMPALTE_H_
#define MATCH_TEMPALTE_H_

#include "cv.h"
#include "highgui.h"

/**
 * @class ShiftValue
 * @author BeanYoung
 * @date 06/25/2011
 * @file match_template.h
 * @brief contain the shift value
 */
struct ShiftValue
{
	double dX;
	double dY;
	double dAngle;
};

/**
 * @brief find best pTemplate in pSrc and the shift result saved in iResult
 * @param pTemplate: template image
 * @param pSrc: source image,where the template to be located 
 * @param dAngle: rotate from -dAngle to +dAngle
 * @param dScore: pass threshold
 * @param iResult: x,y and angle 
 * @return -1 input params error
 * 		    0 match error
 * 			1 match success
 */
int MatchTemplate(const IplImage* pTemplate,
				  const IplImage* pSource,
				  double dAngle,
				  double dScore,
				  ShiftValue& iResult);
				  
/**
 * @brief get the pyrdowned nTimes image
 * @param pSource
 * @param nTime
 * @return pyrdowned image
 */
IplImage* GetPyrDownImage(const IplImage* pSource, int nTime);

/**
 * @brief 
 * @param pTemplate: template to be matched
 * @param pSource: source image to be matched
 * @param dAngle: the rotate angle range is between -dAngle and dAngle
 * @param dStep: distance between each iterator
 * @param iShift: shift params
 * @return -1 input params error
 * 		    0 match error
 * 			1 match success
 */
int MatchWithAngle(const IplImage* pTemplate,
				   const IplImage* pSource,
				   double dAngle,
				   double dStep,
				   ShiftValue& iShift);

/**
 * @brief rotate pSource image with dAngle
 * @param pSource
 * @param dAngle
 * @return if success return rotated image
 * 		   else return NULL
 */
IplImage* RotateImage(const IplImage* pSource, double dAngle);

/**
 * @brief transfer the iShiftParam from (first angle, second transfer)
 * 								   to (laser engraving)
 * @param dX
 * @param dY
 * @param iSrcShiftParam
 * @param iDstShiftParam
 */
void TranferCoordinate(double dX, double dY,
					   const ShiftValue& iSrcShiftParam,
					   ShiftValue& iDstShiftParam);
					   
/**
 * @brief rotate the SrcPoint with Angle
 * @param iSrcPoint
 * @param dAngle
 * @param iDstPoint
 */
int GetRotatedPoint(const CvPoint& iSrcPoint,
					 double dAngle,
					 CvPoint& iDstPoint);

/**
 * @brief 
 * @param pSourceImage
 * @param iTemplateSize
 * @param iCenterPoint
 * @param dExpand: expand range from 0 to max
 * @param iTopLeftPoint: return top-left point of ROI
 * @return 1: success
 * 		  -1: input param is NULL
 * 		   0: unsuccess
 */
int SetImageROIWithCenterPoint(IplImage* pSourceImage,
							   CvSize iTemplateSize,
							   CvPoint iCenterPoint,
							   double dExpand,
							   CvPoint& iTopLeftPoint);

/**
 * @brief draw the rect and center
 * @param pSourceImage
 * @param iTemplateSize
 * @param iShiftParam
 * @param dColor
 */
void DrawResult(IplImage* pSourceImage,
				CvSize iTemplateSize,
				ShiftValue iShiftParam,
				double dColor);

/**
 * @brief add iA to iB and save the result in iC
 * @param iA
 * @param iB
 * @param iC
 */
void AddShiftParams(ShiftValue& iA, ShiftValue& iB, ShiftValue& iC);

/**
 * @brief first canny SourceImage and then expand the edge with witdh nWidth
 * @param pSourceImage
 * @param pEdge
 * @param nWidth
 */
int ExpandEdge(const IplImage* pSourceImage, IplImage* pEdge, int nWidth);

/**
 * @brief get the min pyrdown time
 * @param iTemplateSize
 * @param iSourceSize
 * @param nMinLength
 * @return 
 */
int GetPyrdownTime(CvSize iTemplateSize,
				   CvSize iSourceSize,
				   int nMinLength);

#endif

