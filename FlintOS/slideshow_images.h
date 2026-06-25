/*
 * File: slideshow_images.h
 *    Purpose: Flash memory repository for high-resolution photo slideshow arrays.
 *    * Image Array Map:
 *    - img_panel_0 through img_panel_4: 800x480 full-frame landscape image assets.
 * =========================================================================
 * HOW TO CONVERT YOUR PERSONAL IMAGES TO CODE ARRAYS:
 * =========================================================================
 *   Step 1: Choose your favorite image. Resize it to exactly 800x480 pixels. 
 *         (Landscape-oriented images look best on this layout).
 *
 *   Step 2: Open your web browser and navigate to the image2cpp converter tool:
 *           Link -> https://javl.github.io/image2cpp/
 *
 *   Step 3: Upload your image. In the 'Image Settings' block:
 *           - Change Dithering to 'Atkinson' or 'Floyd-Steinberg' to get a crisp e-ink sketch print.
 *           - Adjust the 'Brightness / Alpha Threshold' slider (works only with Atkinson)
 *           while checking the preview container to confirm image look sharp.
 *
 *   Step 4: Scroll down to the 'Output' zone:
 *           - Set Code Output Format to 'Plain bytes'.
 *           - Click the 'Generate code' button.
 *
 *   Step 5: Copy the generated data block and paste it inside the curly brackets 
 *           of your target variable slot below (e.g., replace the body of img_panel_0).
 *
 * =====================================================================================================================
 */
 
#ifndef _SLIDESHOW_IMAGES_H_
#define _SLIDESHOW_IMAGES_H_

#include <pgmspace.h>

const int NUM_SLIDESHOW_IMAGES = 5;

const unsigned char img_panel_0[] PROGMEM = {
};

const unsigned char img_panel_1[] PROGMEM = {
};

const unsigned char img_panel_2[] PROGMEM = {
};

const unsigned char img_panel_3[] PROGMEM = {
};

const unsigned char img_panel_4[] PROGMEM = {
};

const unsigned char* const slideshow_images[NUM_SLIDESHOW_IMAGES] = {
  img_panel_0,
  img_panel_1,
  img_panel_2,
  img_panel_3,
  img_panel_4
};

#endif