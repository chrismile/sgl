//
// Created by christoph on 12.09.18.
//

#ifndef SGL_HIDPI_HPP
#define SGL_HIDPI_HPP

namespace sgl
{

/**
 * @return The scale factor used for scaling fonts/the UI on the system.
 */
DLL_OBJECT float getHighDPIScaleFactor();

/**
 * @return Overwrites the scaling factor with a manually chosen value.
 */
DLL_OBJECT void overwriteHighDPIScaleFactor(float scaleFactor);

}

#endif //SGL_HIDPI_HPP
