
#include <cstdint>

namespace STLL
{

/** \brief apply a gaussian blurr to a 1 channel image
 *
 * \param s the byte array to apply the blurr to, it is assumed to point to w*h bytes
 * \param w the width of the image
 * \param h the hight of the data
 * \param r the radius to spread the data over
 * \param sx scaling factor for the x direction, using a value bigger than 1 will increase the blurr radius accordingly
 * \param sy scaling factor for the y direction, using a value bigger than 1 will increase the blurr radius accordingly
 */
void gaussBlur (uint8_t * s, int w, int h, double r, int sx, int sy);

};


