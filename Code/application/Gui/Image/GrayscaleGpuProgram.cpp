#include "GrayscaleGpuProgram.h"
#include "GpuProgramDescriptorAdapter.h"


/**
* Constructor
*/
GrayscaleGpuProgram::GrayscaleGpuProgram()
{
   GpuProgramDescriptorAdapter* pGpuDescriptor = new GpuProgramDescriptorAdapter();
   pGpuDescriptor->setName("GrayscaleDisplay.glsl");
   pGpuDescriptor->setType(GpuProgramDescriptor::FRAGMENT_PROGRAM);
   setGpuDescriptor(pGpuDescriptor);

}

/**
* Initialize the program. This must be called in the EventDispatchThread.
* 
* @throws IOException
*            If the descriptor source is invalid.
*/
void GrayscaleGpuProgram::initialize()
{
   DataVariant value(std::string("0"));
   mpGpuProgramDescriptor->setParameter("inputImage", value);

   DataVariant defaultStretch(std::string("0.0"));
   mpGpuProgramDescriptor->setParameter("dataMax", defaultStretch);
   mpGpuProgramDescriptor->setParameter("lowerValue", defaultStretch);
   mpGpuProgramDescriptor->setParameter("upperValue", defaultStretch);
   mpGpuProgramDescriptor->setParameter("alphaValue", defaultStretch);
   mpGpuProgramDescriptor->setParameter("minBound", defaultStretch);
   mpGpuProgramDescriptor->setParameter("maxBound", defaultStretch);
   GlShaderGpuProgram::initialize();
}