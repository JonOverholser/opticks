/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "GpuImage.h"
#include "GlSlContext.h"
#include "ColorBuffer.h"
#include "ConfigurationSettings.h"
#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "glCommon.h"
#include "GpuResourceManager.h"
#include "GpuTile.h"
#include "ImageFilter.h"
#include "ImageUtilities.h"
#include "MathUtil.h"
#include "MultiThreadedAlgorithm.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterLayer.h"
#include "RasterUtilities.h"
#include "UtilityServicesImp.h"

#include <limits>
using namespace mta;
using namespace std;

GLint GpuImage::mMaxTextureSize = 0;
bool GpuImage::mAlwaysAlpha = true;
bool GpuImage::mAlphaConfigChecked = false;

GpuImage::GpuImage() :
   mGrayscaleProgram(),
   mColormapProgram(),
   mColormapTexture(0),
   mRgbProgram(),
   mGpuProgramObject(-1),
   mPreviousBand(0)
{
   GlSlContext* pGlSlContext = GlSlContext::instance();

   mGrayscaleProgram.initialize();
   mColormapProgram.initialize();
   mRgbProgram.initialize();
   
   if (mAlphaConfigChecked == false)
   {
      mAlwaysAlpha = RasterLayer::getSettingGpuImageAlwaysAlpha();
      mAlphaConfigChecked = true;
   }
}

GpuImage::~GpuImage()
{
   GlSlContext* pGlSlontext = GlSlContext::instance();

   // Unload and destory the shader programs
   mGrayscaleProgram.dispose();
   mColormapProgram.dispose();
   mRgbProgram.dispose();
}

// Grayscale
void GpuImage::initialize(int sizeX, int sizeY, DimensionDescriptor channel, unsigned int imageSizeX,
                          unsigned int imageSizeY, unsigned int channels, GLenum format, EncodingType type,
                          void* pData, StretchType stretchType, vector<double>& stretchPoints,
                          RasterElement *pRasterElement, const BadValues* pBadValues)
{
   int tileSizeX = 0;
   int tileSizeY = 0;
   calculateTileSize(type, imageSizeX, imageSizeY, tileSizeX, tileSizeY);

   initializeGrayscale();

   GLenum texFormat = (mAlwaysAlpha || (format == GL_RGBA || format == GL_LUMINANCE_ALPHA) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE);
   Image::initialize(tileSizeX, tileSizeY, channel, imageSizeX, imageSizeY, channels, texFormat, type, pData,
      stretchType, stretchPoints, pRasterElement, pBadValues);
}

void GpuImage::initialize(int sizeX, int sizeY, DimensionDescriptor channel, unsigned int imageSizeX,
                          unsigned int imageSizeY, unsigned int channels, GLenum format, EncodingType type,
                          ComplexComponent component, void* pData, StretchType stretchType,
                          vector<double>& stretchPoints, RasterElement* pRasterElement, const BadValues* pBadValues)
{
   int tileSizeX = 0;
   int tileSizeY = 0;
   calculateTileSize(type, imageSizeX, imageSizeY, tileSizeX, tileSizeY);

   initializeGrayscale();

   GLenum texFormat = (mAlwaysAlpha || (format == GL_RGBA || format == GL_LUMINANCE_ALPHA) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE);
   Image::initialize(tileSizeX, tileSizeY, channel, imageSizeX, imageSizeY, channels, texFormat, type, component,
      pData, stretchType, stretchPoints, pRasterElement, pBadValues);
}

// Colormap
void GpuImage::initialize(int sizeX, int sizeY, DimensionDescriptor channel, unsigned int imageSizeX,
                          unsigned int imageSizeY, unsigned int channels, GLenum format, EncodingType type,
                          void* pData, StretchType stretchType, vector<double>& stretchPoints,
                          RasterElement* pRasterElement, const vector<ColorType>& colorMap,
                          const BadValues* pBadValues)
{
   int tileSizeX = 0;
   int tileSizeY = 0;
   calculateTileSize(type, imageSizeX, imageSizeY, tileSizeX, tileSizeY);

   initializeColormap(colorMap);

   GLenum texFormat = (mAlwaysAlpha || (format == GL_RGBA || format == GL_LUMINANCE_ALPHA) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE);
   Image::initialize(tileSizeX, tileSizeY, channel, imageSizeX, imageSizeY, channels, texFormat, type, pData,
      stretchType, stretchPoints, pRasterElement, colorMap, pBadValues);
}

void GpuImage::initialize(int sizeX, int sizeY, DimensionDescriptor channel, unsigned int imageSizeX,
                          unsigned int imageSizeY, unsigned int channels, GLenum format, EncodingType type,
                          ComplexComponent component, void* pData, StretchType stretchType,
                          vector<double>& stretchPoints, RasterElement* pRasterElement,
                          const vector<ColorType>& colorMap, const BadValues* pBadValues)
{
   int tileSizeX = 0;
   int tileSizeY = 0;
   calculateTileSize(type, imageSizeX, imageSizeY, tileSizeX, tileSizeY);

   initializeColormap(colorMap);

   GLenum texFormat = (mAlwaysAlpha || (format == GL_RGBA || format == GL_LUMINANCE_ALPHA) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE);
   Image::initialize(tileSizeX, tileSizeY, channel, imageSizeX, imageSizeY, channels, texFormat, type, component,
      pData, stretchType, stretchPoints, pRasterElement, colorMap, pBadValues);
}

// RGB
void GpuImage::initialize(int sizeX, int sizeY, DimensionDescriptor band1, DimensionDescriptor band2,
                          DimensionDescriptor band3, unsigned int imageSizeX, unsigned int imageSizeY,
                          unsigned int channels, GLenum format, EncodingType type, void* pData,
                          StretchType stretchType, vector<double>& stretchPointsRed, vector<double>& stretchPointsGreen,
                          vector<double>& stretchPointsBlue, RasterElement* pRasterElement,
                          const BadValues* pBadValues1, const BadValues* pBadValues2,
                          const BadValues* pBadValues3)
{
   initializeRgb();

   int tileSizeX = 0;
   int tileSizeY = 0;
   calculateTileSize(type, imageSizeX, imageSizeY, tileSizeX, tileSizeY);

   Image::initialize(tileSizeX, tileSizeY, band1, band2, band3, imageSizeX, imageSizeY, channels, format, type,
      pData, stretchType, stretchPointsRed, stretchPointsGreen, stretchPointsBlue, pRasterElement, pBadValues1,
      pBadValues2, pBadValues3);
}

void GpuImage::initialize(int sizeX, int sizeY, DimensionDescriptor band1, DimensionDescriptor band2,
                          DimensionDescriptor band3, unsigned int imageSizeX, unsigned int imageSizeY,
                          unsigned int channels, GLenum format, EncodingType type, ComplexComponent component,
                          void* pData, StretchType stretchType, vector<double>& stretchPointsRed,
                          vector<double>& stretchPointsGreen, vector<double>& stretchPointsBlue,
                          RasterElement* pRasterElement, const BadValues* pBadValues1,
                          const BadValues* pBadValues2, const BadValues* pBadValues3)
{
   initializeRgb();

   int tileSizeX = 0;
   int tileSizeY = 0;
   calculateTileSize(type, imageSizeX, imageSizeY, tileSizeX, tileSizeY);

   Image::initialize(tileSizeX, tileSizeY, band1, band2, band3, imageSizeX, imageSizeY, channels, format, type,
      component, pData, stretchType, stretchPointsRed, stretchPointsGreen, stretchPointsBlue, pRasterElement,
      pBadValues1, pBadValues2, pBadValues3);
}

void GpuImage::initialize(int sizeX, int sizeY, DimensionDescriptor band1, DimensionDescriptor band2,
                          DimensionDescriptor band3, unsigned int imageSizeX, unsigned int imageSizeY,
                          unsigned int channels, GLenum format, EncodingType type1, EncodingType type2,
                          EncodingType type3, ComplexComponent component, void* pData, StretchType stretchType,
                          vector<double>& stretchPointsRed, vector<double>& stretchPointsGreen,
                          vector<double>& stretchPointsBlue, RasterElement* pRasterElement1,
                          RasterElement* pRasterElement2, RasterElement* pRasterElement3,
                          const BadValues* pBadValues1, const BadValues* pBadValues2,
                          const BadValues* pBadValues3)
{
   initializeRgb();

   // Calculate the tile size
   int tileSizeX = 0;
   int tileSizeY = 0;

   if ((type1 == type2) && (type1 == type3))
   {
      calculateTileSize(type1, imageSizeX, imageSizeY, tileSizeX, tileSizeY);
   }
   else
   {
      // Since the encoding types are different, float textures are created
      calculateTileSize(FLT4BYTES, imageSizeX, imageSizeY, tileSizeX, tileSizeY);
   }

   Image::initialize(tileSizeX, tileSizeY, band1, band2, band3, imageSizeX, imageSizeY, channels, format, type1,
      type2, type3, component, pData, stretchType, stretchPointsRed, stretchPointsGreen, stretchPointsBlue,
      pRasterElement1, pRasterElement2, pRasterElement3, pBadValues1, pBadValues2, pBadValues3);
}

template<class T>
int roundOffTarget(T target)
{
   return static_cast<int>(target);
}

int roundOffTarget(float target)
{
   double dValue = static_cast<double> (target);
   return roundDouble(dValue);
}

int roundOffTarget(double target)
{
   return roundDouble(target);
}

template<typename In>
float getScale()
{
   return 1.0f;
}

template<>
float getScale<unsigned char>()
{
   return static_cast<float>(numeric_limits<unsigned char>::max());
}

template<>
float getScale<signed char>()
{
   return static_cast<float>(numeric_limits<unsigned char>::max());
}

template<>
float getScale<unsigned short>()
{
   return static_cast<float>(numeric_limits<unsigned short>::max());
}

template<>
float getScale<signed short>()
{
   return static_cast<float>(numeric_limits<unsigned short>::max());
}

template<typename In>
In getOffset()
{
   return 0;
}

template<>
signed char getOffset<signed char>()
{
   return numeric_limits<signed char>::min();
}

template<>
signed short getOffset<signed short>()
{
   return numeric_limits<signed short>::min();
}

template<typename In>
In getFromSource(In src)
{
   return src - getOffset<In>();
}

template<typename Out>
Out getFromSource(EncodingType encoding, double source)
{
   if (encoding == INT1SBYTE)
   {
      signed char value = static_cast<signed char>(source);
      return static_cast<Out>(value - numeric_limits<signed char>::min());
   }
   else if (encoding == INT2SBYTES)
   {
      signed short value = static_cast<signed short>(source);
      return static_cast<Out>(value - numeric_limits<signed short>::min());
   }

   return static_cast<Out>(source);
}

template<typename In, typename Out>
void copyLineFromSource(In* pSource, Out* pTarget, unsigned int count)
{
   memcpy(pTarget, pSource, count*sizeof(Out));
}

template<>
void copyLineFromSource<signed char, unsigned char>(signed char* pSource, unsigned char* pTarget, unsigned int count)
{
   for (unsigned int i=0; i<count; ++i)
   {
      *pTarget = getFromSource(*pSource);
      ++pTarget;
      ++pSource;
   }
}

template<>
void copyLineFromSource<signed short, unsigned short>(signed short* pSource, unsigned short* pTarget, unsigned int count)
{
   for (unsigned int i=0; i<count; ++i)
   {
      *pTarget = getFromSource(*pSource);
      ++pTarget;
      ++pSource;
   }
}

template<>
void copyLineFromSource<signed int, float>(signed int* pSource, float* pTarget, unsigned int count)
{
   for (unsigned int i=0; i<count; ++i)
   {
      *pTarget = static_cast<float>(getFromSource(*pSource));
      ++pTarget;
      ++pSource;
   }
}

template<>
void copyLineFromSource<unsigned int, float>(unsigned int* pSource, float* pTarget, unsigned int count)
{
   for (unsigned int i=0; i<count; ++i)
   {
      *pTarget = static_cast<float>(getFromSource(*pSource));
      ++pTarget;
      ++pSource;
   }
}

template<>
void copyLineFromSource<double, float>(double* pSource, float* pTarget, unsigned int count)
{
   for (unsigned int i=0; i<count; ++i)
   {
      *pTarget = static_cast<float>(getFromSource(*pSource));
      ++pTarget;
      ++pSource;
   }
}

template<typename T>
T getOpenGlMax()
{
   return numeric_limits<T>::max();
}

template<>
float getOpenGlMax<float>()
{
   return 1.0f;
}

template<>
double getOpenGlMax<double>()
{
   return 1.0;
}

class GpuTileProcessor
{
public:
   GpuTileProcessor(const vector<GpuTile*>& tiles,
      vector<unsigned int>& tileZoomIndices, const Image::ImageData& info) :
      mTiles(tiles),
      mTileZoomIndices(tileZoomIndices),
      mInfo(info)
   {}

   void run();

private:
   const vector<GpuTile*>& mTiles;
   vector<unsigned int>& mTileZoomIndices;
   const Image::ImageData& mInfo;

   GpuTileProcessor& operator=(const GpuTileProcessor& rhs);

   template <typename In, typename Out>
   void populateTextureData(Out* pTexData, unsigned int tileSizeX, unsigned int tileSizeY,
      DataAccessor da, InterleaveFormatType interleave, int currentChannel, int totalChannels, 
      EncodingType outputType)
   {
      bool bBadValues = (mInfo.mKey.mpBadValues1 != NULL && mInfo.mKey.mpBadValues1->empty() == false);

      Out* pTargetBase = pTexData;

      int channelMinus1 = currentChannel - 1;    // Subtract one since currentChannel is a one-based value
      int channelStep = totalChannels - currentChannel;
      if (bBadValues)
      {
         double singleRangeLow;
         double singleRangeHigh;
         bool hasSingleBadValueRange = mInfo.mKey.mpBadValues1->getSingleBadValueRange(singleRangeLow,
            singleRangeHigh);
         for (unsigned int y1 = 0; y1 < tileSizeY; y1++, pTargetBase += (mInfo.mTileSizeX * totalChannels))
         {
            Out* pTarget = pTargetBase;
            pTarget += channelMinus1;
            for (unsigned int x1 = 0; x1 < tileSizeX; x1++)
            {
               In source = *static_cast<In*>(da->getColumn());
               *pTarget = static_cast<Out>(getFromSource(source));

               pTarget += channelStep;
               bool isBadValue = false;
               double dSource = static_cast<double>(source);
               if (hasSingleBadValueRange == true)
               {
                  isBadValue = dSource > singleRangeLow && dSource < singleRangeHigh;
               }
               else
               {
                  isBadValue = mInfo.mKey.mpBadValues1->isBadValue(dSource);
               }

               if (isBadValue)
               {
                  *pTarget = static_cast<Out> (0);
               }
               else
               {
                  *pTarget = getOpenGlMax<Out>();
               }

               ++pTarget;
               da->nextColumn();
            }
            da->nextRow();
         }
      }
      else
      {
         for (unsigned int y1 = 0; y1 < tileSizeY; y1++, pTargetBase += (mInfo.mTileSizeX * totalChannels))
         {
            Out* pTarget = pTargetBase;
            pTarget += channelMinus1;
            if (interleave != BIP && totalChannels == 1)
            {
               unsigned int width = min(tileSizeX, static_cast<unsigned int>(mInfo.mTileSizeX));
               copyLineFromSource(static_cast<In*>(da->getRow()), pTarget, width);
            }
            else
            {
               for (unsigned int x1 = 0; x1 < tileSizeX; x1++)
               {
                  *pTarget = static_cast<Out>(getFromSource(*static_cast<In*>(da->getColumn())));

                  if (totalChannels == 2)
                  {
                     ++pTarget;
                     *pTarget = getOpenGlMax<Out>();
                     ++pTarget;
                  }
                  else if (static_cast<int>(x1) < mInfo.mTileSizeX - 1)
                  {
                     pTarget += totalChannels;
                  }
                  da->nextColumn();
               }
            }
            da->nextRow();
         }
      }
   }

   template <typename Out>
   void populateTextureData(Out* pTexData, EncodingType inputType, unsigned int tileSizeX,
      unsigned int tileSizeY, DataAccessor da, InterleaveFormatType interleave, 
      int currentChannel, int totalChannels, EncodingType outputType)
   {
      switch (inputType)
      {
         case INT1UBYTE:
            populateTextureData<unsigned char>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case INT1SBYTE:
            populateTextureData<signed char>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case INT2UBYTES:
            populateTextureData<unsigned short>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case INT2SBYTES:
            populateTextureData<signed short>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case INT4UBYTES:
            populateTextureData<unsigned int>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case INT4SBYTES:
            populateTextureData<signed int>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case FLT4BYTES:
            populateTextureData<float>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case FLT8BYTES:
            populateTextureData<double>(pTexData, tileSizeX, tileSizeY, da, interleave,
               currentChannel, totalChannels, outputType);
            break;

         case INT4SCOMPLEX:   // Fall through to the next case statement
         case FLT8COMPLEX:    // Fall through to the next case statement
         default:
            break;
      }
   }

   template <typename In, typename Out>
   void populateEmptyTextureData(vector<Out>& texData, unsigned int tileSizeX, unsigned int tileSizeY,
      int currentChannel, int totalChannels, EncodingType outputType)
   {
      bool bBadValues = (mInfo.mKey.mpBadValues1 != NULL && mInfo.mKey.mpBadValues1->empty() == false);

      typename vector<Out>::iterator targetBase = texData.begin();
      for (unsigned int y1 = 0; y1 < tileSizeY; y1++, targetBase += (mInfo.mTileSizeX * totalChannels))
      {
         typename vector<Out>::iterator target = targetBase;
         target += (currentChannel - 1);     // Subtract one since currentChannel is a one-based value

         for (unsigned int x1 = 0; x1 < tileSizeX; x1++)
         {
            *target = static_cast<Out> (0);

            if (bBadValues)
            {
               target += (totalChannels - currentChannel);
               *target = static_cast<Out> (0);
               ++target;
            }
            else
            {
               if (totalChannels == 2)
               {
                  ++target;
                  *target = static_cast<Out> (0);
                  ++target;
               }
               else if (static_cast<int>(x1) < mInfo.mTileSizeX - 1)
               {
                  target += totalChannels;
               }
            }
         }
      }
   }

   template <typename Out>
   void populateEmptyTextureData(vector<Out>& texData, EncodingType inputType, unsigned int tileSizeX,
      unsigned int tileSizeY, int currentChannel, int totalChannels, EncodingType outputType)
   {
      switch (inputType)
      {
         case INT1UBYTE:
            populateEmptyTextureData<unsigned char>(texData, tileSizeX, tileSizeY,
               currentChannel, totalChannels, outputType);
            break;

         case INT1SBYTE:
            populateEmptyTextureData<signed char>(texData, tileSizeX, tileSizeY,
               currentChannel, totalChannels, outputType);
            break;

         case INT2UBYTES:
            populateEmptyTextureData<unsigned short>(texData, tileSizeX, tileSizeY,
               currentChannel, totalChannels, outputType);
            break;

         case INT2SBYTES:
            populateEmptyTextureData<signed short>(texData, tileSizeX, tileSizeY, 
               currentChannel, totalChannels, outputType);
            break;

         case INT4UBYTES:
            populateEmptyTextureData<unsigned int>(texData, tileSizeX, tileSizeY,
               currentChannel, totalChannels, outputType);
            break;

         case INT4SBYTES:
            populateEmptyTextureData<signed int>(texData, tileSizeX, tileSizeY, 
               currentChannel, totalChannels, outputType);
            break;

         case FLT4BYTES:
            populateEmptyTextureData<float>(texData, tileSizeX, tileSizeY, 
               currentChannel, totalChannels, outputType);
            break;

         case FLT8BYTES:
            populateEmptyTextureData<double>(texData, tileSizeX, tileSizeY,
               currentChannel, totalChannels, outputType);
            break;

         case INT4SCOMPLEX:   // Fall through to the next case statement
         case FLT8COMPLEX:    // Fall through to the next case statement
         default:
            break;
      }
   }

   template <typename Out>
   void createGrayscale(EncodingType outputType)
   {
      int bufSize = mInfo.mTileSizeX * mInfo.mTileSizeY;
      int channels = 1;

      if (mInfo.mFormat == GL_LUMINANCE_ALPHA)
      {
         bufSize *= 2;
         channels++;
      }

      for (unsigned int i = 0; i < mTiles.size(); ++i)
      {
         GpuTile* pTile = mTiles[i];
         if (pTile != NULL)
         {
            unsigned int posX = static_cast<unsigned int>(pTile->getPos().mX);
            unsigned int posY = static_cast<unsigned int>(pTile->getPos().mY);
            unsigned int geomSizeX = static_cast<unsigned int>(pTile->getGeomSize().mX);
            unsigned int geomSizeY = static_cast<unsigned int>(pTile->getGeomSize().mY);
            RasterElement* pRasterElement = mInfo.mKey.mpRasterElement[0];
            VERIFYNRV(pRasterElement != NULL);

            const RasterDataDescriptor *pDescriptor = dynamic_cast<const RasterDataDescriptor*>(
               pRasterElement->getDataDescriptor());
            VERIFYNRV(pDescriptor != NULL);

            FactoryResource<DataRequest> pRequest;
            pRequest->setRows(pDescriptor->getActiveRow(posY), 
               pDescriptor->getActiveRow(posY+geomSizeY-1), geomSizeY);
            pRequest->setColumns(pDescriptor->getActiveColumn(posX), 
               pDescriptor->getActiveColumn(posX+geomSizeX-1), geomSizeX);
            pRequest->setBands(mInfo.mKey.mBand1,
               mInfo.mKey.mBand1, 1);
            
            DataAccessor da = pRasterElement->getDataAccessor(pRequest.release());
            if (!da.isValid())
            {
               return;
            }

            Out *pTexData = static_cast<Out*>(pTile->getTexData(bufSize * sizeof(Out)));
            populateTextureData(pTexData, mInfo.mRawType[0], geomSizeX, geomSizeY, da, 
               pDescriptor->getInterleaveFormat(), 1, channels, outputType);
            pTile->setupTile(pTexData, outputType, mTileZoomIndices[i]);
         }
      }
   }

   template <typename Out>
   void createRgb(EncodingType outputType)
   {
      bool hasRedBadValues = (mInfo.mKey.mpBadValues1 != NULL && mInfo.mKey.mpBadValues1->empty() == false);
      double singleRangeLowRed = 0.0;
      double singleRangeHighRed = 0.0;
      bool hasSingleRedBadValueRange = false;
      if (hasRedBadValues)
      {
         hasSingleRedBadValueRange = mInfo.mKey.mpBadValues1->getSingleBadValueRange(singleRangeLowRed,
            singleRangeHighRed);
      }

      bool hasGreenBadValues = (mInfo.mKey.mpBadValues2 != NULL && mInfo.mKey.mpBadValues2->empty() == false);
      double singleRangeLowGreen = 0.0;
      double singleRangeHighGreen = 0.0;
      bool hasSingleGreenBadValueRange = false;
      if (hasGreenBadValues)
      {
         hasSingleGreenBadValueRange = mInfo.mKey.mpBadValues2->getSingleBadValueRange(singleRangeLowGreen,
            singleRangeHighGreen);
      }

      bool hasBlueBadValues = (mInfo.mKey.mpBadValues3 != NULL && mInfo.mKey.mpBadValues3->empty() == false);
      double singleRangeLowBlue = 0.0;
      double singleRangeHighBlue = 0.0;
      bool hasSingleBlueBadValueRange = false;
      if (hasBlueBadValues)
      {
         hasSingleBlueBadValueRange = mInfo.mKey.mpBadValues3->getSingleBadValueRange(singleRangeLowBlue,
            singleRangeHighBlue);
      }

      bool hasBadValues = hasRedBadValues || hasGreenBadValues || hasBlueBadValues;

      int totalChannels = 3;
      if (mInfo.mFormat == GL_RGBA)
      {
         totalChannels = 4;
      }

      int bufSize = mInfo.mTileSizeX * mInfo.mTileSizeY * sizeof(unsigned char) * totalChannels;
      vector<Out> texData(bufSize);

      for (unsigned int i = 0; i < mTiles.size(); ++i)
      {
         GpuTile* pTile = mTiles[i];
         if (pTile != NULL)
         {
            // Create a data accessor for each band
            unsigned int posX = static_cast<unsigned int>(pTile->getPos().mX);
            unsigned int posY = static_cast<unsigned int>(pTile->getPos().mY);
            unsigned int geomSizeX = static_cast<unsigned int>(pTile->getGeomSize().mX);
            unsigned int geomSizeY = static_cast<unsigned int>(pTile->getGeomSize().mY);

            RasterElement* pRedRasterElement = mInfo.mKey.mpRasterElement[0];
            DimensionDescriptor redBand = mInfo.mKey.mBand1;
            bool haveRedData = ((pRedRasterElement != NULL) && (redBand.isActiveNumberValid()));
            DataAccessor daRed(NULL, NULL);
            if (haveRedData)
            {
               RasterDataDescriptor* pRedRasterDescriptor =
                  dynamic_cast<RasterDataDescriptor*>(pRedRasterElement->getDataDescriptor());
               VERIFYNRV(pRedRasterDescriptor != NULL);

               FactoryResource<DataRequest> pRedRequest;
               pRedRequest->setRows(pRedRasterDescriptor->getActiveRow(posY),
                  pRedRasterDescriptor->getActiveRow(posY + geomSizeY - 1), geomSizeY);
               pRedRequest->setColumns(pRedRasterDescriptor->getActiveColumn(posX),
                  pRedRasterDescriptor->getActiveColumn(posX + geomSizeX - 1), geomSizeX);
               pRedRequest->setBands(mInfo.mKey.mBand1, mInfo.mKey.mBand1, 1);

               daRed = pRedRasterElement->getDataAccessor(pRedRequest.release());
               if (!daRed.isValid())
               {
                  return;
               }
            }

            RasterElement* pGreenRasterElement = mInfo.mKey.mpRasterElement[1];
            DimensionDescriptor greenBand = mInfo.mKey.mBand2;
            bool haveGreenData = ((pGreenRasterElement != NULL) && (greenBand.isActiveNumberValid()));
            DataAccessor daGreen(NULL, NULL);
            if (haveGreenData)
            {
               RasterDataDescriptor* pGreenRasterDescriptor =
                  dynamic_cast<RasterDataDescriptor*>(pGreenRasterElement->getDataDescriptor());
               VERIFYNRV(pGreenRasterDescriptor != NULL);

               FactoryResource<DataRequest> pGreenRequest;
               pGreenRequest->setRows(pGreenRasterDescriptor->getActiveRow(posY),
                  pGreenRasterDescriptor->getActiveRow(posY + geomSizeY - 1), geomSizeY);
               pGreenRequest->setColumns(pGreenRasterDescriptor->getActiveColumn(posX),
                  pGreenRasterDescriptor->getActiveColumn(posX + geomSizeX - 1), geomSizeX);
               pGreenRequest->setBands(mInfo.mKey.mBand2, mInfo.mKey.mBand2, 1);

               daGreen = pGreenRasterElement->getDataAccessor(pGreenRequest.release());
               if (!daGreen.isValid())
               {
                  return;
               }
            }

            RasterElement* pBlueRasterElement = mInfo.mKey.mpRasterElement[2];
            DimensionDescriptor blueBand = mInfo.mKey.mBand3;
            bool haveBlueData = ((pBlueRasterElement != NULL) && (blueBand.isActiveNumberValid()));
            DataAccessor daBlue(NULL, NULL);
            if (haveBlueData)
            {
               RasterDataDescriptor* pBlueRasterDescriptor =
                  dynamic_cast<RasterDataDescriptor*>(pBlueRasterElement->getDataDescriptor());
               VERIFYNRV(pBlueRasterDescriptor != NULL);

               FactoryResource<DataRequest> pBlueRequest;
               pBlueRequest->setRows(pBlueRasterDescriptor->getActiveRow(posY),
                  pBlueRasterDescriptor->getActiveRow(posY + geomSizeY - 1), geomSizeY);
               pBlueRequest->setColumns(pBlueRasterDescriptor->getActiveColumn(posX),
                  pBlueRasterDescriptor->getActiveColumn(posX + geomSizeX - 1), geomSizeX);
               pBlueRequest->setBands(mInfo.mKey.mBand3, mInfo.mKey.mBand3, 1);

               daBlue = pBlueRasterElement->getDataAccessor(pBlueRequest.release());
               if (!daBlue.isValid())
               {
                  return;
               }
            }

            Out* pTargetBase = &texData.front();
            for (unsigned int y1 = 0; y1 < geomSizeY; y1++, pTargetBase += (mInfo.mTileSizeX * totalChannels))
            {
               Out* pTarget = pTargetBase;
               for (unsigned int x1 = 0; x1 < geomSizeX; x1++)
               {
                  // Red
                  bool isRedValueBad = true;
                  if (haveRedData)
                  {
                     VERIFYNRV(daRed.isValid());
                     void* pSource = daRed->getColumn();
                     double source = ModelServices::getDataValue(mInfo.mRawType[0], pSource, mInfo.mKey.mComponent, 0);
                     *pTarget = getFromSource<Out>(mInfo.mRawType[0], source);

                     if (hasRedBadValues)
                     {
                        if (hasSingleRedBadValueRange)
                        {
                           isRedValueBad = source > singleRangeLowRed && source < singleRangeHighRed;
                        }
                        else
                        {
                           isRedValueBad = mInfo.mKey.mpBadValues1->isBadValue(source);
                        }
                     }
                     else
                     {
                        isRedValueBad = false;
                     }

                     daRed->nextColumn();
                  }

                  if (isRedValueBad == true)
                  {
                     *pTarget = static_cast<Out>(0);
                  }

                  ++pTarget;

                  // Green
                  bool isGreenValueBad = true;
                  if (haveGreenData)
                  {
                     VERIFYNRV(daGreen.isValid());
                     void* pSource = daGreen->getColumn();
                     double source = ModelServices::getDataValue(mInfo.mRawType[1], pSource, mInfo.mKey.mComponent, 0);
                     *pTarget = getFromSource<Out>(mInfo.mRawType[1], source);

                     if (hasGreenBadValues)
                     {
                        if (hasSingleGreenBadValueRange)
                        {
                           isGreenValueBad = source > singleRangeLowGreen && source < singleRangeHighGreen;
                        }
                        else
                        {
                           isGreenValueBad = mInfo.mKey.mpBadValues2->isBadValue(source);
                        }
                     }
                     else
                     {
                        isGreenValueBad = false;
                     }

                     daGreen->nextColumn();
                  }

                  if (isGreenValueBad == true)
                  {
                     *pTarget = static_cast<Out>(0);
                  }

                  ++pTarget;

                  // Blue
                  bool isBlueValueBad = true;
                  if (haveBlueData)
                  {
                     VERIFYNRV(daBlue.isValid());
                     void* pSource = daBlue->getColumn();
                     double source = ModelServices::getDataValue(mInfo.mRawType[2], pSource, mInfo.mKey.mComponent, 0);
                     *pTarget = getFromSource<Out>(mInfo.mRawType[2], source);

                     if (hasBlueBadValues)
                     {
                        if (hasSingleBlueBadValueRange)
                        {
                           isBlueValueBad = source > singleRangeLowBlue && source < singleRangeHighBlue;
                        }
                        else
                        {
                           isBlueValueBad = mInfo.mKey.mpBadValues3->isBadValue(source);
                        }
                     }
                     else
                     {
                        isBlueValueBad = false;
                     }

                     daBlue->nextColumn();
                  }

                  if (isBlueValueBad == true)
                  {
                     *pTarget = static_cast<Out>(0);
                  }

                  ++pTarget;

                  // Bad values
                  if (hasBadValues == true)
                  {
                     if ((isRedValueBad == true) && (isGreenValueBad == true) && (isBlueValueBad == true))
                     {
                        *pTarget = static_cast<Out>(0);
                     }
                     else
                     {
                        *pTarget = getOpenGlMax<Out>();
                     }

                     ++pTarget;
                  }
                  else if (mInfo.mFormat == GL_RGBA)
                  {
                     *pTarget = getOpenGlMax<Out>();
                     ++pTarget;
                  }
               }

               if (daRed.isValid() == true)
               {
                  daRed->nextRow();
               }

               if (daGreen.isValid() == true)
               {
                  daGreen->nextRow();
               }

               if (daBlue.isValid() == true)
               {
                  daBlue->nextRow();
               }
            }

            pTile->setupTile(&texData.front(), outputType, mTileZoomIndices[i]);
         }
      }
   }
};    // End of GpuTileProcessor class declaration

void GpuImage::updateTiles(vector<Tile*>& tilesToUpdate, vector<unsigned int>& tileZoomIndices)
{
   vector<GpuTile*> tiles;
   for (unsigned int i = 0; i < tilesToUpdate.size(); ++i)
   {
      GpuTile* pTile = dynamic_cast<GpuTile*> (tilesToUpdate[i]);
      if (pTile != NULL)
      {
         tiles.push_back(pTile);
      }
   }

   const ImageData& info = getImageData();
   GpuTileProcessor processor(tiles, tileZoomIndices, info);
   processor.run();
}

void GpuTileProcessor::run()
{
   if (mInfo.mKey.mStretchPoints2.empty() == true)    // Grayscale or colormap
   {
      if ((mInfo.mRawType[0] == INT1UBYTE) || (mInfo.mRawType[0] == INT1SBYTE))
      {
         createGrayscale<unsigned char>(INT1UBYTE);
      }
      else if ((mInfo.mRawType[0] == INT2UBYTES) || (mInfo.mRawType[0] == INT2SBYTES))
      {
         createGrayscale<unsigned short>(INT2UBYTES);
      }
      else if ((mInfo.mRawType[0] == INT4UBYTES) || (mInfo.mRawType[0] == INT4SBYTES) ||
         (mInfo.mRawType[0] == FLT4BYTES) || (mInfo.mRawType[0] == FLT8BYTES))
      {
         createGrayscale<float>(FLT4BYTES);
      }
   }
   else     // RGB
   {
      if ((mInfo.mRawType[0] == mInfo.mRawType[1]) && (mInfo.mRawType[0] == mInfo.mRawType[2]))
      {
         if ((mInfo.mRawType[0] == INT1UBYTE) || (mInfo.mRawType[0] == INT1SBYTE))
         {
            createRgb<unsigned char>(INT1UBYTE);
         }
         else if ((mInfo.mRawType[0] == INT2UBYTES) || (mInfo.mRawType[0] == INT2SBYTES))
         {
            createRgb<unsigned short>(INT2UBYTES);
         }
         else if ((mInfo.mRawType[0] == INT4UBYTES) || (mInfo.mRawType[0] == INT4SBYTES) ||
            (mInfo.mRawType[0] == FLT4BYTES) || (mInfo.mRawType[0] == FLT8BYTES))
         {
            createRgb<float>(FLT4BYTES);
         }
      }
      else
      {
         createRgb<float>(FLT4BYTES);
      }
   }
}

void GpuImage::enableFilter(ImageFilterDescriptor *pDescriptor)
{
   if (isFilterEnabled(pDescriptor) == true)
   {
      return;
   }

   initializeFilter(pDescriptor);
}

void GpuImage::initializeFilter(ImageFilterDescriptor *pDescriptor)
{
   VERIFYNRV(pDescriptor != NULL);

   const map<ImageKey, TileSet>& tileSets = getTileSets();
   for (map<ImageKey, TileSet>::const_iterator iter = tileSets.begin(); iter != tileSets.end(); ++iter)
   {
      const vector<Tile*>& tiles = iter->second.getTiles();

      unsigned int numTiles = tiles.size();
      vector<unsigned int> tileZoomIndices;
      vector<Tile*> tilesToUpdate;

      tileZoomIndices.reserve(numTiles);
      tilesToUpdate.reserve(numTiles);

      for (vector<Tile*>::const_iterator tileIter = tiles.begin(); tileIter != tiles.end(); ++tileIter)
      {
         GpuTile* pTile = dynamic_cast<GpuTile*>(*tileIter);
         if (pTile != NULL)
         {
            pTile->createFilter(pDescriptor);
            tilesToUpdate.push_back(pTile);
            tileZoomIndices.push_back(pTile->getTextureIndex());
         }
      }

      updateTiles(tilesToUpdate, tileZoomIndices);

      // Initialize tile filters
      if (pDescriptor->getType() == ImageFilterDescriptor::FEEDBACK_FILTER)
      {
         // number of iterations to use to initialize the feedback buffer
         // filters can override this value through the gic file
         unsigned int numInitFrames = 20;

         const unsigned int* pInitFrames = dv_cast<unsigned int>(&pDescriptor->getParameter("initializationIterations"));
         if (pInitFrames != NULL)
         {
            numInitFrames = *pInitFrames;
         }

         for (unsigned int j = 0; j < tilesToUpdate.size(); ++j)
         {
            GpuTile* pGpuTile = static_cast<GpuTile*>(tilesToUpdate[j]);
            bool freeze = pGpuTile->getFilterFreezeFlag(pDescriptor);
            pGpuTile->freezeFilter(pDescriptor, false);
            for (unsigned int i = 0; i < numInitFrames; ++i)
            {
               // Apply the filter repeatedly on the current frame on each tile
               // to initialize the feedback buffer. This all happens in the GPU.
               pGpuTile->applyFilters();
            }
            pGpuTile->freezeFilter(pDescriptor, freeze);
         }
      }
   }

   // Set the member variable, mPreviousBand, to 0 to allow the filter to be applied
   // to the image when it is enabled
   mPreviousBand = 0;
}

void GpuImage::enableFilters(const vector<ImageFilterDescriptor*>& descriptors)
{
   const vector<Tile*>* pTiles = getActiveTiles();
   if (pTiles == NULL)
   {
      return;
   }

   if (pTiles->empty() == true)
   {
      return;
   }

   // All tiles have the same filters so just get the current filters from the first tile
   vector<ImageFilterDescriptor*> currentFilters;
   vector<ImageFilterDescriptor*> newFilters = descriptors;

   const GpuTile* pTile = dynamic_cast<const GpuTile*>(pTiles->front());
   if (pTile != NULL)
   {
      currentFilters = pTile->getFilters();
   }

   // Delete currently enabled filters that are not in the new enabled vector
   vector<ImageFilterDescriptor*>::iterator currentIter;
   for (currentIter = currentFilters.begin(); currentIter != currentFilters.end(); ++currentIter)
   {
      ImageFilterDescriptor* pDescriptor = *currentIter;
      if (pDescriptor != NULL)
      {
         vector<ImageFilterDescriptor*>::iterator newIter = find(newFilters.begin(), newFilters.end(), pDescriptor);
         if (newIter == newFilters.end())
         {
            disableFilter(pDescriptor);
         }
         else
         {
            // Remove the filter from the vector of filters to enable
            newFilters.erase(newIter);
         }
      }
   }

   // Enable the remaining image filters
   vector<ImageFilterDescriptor*>::iterator newIter;
   for (newIter = newFilters.begin(); newIter != newFilters.end(); ++newIter)
   {
      ImageFilterDescriptor* pDescriptor = *newIter;
      if (pDescriptor != NULL)
      {
         enableFilter(pDescriptor);
      }
   }
}

void GpuImage::resetFilter(ImageFilterDescriptor *pDescriptor)
{
   if (!isFilterEnabled(pDescriptor))
   {
      return;
   }

   const map<ImageKey, TileSet>& tileSets = getTileSets();
   for (map<ImageKey, TileSet>::const_iterator iter = tileSets.begin(); iter != tileSets.end(); ++iter)
   {
      const vector<Tile*>& tiles = iter->second.getTiles();

      vector<Tile*>::const_iterator tileIter = tiles.begin();
      while (tileIter != tiles.end())
      {
         GpuTile* pTile = dynamic_cast<GpuTile*>(*tileIter);
         if (pTile != NULL)
         {
            pTile->resetFilter(pDescriptor);
         }
         ++tileIter;
      }
   }

   initializeFilter(pDescriptor);
}

void GpuImage::freezeFilter(ImageFilterDescriptor *pDescriptor, bool toggle)
{
   if (!isFilterEnabled(pDescriptor))
   {
      return;
   }

   const map<ImageKey, TileSet>& tileSets = getTileSets();
   for (map<ImageKey, TileSet>::const_iterator iter = tileSets.begin(); iter != tileSets.end(); ++iter)
   {
      const vector<Tile*>& tiles = iter->second.getTiles();

      vector<Tile*>::const_iterator tileIter = tiles.begin();
      while (tileIter != tiles.end())
      {
         GpuTile* pTile = dynamic_cast<GpuTile*>(*tileIter);
         if (pTile != NULL)
         {
            pTile->freezeFilter(pDescriptor, toggle);
         }
         ++tileIter;
      }
   }
}

void GpuImage::disableFilter(ImageFilterDescriptor *pDescriptor)
{
   if (isFilterEnabled(pDescriptor) == false)
   {
      return;
   }

   const map<ImageKey, TileSet>& tileSets = getTileSets();
   for (map<ImageKey, TileSet>::const_iterator iter = tileSets.begin(); iter != tileSets.end(); ++iter)
   {
      const vector<Tile*>& tiles = iter->second.getTiles();
      for (vector<Tile*>::const_iterator tileIter = tiles.begin(); tileIter != tiles.end(); ++tileIter)
      {
         GpuTile* pTile = dynamic_cast<GpuTile*>(*tileIter);
         if (pTile != NULL)
         {
            pTile->destroyFilter(pDescriptor);
         }
      }
   }
}

bool GpuImage::isFilterEnabled(ImageFilterDescriptor *pDescriptor) const
{
   const vector<Tile*>* pTiles = getActiveTiles();
   if (pTiles == NULL)
   {
      return false;
   }

   if (pTiles->empty() == true)
   {
      return false;
   }

   unsigned int numTiles = pTiles->size();
   for (unsigned int i = 0; i < numTiles; ++i)
   {
      GpuTile* pTile = dynamic_cast<GpuTile*> (pTiles->at(i));
      if (pTile != NULL)
      {
         bool bEnabled = pTile->hasFilter(pDescriptor);
         if (bEnabled == false)
         {
            return false;
         }
      }
   }

   return true;
}

void GpuImage::initializeGrayscale()
{
   GlSlContext* pGlSlontext = GlSlContext::instance();

   if (mGpuProgramObject != mGrayscaleProgram.getProgramObjectId())
   {
      mGrayscaleProgram.initialize();
      mGpuProgramObject = mGrayscaleProgram.getProgramObjectId();
   }
}

void GpuImage::initializeColormap(const vector<ColorType>& colorMap)
{
   GlSlContext* pGlSlontext = GlSlContext::instance();
   if (mGpuProgramObject != mColormapProgram.getProgramObjectId())
   {
      mColormapProgram.initialize();
      mGpuProgramObject = mColormapProgram.getProgramObjectId();
   }
   if (mColorMapChanged || static_cast<GLuint>(mColormapTexture) == 0)
   {
      unsigned int numColors = colorMap.size();
      vector<unsigned char> colors(numColors * 4);
      for (unsigned int i = 0, textureIndex = 0; i < numColors; ++i)
      {
         ColorType color = colorMap[i];
         colors[textureIndex++] = color.mRed;
         colors[textureIndex++] = color.mGreen;
         colors[textureIndex++] = color.mBlue;
         colors[textureIndex++] = color.mAlpha;
      }

      mColormapTexture = GlTextureResource(1);
      if (static_cast<GLuint>(mColormapTexture) != 0)
      {
         glBindTexture(GL_TEXTURE_RECTANGLE_ARB, mColormapTexture);
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         //glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, numColors, 1, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &colors[0]);
         glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, numColors, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &colors[0]);
         glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
      }
   }
}

void GpuImage::initializeRgb()
{
   GlSlContext* pGlSlontext = GlSlContext::instance();
   if (mGpuProgramObject != mRgbProgram.getProgramObjectId())
   {
      mRgbProgram.initialize();
      mGpuProgramObject = mRgbProgram.getProgramObjectId();
   }
}

void GpuImage::setMaxTextureSize(GLint maxSize)
{
   mMaxTextureSize = 2048;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);
   if (maxSize > 0 && maxSize < mMaxTextureSize)
   {
      mMaxTextureSize = maxSize;
   }
}

GLint GpuImage::getMaxTextureSize()
{
   if (mMaxTextureSize == 0)
   {
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);
   }
   return mMaxTextureSize;
}

void GpuImage::calculateTileSize(EncodingType dataType, const unsigned int imageWidth, const unsigned int imageHeight,
                                 int& tileWidth, int& tileHeight) const
{
   // Initially the tile size equal to the image size
   tileWidth = imageWidth;
   tileHeight = imageHeight;

   // Retrieve the current maximum texture size allowed
   GLint maxTextureSize = getMaxTextureSize();

   // Update the maximum texture size to include the internal data format
   unsigned int bytesPerElement = RasterUtilities::bytesInEncoding(dataType);
   if (bytesPerElement == 0)
   {
      bytesPerElement = 1;
   }

   maxTextureSize = maxTextureSize / bytesPerElement;

   // Compute tile height
   while (tileHeight > maxTextureSize)
   {
      if (tileHeight % 2 == 1)
      {
         tileHeight = (tileHeight + 1) / 2;
      }
      else
      {
         tileHeight /= 2;
      }
   }

   // Compute tile width
   while (tileWidth > maxTextureSize)
   {
      if (tileWidth % 2 == 1)
      {
         tileWidth = (tileWidth + 1) / 2;
      }
      else
      {
         tileWidth /= 2;
      }
   }
}

Tile* GpuImage::createTile() const
{
   return (new GpuTile());
}

void GpuImage::drawTiles(const vector<Tile*>& tiles, GLfloat textureMode)
{
   if (tiles.empty() == true)
   {
      return;
   }

   GlShaderGpuProgram* pProgram = NULL;
   if (mGpuProgramObject == mGrayscaleProgram.getProgramObjectId()) {
      pProgram = &mGrayscaleProgram;
   }
   else if (mGpuProgramObject == mColormapProgram.getProgramObjectId()) {
      pProgram = &mColormapProgram;
   }
   else if (mGpuProgramObject == mRgbProgram.getProgramObjectId()) {
      pProgram = &mRgbProgram;
   }
   else
      return;
   
   // Enable the fragment profile and bind the display program
   const ImageData& imageInfo = getImageData();

   pProgram->bind();
   pProgram->setParameterValues(imageInfo, getAlpha(), mColormapTexture);

   // Draw the tiles
   for (vector<Tile*>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter)
   {
      GpuTile* pTile = dynamic_cast<GpuTile*>(*iter);
      if (pTile != NULL)
      {
         pTile->draw(pProgram, textureMode);
      }
   }

   // Disable the profile
   pProgram->disable();
}

float GpuImage::getTextureStretchValue(float rawValue, EncodingType dataType) const
{
   const ImageData& imageInfo = getImageData();

   float stretchValue = rawValue;
   if ((imageInfo.mRawType[0] == imageInfo.mRawType[1]) && (imageInfo.mRawType[0] == imageInfo.mRawType[2]))
   {
      if (dataType == INT1SBYTE)
      {
         stretchValue -= getOffset<signed char>();
      }
      else if (dataType == INT2SBYTES)
      {
         stretchValue -= getOffset<signed short>();
      }
   }

   return stretchValue;
}

void GpuImage::setActiveTileSet(const ImageKey &key)
{
   ImageKey defaultImageKey;

   if (mGpuProgramObject == mGrayscaleProgram.getProgramObjectId())
   {
      if (mGrayscaleImageKey == defaultImageKey)
      {
         mGrayscaleImageKey = key;
      }

      Image::setActiveTileSet(mGrayscaleImageKey);
   }
   else if (mGpuProgramObject == mColormapProgram.getProgramObjectId())
   {
      if (mColormapImageKey == defaultImageKey)
      {
         mColormapImageKey = key;
      }

      Image::setActiveTileSet(mColormapImageKey);
   }
   else if (mGpuProgramObject == mRgbProgram.getProgramObjectId())
   {
      if (mRgbImageKey == defaultImageKey)
      {
         mRgbImageKey = key;
      }

      Image::setActiveTileSet(mRgbImageKey);
   }
}

unsigned int GpuImage::getMaxNumTileSets() const
{
   // One tile set for grayscale, one for RGB, and one for the color map
   return 3;
}

vector<Tile*> GpuImage::getTilesToUpdate(const vector<Tile*>& tilesToDraw, vector<unsigned int>& tileZoomIndices)
{
   const Image::ImageData imageInfo = Image::getImageData();

   // check for a change in the band number by comparing a one-based band
   // number to allow for an invalid default value of zero
   if (mPreviousBand != (imageInfo.mKey.mBand1.getActiveNumber() + 1))
   {
      // band change has occurred and ALL the tiles need to be updated
      // not just the ones that are visable and will be drawn to the screen
      const vector<Tile*>* pTileSet = getActiveTiles();
      if (pTileSet != NULL)
      {
         int numTiles = pTileSet->size();

         vector<Tile*> tilesToUpdate;
         tilesToUpdate.reserve(numTiles);

         tileZoomIndices.clear();
         tileZoomIndices.reserve(numTiles);

         vector<Tile*>::const_iterator iter;
         for (iter = pTileSet->begin(); iter != pTileSet->end(); ++iter)
         {
            Tile* pTile = *iter;
            if (pTile != NULL)
            {
               tilesToUpdate.push_back(pTile);
               tileZoomIndices.push_back(pTile->getTextureIndex());
            }
         }

         mPreviousBand = imageInfo.mKey.mBand1.getActiveNumber() + 1;
         return tilesToUpdate;
      }
   }

   return Image::getTilesToUpdate(tilesToDraw, tileZoomIndices);
}

unsigned int GpuImage::readTiles(double xCoord, double yCoord, GLsizei width, GLsizei height, vector<float>& values, bool& hasAlphas)
{
   if ((width == 0) || (height == 0))
   {
      values.resize(0);
      hasAlphas = false;
      return 0;
   }

   // get the active tile set
   const vector<Tile*>* pTileSet = getActiveTiles();
   VERIFY(pTileSet != NULL);

   int x1Coord = static_cast<int>(xCoord);
   int y1Coord = static_cast<int>(yCoord);

   GLsizei calculatedWidth = width;
   GLsizei calculatedHeight = height;

   vector<Tile*> tiles;
   vector<LocationType> tileLocations;

   // get the tiles and their locations within the image that have the requested data
   getTilesToRead(x1Coord, y1Coord, width, height, tiles, tileLocations);

   vector<GLsizei> tileWidths;
   vector<GLsizei> tileHeights;
   vector<GLsizei> sourceOffsets;
   vector<GLsizei> destinationOffsets;
   vector<float> dataVector;

   unsigned int numElements = 0;

   size_t tileNum = 0;
   size_t numTiles = tiles.size();

   if (numTiles == 0)
   {
      return 0;
   }

   GpuTile* pTile = dynamic_cast<GpuTile*>(tiles.front());
   if (pTile == NULL)
   {
      return 0;
   }

   std::vector<ImageFilterDescriptor*> filters = pTile->getFilters();
   if (filters.empty())
   {
      return 0;
   }

   ImageFilter* pFilter = pTile->getFilter(filters.front());
   if (pFilter == NULL)
   {
      return 0;
   }

   // need to get results filter buffer in order to determine how many channels there are in the texture
   ColorBuffer* pColorBuffer = pFilter->getResultsBuffer();
   if (pColorBuffer == NULL)
   {
      return 0;
   }

   GLenum textureFormat = pColorBuffer->getTextureFormat();
   unsigned int numChannels = ImageUtilities::getNumColorChannels(textureFormat);

   if (numChannels == 2 || numChannels == 4) // GL_LUMINANCE_ALPHA or GL_RGBA
   {
      hasAlphas = true;
   }
   else // GL_LUMINANCE or GL_RGB
   {
      hasAlphas = false;
   }

   if (values.size() < width * height * numChannels)
   {
      values.resize(width * height * numChannels);
   }

   if (values.empty() == true)
   {
      return 0;
   }

   // The data will be read into pValueData. Down below we may point this
   // to a different memory buffer if we determine that we need to reorder the
   // data before returning it.
   float* pValueData = &values[0];

   tileWidths.reserve(numTiles);
   tileHeights.reserve(numTiles);
   sourceOffsets.reserve(numTiles);
   destinationOffsets.reserve(numTiles);

   // The values in the vector are ordered according to rows, so if the tiles 
   // are side-by-side, we will need to reorder the values later. 

   // If there are 2 or more tiles, it is hard to test for the need to
   // reorder the data, so we will simply assume that we do.
   dataVector.resize(width * height * numChannels);
   pValueData = &dataVector[0];

   vector<Tile*>::iterator tileIter = tiles.begin();

   // read the tiles
   while (tileIter != tiles.end())
   {
      calculatedWidth = width;
      calculatedHeight = height;

      unsigned int tileElements = readTile(*tileIter, tileLocations.at(tileNum), x1Coord, y1Coord, 
                              calculatedWidth, calculatedHeight, pValueData);

      // set the initial offsets for the tiles
      if (tileNum == 0)
      {
         destinationOffsets.push_back(0);
      }
      else
      {
         // check to see if current tile is on the same row as the previous tile
         if (tileLocations.at(tileNum).mY == tileLocations.at(tileNum-1).mY)
         {
            destinationOffsets.push_back(tileWidths.at(tileNum-1) + destinationOffsets.at(tileNum-1));
         }
         else
         {
            destinationOffsets.push_back(numElements);
         }
      }

      sourceOffsets.push_back(numElements);
      numElements += tileElements;

      // move the pointer to the next position in the array
      pValueData += tileElements;

      tileWidths.push_back(calculatedWidth * numChannels);
      tileHeights.push_back(calculatedHeight);

      tileNum++;
      tileIter++;
   }

   // Reorder the data into the destination vector to be row order
   pValueData = &values[0];
   for (tileNum = 0; tileNum < numTiles; tileNum++)
   {
      for (int row = 0; row < tileHeights[tileNum]; ++row)
      {
         memcpy((pValueData + destinationOffsets[tileNum]), &dataVector[sourceOffsets[tileNum]],
            (sizeof(float) * tileWidths[tileNum]));

         // set source and destination offsets for next pass
         sourceOffsets[tileNum] += tileWidths[tileNum];
         destinationOffsets[tileNum] += width * numChannels;
      }
   }

      // get scale factor
   float gpuFactor = Service<GpuResourceManager>()->getGpuScalingFactor(textureFormat);
   float scaleFactor = 1.0f / gpuFactor;
   float offset = 0.0;
   switch (mInfo.mRawType[0])
   {
   case INT1SBYTE:
      scaleFactor = getScale<signed char>() / gpuFactor;
      offset = static_cast<float>(getOffset<signed char>());
      break;
   case INT1UBYTE:
      scaleFactor = getScale<unsigned char>() / gpuFactor;
      offset = static_cast<float>(getOffset<unsigned char>());
      break;
   case INT2SBYTES:
      scaleFactor = getScale<signed short>() / gpuFactor;
      offset = static_cast<float>(getOffset<signed short>());
      break;
   case INT2UBYTES:
      scaleFactor = getScale<unsigned short>() / gpuFactor;
      offset = static_cast<float>(getOffset<unsigned short>());
      break;
   case INT4SBYTES:
      scaleFactor = getScale<signed int>() / gpuFactor;
      offset = static_cast<float>(getOffset<signed int>());
      break;
   case INT4UBYTES:
      scaleFactor = getScale<unsigned int>() / gpuFactor;
      offset = static_cast<float>(getOffset<unsigned int>());
      break;
   case FLT4BYTES:
      scaleFactor = getScale<float>() / gpuFactor;
      offset = static_cast<float>(getOffset<float>());
      break;
   case FLT8BYTES:
      scaleFactor = getScale<double>() / gpuFactor;
      offset = static_cast<float>(getOffset<double>());
      break;
   default:
      break;
   }

   // scale the filtered results
   for (unsigned int element = 0; element < numElements; element++)
   {
      values[element] *= scaleFactor;
      values[element] += offset;
   }

   return numElements;
}

void GpuImage::getTilesToRead(int xCoord, int yCoord, GLsizei width, GLsizei height, 
                              vector<Tile*> &tiles, vector<LocationType> &tileLocations)
{
   tiles.clear();
   tileLocations.clear();

   const vector<Tile*>* pTileSet = getActiveTiles();
   VERIFYNRV(pTileSet != NULL);

   const ImageData& imageData = getImageData();
   int imageWidth = imageData.mImageSizeX;

   LocationType tileGeomSize;
   bool inTile = false;
   vector<Tile*>::const_iterator tileIter = pTileSet->begin();
   LocationType tileLocation;
   int currentWidth = 0;
   int rowNum = 0;
   int colNum = 0;
   int x2Coord = xCoord + width;
   int y2Coord = yCoord + height;
   while (tileIter != pTileSet->end())
   {
      tileGeomSize = (*tileIter)->getGeomSize();

      inTile = 
         (colNum+1)*tileGeomSize.mX >= xCoord &&
         colNum*tileGeomSize.mX < x2Coord &&
         (rowNum+1)*tileGeomSize.mY >= yCoord &&
         rowNum*tileGeomSize.mY < y2Coord;

      if (inTile)
      {
         tileLocation.mX = colNum;
         tileLocation.mY = rowNum;
         tileLocations.push_back(tileLocation);
         tiles.push_back(*tileIter);
      }

      currentWidth += static_cast<int>(tileGeomSize.mX);
      if (currentWidth >= imageWidth)
      {
         ++rowNum;
         colNum = 0;
         currentWidth = 0;
      }
      else
      {
         ++colNum;
      }
      ++tileIter;
   }
}

unsigned int GpuImage::readTile(Tile *pTile, const LocationType &tileLocation, int x1Coord, int y1Coord, 
                                GLsizei &calculatedWidth, GLsizei &calculatedHeight, GLvoid *pValues)
{
   LocationType geomSize = pTile->getGeomSize();
   int x1TileCoord = static_cast<int>(tileLocation.mX * geomSize.mX);
   int y1TileCoord = static_cast<int>(tileLocation.mY * geomSize.mY);
   int x2TileCoord = static_cast<int>((tileLocation.mX + 1.0) * geomSize.mX);
   int y2TileCoord = static_cast<int>((tileLocation.mY + 1.0) * geomSize.mY);

   int x2Coord = x1Coord + calculatedWidth;
   int y2Coord = y1Coord + calculatedHeight;

   // convert coordinates since each tile's image buffer has coordinates
   // starting at (0,0)

   int left = max(x1Coord, x1TileCoord);
   int top = max(y1Coord, y1TileCoord);

   int calculatedXCoord = left - x1TileCoord;
   int calculatedYCoord = top - y1TileCoord;

   calculatedWidth = min(x2Coord, x2TileCoord) - left;
   calculatedHeight = min(y2Coord, y2TileCoord) - top;

   GpuTile* pGpuTile = static_cast<GpuTile*>(pTile);
   return (pGpuTile->readFilterBuffer(calculatedXCoord, calculatedYCoord, calculatedWidth, calculatedHeight, pValues));
}
